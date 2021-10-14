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

ae::file::read_access::read_access(std::string_view aFilename)
{
    if (aFilename == "-") {
        constexpr size_t chunk_size = 1024 * 100;
        data_.resize(chunk_size);
        ssize_t start = 0;
        for (;;) {
            if (const auto bytes_read = ::read(0, data_.data() + start, chunk_size); bytes_read > 0) {
                start += bytes_read;
                data_.resize(static_cast<size_t>(start));
            }
            else
                break;
        }
    }
    else if (std::filesystem::exists(aFilename)) {
        len_ = std::filesystem::file_size(aFilename);
        fd = ::open(aFilename.data(), O_RDONLY);
        if (fd >= 0) {
            mapped_ = reinterpret_cast<char*>(mmap(nullptr, len_, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0));
            if (mapped_ == MAP_FAILED)
                throw cannot_read{fmt::format("{}: {}", aFilename, strerror(errno))};
        }
        else {
            throw not_opened{fmt::format("{}: {}", aFilename, strerror(errno))};
        }
    }
    else {
        throw not_found{std::string{aFilename}};
    }

} // ae::file::read_access

// ----------------------------------------------------------------------

ae::file::read_access::read_access(read_access&& other)
    : fd{other.fd}, len_{other.len_}, mapped_{other.mapped_}, data_{other.data_}
{
    other.fd = -1;
    other.len_ = 0;
    other.mapped_ = nullptr;
    other.data_.clear();

} // ae::file::read_access::read_access

// ----------------------------------------------------------------------

ae::file::read_access::~read_access()
{
    if (fd > 2) {
        if (mapped_)
            munmap(mapped_, len_);
        close(fd);
    }

} // ae::file::read_access::~read_access

// ----------------------------------------------------------------------

ae::file::read_access& ae::file::read_access::operator=(read_access&& other)
{
    fd = other.fd;
    len_ = other.len_;
    mapped_ = other.mapped_;
    data_ = other.data_;

    other.fd = -1;
    other.len_ = 0;
    other.mapped_ = nullptr;
    other.data_.clear();

    return *this;

} // ae::file::read_access::operator=

// ----------------------------------------------------------------------

std::string ae::file::decompress_if_necessary(std::string_view aSource)
{
    if (xz_compressed(aSource.data()))
        return xz_decompress(aSource);
    else if (bz2_compressed(aSource.data()))
        return bz2_decompress(aSource);
    else if (gzip_compressed(aSource.data())) {
        return gzip_decompress(aSource);
    }
    else
        return std::string(aSource);

} // ae::file::decompress_if_necessary

// ----------------------------------------------------------------------

std::string ae::file::read_from_file_descriptor(int fd, size_t chunk_size)
{
    std::string buffer;
    std::string::size_type offset = 0;
    for (;;) {
        buffer.resize(buffer.size() + chunk_size, ' ');
        const auto bytes_read = ::read(fd, (&*buffer.begin()) + offset, chunk_size);
        if (bytes_read < 0)
            throw std::runtime_error(std::string("Cannot read from file descriptor: ") + strerror(errno));
        if (static_cast<size_t>(bytes_read) < chunk_size) {
            buffer.resize(buffer.size() - chunk_size + static_cast<size_t>(bytes_read));
            break;
        }
        offset += static_cast<size_t>(bytes_read);
    }
    return decompress_if_necessary(std::string_view(buffer));

} // ae::file::read_from_file_descriptor

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
            const auto compressed = aFilename.ends_with(".gz") ? gzip_compress(aData) : xz_compress(aData);
            if (::write(f, compressed.data(), compressed.size()) < 0)
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
