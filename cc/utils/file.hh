#pragma once

#include <stdexcept>
#include <string_view>

#include "ext/fmt.hh"

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

    std::string decompress_if_necessary(std::string_view aSource);

      // ----------------------------------------------------------------------

    class file_error : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    class not_opened : public file_error { public: not_opened(std::string_view aMsg) : file_error(fmt::format("cannot open {}", aMsg)) {} };
    class cannot_read : public file_error { public: cannot_read(std::string_view aMsg) : file_error(fmt::format("cannot read {}", aMsg)) {} };
    class not_found : public file_error { public: not_found(std::string_view aFilename) : file_error(fmt::format("not found: {}", aFilename)) {} };

    class read_access
    {
     public:
        read_access() = default;
        read_access(std::string_view aFilename);
        ~read_access();
        read_access(const read_access&) = delete;
        read_access(read_access&&);
        read_access& operator=(const read_access&) = delete;
        read_access& operator=(read_access&&);
        operator std::string() const { return mapped_ ? decompress_if_necessary({mapped_, len_}) : decompress_if_necessary(data_); }
        size_t size() const { return mapped_ ? len_ : data_.size(); }
        const char* data() const { return mapped_ ? mapped_ : data_.data(); }
        bool valid() const { return mapped_ != nullptr || !data_.empty(); }

     private:
        int fd = -1;
        size_t len_ = 0;
        char* mapped_ = nullptr;
        std::string data_;

    }; // class read_access

    inline read_access read(std::string_view aFilename) { return read_access{aFilename}; }
    std::string read_from_file_descriptor(int fd, size_t chunk_size = 1024);
    inline std::string read_stdin() { return read_from_file_descriptor(0); }
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
