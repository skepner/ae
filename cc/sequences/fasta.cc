#include "utils/file.hh"
#include "utils/string.hh"
#include "sequences/fasta.hh"

// ======================================================================

ae::sequences::fasta::Reader::iterator& ae::sequences::fasta::Reader::iterator::operator++()
{
    using namespace std::string_view_literals;

    while (next_ != data_.end() && *next_ == '\n') {
        ++next_;
        ++line_no_;
    }

    value_ = value_t{.filename = filename_};
    if (next_ == data_.end())
        return *this;

    if (*next_ == '>') {
        ++next_;

        // read name
        const auto name_start = next_;
        next_ = std::find(next_, data_.end(), '\n');
        if (next_ == data_.end())
            throw std::runtime_error{fmt::format("invalid format at {}", line_no_)};
        std::string_view raw_name(name_start, static_cast<size_t>(next_ - name_start));
        if (raw_name.back() == '\r')
            raw_name.remove_suffix(1);
        value_.sequence = std::make_shared<RawSequence>(raw_name);
        value_.line_no = line_no_;
        ++next_;
        ++line_no_;
        if (next_ == data_.end())
            throw std::runtime_error{fmt::format("invalid format at {}", line_no_)};

        // read sequence
        for (size_t seq_line_no = 0; next_ != data_.end() && *next_ != '>'; ++seq_line_no) {
            const auto line_start = next_;
            next_ = std::find(next_, data_.end(), '\n');
            if (next_ != data_.end()) {
                const auto line_end = *std::prev(next_) == '\r' ? std::prev(next_) : next_;
                const auto line_size = static_cast<size_t>(line_end - line_start);
                if (seq_line_no == 0 && std::string_view{line_start, line_size}.find_first_of("/|_"sv) != std::string_view::npos) // gisaid sometimes have name split into two lines
                    value_.sequence->raw_name = std::string_view{name_start, static_cast<size_t>(line_end - name_start)};
                else
                    value_.sequence->raw_sequence.append(line_start, line_end);
                ++next_;
                ++line_no_;
            }
        }
        if (value_.sequence->raw_sequence.empty())
            throw std::runtime_error{fmt::format("invalid format at {}", line_no_)};
        ae::string::uppercase_in_place(value_.sequence->raw_sequence);
    }
    else
        throw std::runtime_error{fmt::format("invalid format at {}", line_no_)};

    return *this;

} // ae::sequences::fasta::Reader::iterator::operator++

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
