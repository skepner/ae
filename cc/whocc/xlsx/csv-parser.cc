#include <stack>

#include "utils/file.hh"
#include "utils/log.hh"
#include "whocc/xlsx/csv-parser.hh"

enum class state {
    cell,
    quoted,
    escaped
};

constexpr const char separator{','};
constexpr const char quote{'"'};
constexpr const char escape{'\\'};

// ----------------------------------------------------------------------

ae::xlsx::v1::csv::Sheet::Sheet(const std::filesystem::path& filename)
{
    const std::string src{ae::file::read(filename)};

    const auto convert_cell = [&]() {};

    const auto new_cell = [&]() {
        convert_cell();
        data_.back().emplace_back(std::string{});
    };

    const auto new_row = [&]() {
        convert_cell();
        number_of_columns_ = std::max(number_of_columns_, ae::xlsx::ncol_t{data_.back().size()});
        data_.emplace_back().emplace_back(std::string{});
    };

    const auto append = [&](char sym) {
        std::visit(
            [sym]<typename Content>(Content& content) {
                if constexpr (std::is_same_v<Content, std::string>)
                    content.push_back(sym);
            },
            data_.back().back());
    };

    std::stack<enum state> states;
    states.push(state::cell);
    data_.emplace_back().emplace_back(std::string{});
    for (const char sym : src) {
        if (states.top() == state::escaped) {
            states.pop();
            append(sym);
        }
        else {
            switch (sym) {
                case separator:
                    if (states.top() == state::quoted)
                        append(sym);
                    else
                        new_cell();
                    break;
                case '\n':
                    if (states.top() == state::quoted)
                        append(sym);
                    else
                        new_row();
                    break;
                case quote:
                    if (states.top() == state::quoted)
                        states.pop();
                    else
                        states.push(state::quoted);
                    break;
                case escape:
                    states.push(state::escaped);
                    break;
                default:
                    append(sym);
                    break;
            }
        }
    }

    if (!data_.empty() && data_.back().size() <= 1 && number_of_columns_ > xlsx::ncol_t{1})
        data_.erase(std::prev(data_.end()));

    // normalize number of columns
    for (auto& row : data_) {
        while (row.size() < *number_of_columns_)
            row.emplace_back(std::string{});
    }

    AD_INFO("csv: rows: {} cols: {}", number_of_rows(), number_of_columns());
    // for (const auto& row : data_) {
    //     bool first{true};
    //     for (const auto& cell : row) {
    //         if (first)
    //             first = false;
    //         else
    //             fmt::print("|");
    //         fmt::print("{}", cell);
    //     }
    //     fmt::print("\n");
    // }
}

// ----------------------------------------------------------------------
