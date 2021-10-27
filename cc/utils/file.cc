#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/mman.h>

#include <filesystem>

#include "utils/file.hh"
// #include "acmacs-base/string-compare.hh"
#include "ext/xz.hh"
#include "ext/bzip2.hh"
#include "ext/gzip.hh"
#include "ext/date.hh"

// ----------------------------------------------------------------------

namespace ae::file::detail
{

    inline std::unique_ptr<Compressor> compressor_factory(std::string_view initial_bytes, std::string_view filename, size_t padding)
    {
        if ((!initial_bytes.empty() && xz_compressed(initial_bytes)) || (!filename.empty() && filename.ends_with(".xz")))
            return std::make_unique<XZ_Compressor>(padding);
        else if ((!initial_bytes.empty() && bz2_compressed(initial_bytes)) || (!filename.empty() && filename.ends_with(".bz2")))
            return std::make_unique<BZ2_Compressor>(padding);
        else if ((!initial_bytes.empty() && gzip_compressed(initial_bytes)) || (!filename.empty() && filename.ends_with(".gz")))
            return std::make_unique<GZIP_Compressor>(padding);
        else
            return std::make_unique<NotCompressed>(padding);

    } // ae::file::read_access::compressor_factory

} // namespace ae::file::detail

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------

ae::file::read_access::read_access(const std::filesystem::path& filename, size_t padding) : filename_{filename}, padding_{padding}
{
    if (filename == "-") {
        fd_ = 0;
        decompressed_ = next_chunk(initial::yes);
    }
    else if (std::filesystem::exists(filename)) {
        mapped_len_ = std::filesystem::file_size(filename);
        fd_ = ::open(filename.c_str(), O_RDONLY);
        if (fd_ >= 0) {
            mapped_ = reinterpret_cast<char*>(mmap(nullptr, mapped_len_, PROT_READ, MAP_FILE | MAP_PRIVATE, fd_, 0));
            if (mapped_ == MAP_FAILED)
                throw cannot_read{fmt::format("{}: {}", filename.native(), strerror(errno))};
            decompressed_ = next_chunk(initial::yes);
        }
        else {
            throw not_opened{fmt::format("{}: {}", filename.native(), strerror(errno))};
        }
    }
    else {
        throw not_found{std::string{filename}};
    }

} // ae::file::read_access::read_access

// ----------------------------------------------------------------------

ae::file::read_access::read_access(int fd, size_t chunk_size, size_t padding) : chunk_size_{chunk_size}, fd_{fd}, padding_{padding}
{
    decompressed_ = next_chunk(initial::yes);
}

// ----------------------------------------------------------------------

ae::file::read_access::~read_access()
{
    if (fd_ > 2) {
        if (mapped_)
            munmap(mapped_, mapped_len_);
        close(fd_);
    }

} // ae::file::read_access::~read_access

// ----------------------------------------------------------------------

std::string_view ae::file::read_access::rest()
{
    return decompressed_.substr(decompressed_offset_);

} // ae::file::read_access::rest

// ----------------------------------------------------------------------

std::pair<std::string_view, bool> ae::file::read_access::line()
{
    if (const auto eol = decompressed_.find('\n', decompressed_offset_); eol == std::string_view::npos) {
        const auto result = decompressed_.substr(decompressed_offset_);
        decompressed_offset_ = decompressed_.size();
        return {result, result.size() == 0};
    }
    else {
        const auto result = decompressed_.substr(decompressed_offset_, eol - decompressed_offset_);
        decompressed_offset_ = eol + 1;
        return {result, false};
    }

} // ae::file::read_access::line

// ----------------------------------------------------------------------

std::string_view ae::file::read_access::next_chunk(initial ini)
{
    if (mapped_) {
        if (ini == initial::yes)
            compressor_ = detail::compressor_factory(std::string_view(mapped_, mapped_len_), {}, padding_);
        return compressor_->decompress(std::string_view(mapped_, mapped_len_), Compressor::first_chunk::yes);
    }
    else {
        data_.reserve(chunk_size_ + padding_);
        data_.resize(chunk_size_);
        ssize_t start = 0;
        for (;;) {
            if (const auto bytes_read = ::read(0, data_.data() + start, chunk_size_); bytes_read > 0) {
                start += bytes_read;
                data_.reserve(static_cast<size_t>(start) + chunk_size_ + padding_);
                data_.resize(static_cast<size_t>(start));
            }
            else if (bytes_read < 0)
                throw cannot_read{fmt::format("{}: {}", filename_, strerror(errno))};
            else
                break;
        }
        if (ini == initial::yes)
            compressor_ = detail::compressor_factory(data_, {}, padding_);
        return compressor_->decompress(data_, Compressor::first_chunk::yes);
    }

} // ae::file::read_access::next_chunk

