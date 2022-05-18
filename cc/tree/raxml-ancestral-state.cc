#include <unordered_map>

#include "utils/file.hh"
#include "utils/string.hh"
#include "tree/tree.hh"

// ======================================================================

class RaxmlAncestralState
{
  public:
    RaxmlAncestralState(const std::filesystem::path& filename) : data_{ae::file::read(filename)}
        {
            for (const auto line : ae::string::split_into_lines(data_)) {
                const auto name_seq = ae::string::split(line, "\t");
                name_to_seq_.emplace(name_seq[0], name_seq[1]);
            }
        }

  private:
    const std::string data_;
    std::unordered_map<std::string_view, std::string_view> name_to_seq_;

};

// ======================================================================

void ae::tree::Tree::set_raxml_ancestral_state_reconstruction_data(const std::filesystem::path& raxml_tree_file, const std::filesystem::path& raxml_states_file)
{
    // AD_DEBUG("\"{}\" \"{}\"", raxml_tree_file, raxml_states_file);
    auto raxml_tree = load(raxml_tree_file);
    RaxmlAncestralState state{raxml_states_file};


} // ae::tree::Tree::set_raxml_ancestral_state_reconstruction_data

// ----------------------------------------------------------------------
