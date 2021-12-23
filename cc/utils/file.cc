#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <sys/mman.h>

#include <filesystem>

#include "utils/file.hh"
#include "ext/xz.hh"
#include "ext/bzip2.hh"
#include "ext/gzip.hh"
#include "ext/date.hh"

// ----------------------------------------------------------------------

namespace ae::file::detail
{
    inline std::unique_ptr<Compressor> compressor_factory(std::string_view initial_bytes, const std::filesystem::path& filename, force_compression fc, size_t padding)
    {
        if ((!initial_bytes.empty() && xz_compressed(initial_bytes)) || extension_of(filename, {".xz", ".tjz", ".jxz"}))
            return std::make_unique<XZ_Compressor>(padding);
        else if ((!initial_bytes.empty() && bz2_compressed(initial_bytes)) || extension_of(filename, {".bz2"}))
            return std::make_unique<BZ2_Compressor>(padding);
        else if ((!initial_bytes.empty() && gzip_compressed(initial_bytes)) || extension_of(filename, {".gz"}))
            return std::make_unique<GZIP_Compressor>(padding);
        else if (fc == force_compression::yes)
            return std::make_unique<XZ_Compressor>(padding);
        else
            return nullptr;

    } // ae::file::read_access::compressor_factory

    inline std::string read_stdin(size_t padding)
    {
        constexpr size_t chunk_size = 1024 * 100;
        std::string data;
        data.reserve(chunk_size + padding);
        data.resize(chunk_size);
        ssize_t start = 0;
        for (;;) {
            if (const auto bytes_read = ::read(0, data.data() + start, chunk_size); bytes_read > 0) {
                start += bytes_read;
                data.resize(static_cast<size_t>(start));
                data.reserve(static_cast<size_t>(start) + padding);
            }
            else
                break;
        }
        return decompress_if_necessary(data, padding);
    }

    class mmapped
    {
      public:
        mmapped(const std::filesystem::path& filename)
        {
            if (std::filesystem::exists(filename)) {
                mmapped_len_ = std::filesystem::file_size(filename);
                fd_ = ::open(filename.c_str(), O_RDONLY);
                if (fd_ >= 0) {
                    mmapped_ = reinterpret_cast<char*>(mmap(nullptr, mmapped_len_, PROT_READ, MAP_FILE | MAP_PRIVATE, fd_, 0));
                    if (mmapped_ == MAP_FAILED)
                        throw cannot_read{fmt::format("{}: {}", filename.native(), strerror(errno))};
                }
                else
                    throw not_opened{fmt::format("{}: {}", filename.native(), strerror(errno))};
            }
            else
                throw not_found{std::string{filename}};
        }
        mmapped(const mmapped&) = delete;
        mmapped operator=(const mmapped&) = delete;

        ~mmapped()
        {
            if (fd_ > 2) {
                if (mmapped_)
                    munmap(mmapped_, mmapped_len_);
                close(fd_);
            }
        }

        operator std::string_view() { return {mmapped_, mmapped_len_}; }

      private:
        int fd_{-1};
        char* mmapped_{nullptr};
        size_t mmapped_len_{0};
    };

    inline std::string read_via_mmap(const std::filesystem::path& filename, size_t padding)
    {
        mmapped mapped{filename};
        return decompress_if_necessary(mapped, padding);
    }

} // namespace ae::file::detail

// ----------------------------------------------------------------------

std::string ae::file::read(const std::filesystem::path& filename, size_t padding)
{
    if (filename == "-")
        return detail::read_stdin(padding);
    else
        return detail::read_via_mmap(filename, padding);

} // ae::file::read

// ----------------------------------------------------------------------

std::string ae::file::decompress_if_necessary(std::string_view source, size_t padding)
{
    if (auto compressor = detail::compressor_factory(source, {}, force_compression::no, padding); compressor) {
        return compressor->decompress(source);
    }
    else {
        std::string output;
        output.reserve(source.size() + padding);
        output = source;
        return output;
    }
}

void ae::file::backup(const std::filesystem::path& to_backup, const std::filesystem::path& backup_dir, backup_move bm)
{
    if (std::filesystem::exists(to_backup)) {
        try {
            std::filesystem::create_directory(backup_dir);
        }
        catch (std::exception& err) {
            fmt::print(stderr, "> cannot create directory {}: {}\n", backup_dir.native(), err.what());
            throw;
        }

        auto extension = to_backup.extension();
        auto stem = to_backup.stem();
        if ((extension == ".bz2" || extension == ".xz" || extension == ".gz") && !stem.extension().empty()) {
            extension = stem.extension();
            extension += to_backup.extension();
            stem = stem.stem();
        }
        const auto now = fmt::format("{:%Y-%m%d-%H%M%S}", std::chrono::system_clock::now());
        for (int version = 1; version < 1000; ++version) {
            const std::filesystem::path new_name = backup_dir / fmt::format("{}.~{}{}~{}", stem.string(), now, version == 1 ? std::string{} : fmt::format("{:03d}", version), extension.string());
            if (!std::filesystem::exists(new_name) || version == 999) {
                // fmt::print(">>>> making backup \"{}\" -> \"{}\"\n", to_backup, new_name);
                try {
                    if (bm == backup_move::yes)
                        std::filesystem::rename(to_backup, new_name); // if new_name exists it will be removed before doing rename
                    else
                        std::filesystem::copy_file(to_backup, new_name, std::filesystem::copy_options::overwrite_existing);
                }
                catch (std::exception& err) {
                    fmt::print(stderr, ">> backing up \"{}\" to \"{}\" failed: {}\n", to_backup.native(), new_name.native(), err.what());
                }
                break;
            }
        }
    }

} // ae::file::backup

// ----------------------------------------------------------------------

void ae::file::backup(const std::filesystem::path& to_backup, backup_move bm)
{
    backup(to_backup, (std::filesystem::path{to_backup}.parent_path() / ".backup").native(), bm);

} // ae::file::backup

// ----------------------------------------------------------------------

void ae::file::write(const std::filesystem::path& filename, std::string_view data, force_compression aForceCompression, backup_file a_backup_file)
{
    using namespace std::string_view_literals;
    int f = -1;
    if (filename == "-") {
        f = 1;
    }
    else if (filename == "=") {
        f = 2;
    }
    else if (filename == "/") {
        f = open("/dev/null", O_WRONLY | O_TRUNC | O_CREAT, 0644);
        if (f < 0)
            throw std::runtime_error(fmt::format("Cannot open /dev/null: {}", strerror(errno)));
    }
    else {
        if (a_backup_file == backup_file::yes && filename.native().substr(0, 4) != "/dev") // allow writing to /dev/ without making backup attempt
            backup(filename);
        f = open(filename.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
        if (f < 0)
            throw std::runtime_error(fmt::format("Cannot open {}: {}", filename, strerror(errno)));
    }
    try {
        if (aForceCompression == force_compression::yes || extension_of(filename, {".xz", ".gz", ".tjz", ".jxz"})) {
            std::string compressed_data;
            if (auto compressor = detail::compressor_factory({}, filename, aForceCompression, 0); compressor) {
                compressed_data = compressor->compress(data);
                data = compressed_data;
            }
            if (::write(f, data.data(), data.size()) < 0)
                throw std::runtime_error(fmt::format("Cannot write {}: {}", filename, strerror(errno)));
        }
        else {
            if (::write(f, data.data(), data.size()) < 0)
                throw std::runtime_error(fmt::format("Cannot write {}: {}", filename, strerror(errno)));
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

// ======================================================================