// ----------------------------------------------------------------------

void ae::file::backup(std::string_view _to_backup, std::string_view _backup_dir, backup_move bm)
{
    const std::filesystem::path to_backup{_to_backup}, backup_dir{_backup_dir};

    if (std::filesystem::exists(to_backup)) {
        try {
            std::filesystem::create_directory(backup_dir);
        }
        catch (std::exception& err) {
            fmt::print(stderr, "> ERROR cannot create directory {}: {}\n", backup_dir.native(), err.what());
            throw;
        }

        auto extension = to_backup.extension();
        auto stem = to_backup.stem();
        if ((extension == ".bz2" || extension == ".xz" || extension == ".gz") && !stem.extension().empty()) {
            extension = stem.extension();
            extension += to_backup.extension();
            stem = stem.stem();
        }
        const auto today = fmt::format("%Y%m%d", date::today());
        for (int version = 1; version < 1000; ++version) {
            char infix[4];
            std::sprintf(infix, "%03d", version);
            std::filesystem::path new_name = backup_dir / (stem.string() + ".~" + today + '-' + infix + '~' + extension.string());
            if (!std::filesystem::exists(new_name) || version == 999) {
                try {
                    if (bm == backup_move::yes)
                        std::filesystem::rename(to_backup, new_name); // if new_name exists it will be removed before doing rename
                    else
                        std::filesystem::copy_file(to_backup, new_name, std::filesystem::copy_options::overwrite_existing);
                }
                catch (std::exception& err) {
                    fmt::print(stderr, ">> WARNING backing up \"{}\" to \"{}\" failed: {}\n", to_backup.native(), new_name.native(), err.what());
                }
                break;
            }
        }
    }

} // ae::file::backup

// ----------------------------------------------------------------------

void ae::file::backup(const std::string_view to_backup, backup_move bm)
{
    backup(to_backup, (std::filesystem::path{to_backup}.parent_path() / ".backup").native(), bm);

} // ae::file::backup

// ----------------------------------------------------------------------

void ae::file::write(std::string_view aFilename, std::string_view aData, force_compression aForceCompression, backup_file aBackupFile)
{
    using namespace std::string_view_literals;
    int f = -1;
    if (aFilename == "-") {
        f = 1;
    }
    else if (aFilename == "=") {
        f = 2;
    }
    else if (aFilename == "/") {
        f = open("/dev/null", O_WRONLY | O_TRUNC | O_CREAT, 0644);
        if (f < 0)
            throw std::runtime_error(fmt::format("Cannot open /dev/null: {}", strerror(errno)));
    }
    else {
        if (aBackupFile == backup_file::yes && aFilename.substr(0, 4) != "/dev") // allow writing to /dev/ without making backup attempt
            backup(aFilename);
        f = open(aFilename.data(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
        if (f < 0)
            throw std::runtime_error(fmt::format("Cannot open {}: {}", aFilename, strerror(errno)));
    }
    try {
        if (aForceCompression == force_compression::yes || (aFilename.size() > 3 && (aFilename.ends_with(".xz") || aFilename.ends_with(".gz")))) {
            auto compressor = detail::compressor_factory({}, aFilename, 0);
            const auto data = compressor->compress(aData);
            if (::write(f, data.data(), data.size()) < 0)
                throw std::runtime_error(fmt::format("Cannot write {}: {}", aFilename, strerror(errno)));
        }
        else {
            if (::write(f, aData.data(), aData.size()) < 0)
                throw std::runtime_error(fmt::format("Cannot write {}: {}", aFilename, strerror(errno)));
        }
        if (f > 2)
            close(f);
    }
    catch (std::exception&) {
        if (f > 2)
            close(f);
        throw;
    }

} // ae::file::write

// ----------------------------------------------------------------------

ae::file::temp::temp(std::string_view prefix, std::string_view suffix, bool autoremove)
    : name_{fmt::format("{}{}", make_template(prefix), suffix)}, autoremove_{autoremove}, fd_(mkstemps(const_cast<char*>(name_.c_str()), static_cast<int>(suffix.size())))
{
    if (fd_ < 0)
        throw std::runtime_error(std::string("Cannot create temporary file using template ") + name_ + ": " + strerror(errno));

} // ae::file::temp::temp

// ----------------------------------------------------------------------

ae::file::temp::~temp()
{
    if (autoremove_ && !name_.empty())
        std::filesystem::remove(name_.c_str());

} // ae::file::temp::~temp

// ----------------------------------------------------------------------

std::string ae::file::temp::make_template(std::string_view prefix)
{
    const char* tdir = std::getenv("TMPDIR");
    if (!tdir)
        tdir = "/tmp";
    return fmt::format("{}/{}.XXXXXXXX", tdir, prefix);

} // ae::file::temp::make_template

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
