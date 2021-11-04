#include "utils/file.hh"
#include "sequences/fasta.hh"

// ======================================================================

ae::sequences::fasta::Reader::iterator& ae::sequences::fasta::Reader::iterator::operator++()
{
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
        while (next_ != data_.end() && *next_ != '>') {
            const auto line_start = next_;
            next_ = std::find(next_, data_.end(), '\n');
            if (next_ != data_.end()) {
                value_.sequence->sequence.append(line_start, *std::prev(next_) == '\r' ? std::prev(next_) : next_);
                ++next_;
                ++line_no_;
            }
        }
        if (value_.sequence->sequence.empty())
            throw std::runtime_error{fmt::format("invalid format at {}", line_no_)};
    }
    else
        throw std::runtime_error{fmt::format("invalid format at {}", line_no_)};

    return *this;

} // ae::sequences::fasta::Reader::iterator::operator++

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
