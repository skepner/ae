#pragma once

#include <stdexcept>
#include <string_view>
#include <memory>
#include <filesystem>

#include "ext/fmt.hh"
#include "ext/compressor.hh"

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wpadded"
#endif

// ----------------------------------------------------------------------

namespace ae::file
{
    enum class force_compression { no, yes };
    enum class backup_file { no, yes };
    enum class backup_move { no, yes };

      // ----------------------------------------------------------------------

    std::string decompress_if_necessary(std::string_view aSource, size_t padding = 0); // padding to support simdjson

      // ----------------------------------------------------------------------

    class file_error : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    class not_opened : public file_error { public: not_opened(std::string_view aMsg) : file_error(fmt::format("cannot open {}", aMsg)) {} };
    class cannot_read : public file_error { public: cannot_read(std::string_view aMsg) : file_error(fmt::format("cannot read {}", aMsg)) {} };
    class not_found : public file_error { public: not_found(std::string_view aFilename) : file_error(fmt::format("not found: {}", aFilename)) {} };

    std::string read(const std::filesystem::path& filename, size_t padding = 0);
    // inline read_access read_from_file_descriptor(int fd, size_t chunk_size = 1024) { return read_access(fd, chunk_size); }
    // inline read_access read_stdin() { return read_from_file_descriptor(0); }
    void write(std::string_view aFilename, std::string_view aData, force_compression aForceCompression = force_compression::no, backup_file aBackupFile = backup_file::yes);

    void backup(std::string_view to_backup, std::string_view backup_dir, backup_move bm = backup_move::no);
    void backup(std::string_view to_backup, backup_move bm = backup_move::no);

    class temp
    {
     public:
        temp(std::string_view prefix, std::string_view suffix, bool autoremove = true);
        temp(std::string_view suffix, bool autoremove = true) : temp("ae.", suffix, autoremove) {}
        ~temp();

        temp& operator = (temp&& from) noexcept { name_ = std::move(from.name_); fd_ = from.fd_; from.name_.clear(); return *this; }
        operator std::string_view() const { return name_; }
        // constexpr operator int() const { return fd_; }

     private:
        std::string name_;
        bool autoremove_;
        int fd_;

        std::string make_template(std::string_view prefix);

    }; // class temp

} // namespace acmacs::file

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
