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

    class read_access
    {
      public:
        // read_access() = default;
        read_access(const std::filesystem::path& filename, size_t padding = 0);
        read_access(int fd, size_t chunk_size, size_t padding = 0);
        ~read_access();
        read_access(const read_access&) = delete;
        read_access(read_access&&) = default;
        read_access& operator=(const read_access&) = delete;
        read_access& operator=(read_access&&) = default;

        // operator std::string() const { return mapped_ ? decompress_if_necessary({mapped_, len_}, padding_) : decompress_if_necessary(data_, padding_); }
        // bool valid() const { return mapped_ != nullptr || !data_.empty(); }

        std::string_view rest();
        operator std::string_view() { return rest(); }
        operator std::string() { return std::string(rest()); }
        std::pair<std::string_view, bool> line(); // line, end_of_file

      private:
        enum class initial { no, yes };

        size_t chunk_size_{1024 * 1024 * 10};
        std::unique_ptr<Compressor> compressor_{};
        std::string filename_{};
        int fd_{-1};
        size_t mapped_len_{0};
        char* mapped_{nullptr};
        std::string data_{};
        std::string_view decompressed_{};
        size_t decompressed_offset_{0};
        size_t padding_{0}; // to support simdjson

        std::string_view next_chunk(initial ini = initial::no);

    }; // class read_access

    inline read_access read(const std::filesystem::path& filename, size_t padding = 0) { return read_access{filename, padding}; }
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
