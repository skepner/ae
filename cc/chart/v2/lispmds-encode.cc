#include <cctype>
#include <regex>

#include "utils/string.hh"
#include "chart/v2/lispmds-encode.hh"
#include "ext/from_chars.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

static const std::string sEncodedSignature_strain_name_lc{"!b"};
static const std::string sEncodedSignature_strain_name_uc{"!B"};
static const std::string sEncodedSignature_table_name_lc{"@b"};
static const std::string sEncodedSignature_table_name_uc{"@B"};

static const std::string sEncodedSignature_strain_name_lc_v1{"/a"};
static const std::string sEncodedSignature_strain_name_uc_v1{"/A"};
static const std::string sEncodedSignature_table_name_lc_v1{"@a"};
static const std::string sEncodedSignature_table_name_uc_v1{"@A"};

#pragma GCC diagnostic pop

inline bool strain_name_encoded(std::string_view name)
{
    auto eqc = [](std::string_view tail) { return tail == sEncodedSignature_strain_name_lc || tail == sEncodedSignature_strain_name_uc || tail == sEncodedSignature_strain_name_lc_v1 || tail == sEncodedSignature_strain_name_uc_v1; };
    return name.size() > 2 && eqc(std::string_view(name.data() + name.size() - 2, 2));
}

inline bool table_name_encoded(std::string_view name)
{
    auto eqc = [](std::string_view tail) { return tail == sEncodedSignature_table_name_lc || tail == sEncodedSignature_table_name_uc || tail == sEncodedSignature_table_name_lc_v1 || tail == sEncodedSignature_table_name_uc_v1; };
    return name.size() > 2 && eqc(std::string_view(name.data() + name.size() - 2, 2));
}

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

static std::regex sFluASubtype{"AH([0-9]+N[0-9]+).*"};
static std::regex sPassageDate{".+ ([12][90][0-9][0-9]-[0-2][0-9]-[0-3][0-9])"};

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

static inline std::string append_signature(const std::string& aSource, ae::chart::v2::lispmds_encoding_signature signature)
{
    using namespace ae::chart::v2;

    switch (signature) {
      case lispmds_encoding_signature::no:
          return aSource;
      case lispmds_encoding_signature::strain_name:
          return aSource + sEncodedSignature_strain_name_lc;
      case lispmds_encoding_signature::table_name:
          return aSource + sEncodedSignature_table_name_lc;
    }
    return aSource;
}

// ----------------------------------------------------------------------

std::string ae::chart::v2::lispmds_encode(std::string_view aName, lispmds_encoding_signature signature)
{
    std::string result;
    bool encoded = false;
    for (auto c : aName) {
        switch (c) {
            case ' ':
                result.append(1, '_');
                encoded = true;
                break;
            case '(':
            case ')':
                encoded = true;
                break;
            case ',':
                result.append("..");
                encoded = true;
                break;
            case '%': // we use it as a hex code prefix
            case ':': // : is a symbol module separator in lisp
            case '$': // tk tries to subst var when sees $
            case '?': // The "?" in strain names causes an issue with strain matching. (Blake 2018-06-11)
            case '"':
            case '\'':
            case '`':
            case ';':
            case '[':
            case ']':
            case '{':
            case '}':
            case '#': // special character in lisp that get expanded before the usual readers sees them
            case '|': // special character in lisp that get expanded before the usual readers sees them
                result.append(fmt::format("%{:02X}", c));
                encoded = true;
                break;
            case '/':
            case '~': // perhaps avoid in the table name
                if (signature == lispmds_encoding_signature::table_name) {
                    result.append(fmt::format("%{:02X}", c));
                    encoded = true;
                }
                else {
                    result.append(1, c);
                }
                break;
            case '+': // approved by Derek on 2018-06-11
            case '-':
            case '*':
            case '@':
            case '^':
            case '&':
            case '_':
            case '=':
            case '<':
            case '>':
            case '.':
            case '!':
            default:
                result.append(1, c);
                break;
        }
    }
    return encoded ? append_signature(result, signature) : result;

} // ae::chart::v2::lispmds_encode

// ----------------------------------------------------------------------

std::string ae::chart::v2::lispmds_table_name_encode(std::string_view name)
{
      // lispmds does not like / in the table name
      // it interprets / as being part of a file name when we doing procrustes (Blake 2018-06-11)
    const auto encoded = lispmds_encode(name, lispmds_encoding_signature::no);
    if (encoded == name)
        return encoded;          // nothing encoded, no need in signature
    else
        return append_signature(encoded, lispmds_encoding_signature::table_name);

} // ae::chart::v2::lispmds_table_name_encode

// ----------------------------------------------------------------------

std::string ae::chart::v2::lispmds_antigen_name_encode(const ae::virus::Name& aName, const ae::virus::Reassortant& aReassortant, const ae::virus::Passage& aPassage, const Annotations& aAnnotations, lispmds_encoding_signature signature)
{
    std::string result = lispmds_encode(aName, lispmds_encoding_signature::no);
    if (!aReassortant.empty())
        result += "_r!" + lispmds_encode(aReassortant, lispmds_encoding_signature::no);
    if (!aPassage.empty())
        result += "_p!" + lispmds_encode(aPassage.to_string(), lispmds_encoding_signature::no);
    for (const auto& anno: aAnnotations)
        result += "_a!" + lispmds_encode(anno, lispmds_encoding_signature::no);
    if (result == *aName)
        return result;          // nothing encoded, no need in signature
    else
        return append_signature(result, signature);

} // ae::chart::v2::lispmds_antigen_name_encode

// ----------------------------------------------------------------------

std::string ae::chart::v2::lispmds_serum_name_encode(const ae::virus::Name& aName, const ae::virus::Reassortant& aReassortant, const Annotations& aAnnotations, const SerumId& aSerumId, lispmds_encoding_signature signature)
{
    std::string result = lispmds_encode(aName, lispmds_encoding_signature::no);
    if (!aReassortant.empty())
        result += "_r!" + lispmds_encode(aReassortant, lispmds_encoding_signature::no);
    for (const auto& anno: aAnnotations)
        result += "_a!" + lispmds_encode(anno, lispmds_encoding_signature::no);
    if (!aSerumId.empty())
        result += "_i!" + lispmds_encode(aSerumId, lispmds_encoding_signature::no);
    if (result == *aName)
        return result;          // nothing encoded, no need in signature
    else
        return append_signature(result, signature);

} // ae::chart::v2::lispmds_serum_name_encode

// ----------------------------------------------------------------------

std::string ae::chart::v2::lispmds_decode(std::string_view aName)
{
    if (strain_name_encoded(aName) || table_name_encoded(aName)) {
        std::string result;
        bool last_was_space = true;
        for (size_t pos = 0; pos < (aName.size() - 2); ++pos) {
            switch (aName[pos]) {
              case '.':
                  if (aName[pos + 1] == '.') {
                      result.append(1, ',');
                      ++pos;
                  }
                  else
                      result.append(1, aName[pos]);
                  break;
              case '_':
                  result.append(1, ' ');
                  break;
              case '%':
                  if (std::isxdigit(aName[pos + 1]) && std::isxdigit(aName[pos + 2])) {
                      result.append(1, static_cast<char>(ae::from_chars<size_t>(aName.substr(pos + 1, 2), 16)));
                      pos += 2;
                  }
                  else
                      result.append(1, aName[pos]);
                  break;
              case 'A':
                  if (last_was_space && aName[pos + 1] == 'H') {
                      std::smatch flu_a_match;
                      if (const std::string text(std::string{aName}, pos); std::regex_match(text, flu_a_match, sFluASubtype)) {
                          result.append("A(H").append(flu_a_match.str(1)).append(1, ')');
                          pos += static_cast<size_t>(flu_a_match.length(1)) + 1;
                      }
                      else
                          result.append(1, aName[pos]);
                  }
                  else
                      result.append(1, aName[pos]);
                  break;
              default:
                  result.append(1, aName[pos]);
                  break;
            }
            last_was_space = result.back() == ' ';
        }
        return result;
    }
    else
        return std::string{aName};

} // ae::chart::v2::lispmds_decode

// ----------------------------------------------------------------------

static inline std::vector<size_t> find_sep(std::string_view source, bool v1)
{
    std::vector<size_t> sep_pos;
    for (size_t pos = 0; pos < (source.size() - 2); ++pos) {
        if (source[pos] == ' ' && (v1 || source[pos+2] == '!')) {
            switch (std::tolower(source[pos + 1])) {
              case 'r':
              case 'p':
              case 'a':
              case 'i':
                  sep_pos.push_back(pos);
                  ++pos;
                  break;
            }
        }
    }
    return sep_pos;
}

// ----------------------------------------------------------------------

static inline std::string fix_passage_date(std::string source)
{
    if (std::smatch passage_date; std::regex_match(source, passage_date, sPassageDate)) {
        source.insert(static_cast<size_t>(passage_date.position(1)), 1, '(');
        source.append(1, ')');
    }
    return source;
}

void ae::chart::v2::lispmds_antigen_name_decode(std::string_view aSource, ae::virus::Name& aName, ae::virus::Reassortant& aReassortant, ae::virus::Passage& aPassage, Annotations& aAnnotations)
{
    if (strain_name_encoded(aSource)) {
        const bool v1 = std::tolower(aSource.back()) == 'a';
        const size_t sep_len = v1 ? 2 : 3;
        const std::string stage1 = lispmds_decode(aSource);
        auto sep_pos = find_sep(stage1, v1);
        if (!sep_pos.empty()) {
            aName = ae::virus::Name{stage1.substr(0, sep_pos[0])};
            for (size_t sep_no = 0; sep_no < sep_pos.size(); ++sep_no) {
                const size_t chunk_len = (sep_no + 1) < sep_pos.size() ? (sep_pos[sep_no + 1] - sep_pos[sep_no] - sep_len) : std::string::npos;
                switch (std::tolower(stage1[sep_pos[sep_no] + 1])) {
                  case 'r':
                      aReassortant = ae::virus::Reassortant{stage1.substr(sep_pos[sep_no] + sep_len, chunk_len)};
                      break;
                  case 'p':
                      aPassage = ae::virus::Passage{fix_passage_date(stage1.substr(sep_pos[sep_no] + sep_len, chunk_len))};
                      break;
                  case 'a':
                      aAnnotations.push_back(stage1.substr(sep_pos[sep_no] + sep_len, chunk_len));
                      break;
                }
            }
        }
        else
            aName = ae::virus::Name{stage1};
    }
    else
        aName = ae::virus::Name{aSource};

} // ae::chart::v2::lispmds_antigen_name_decode

// ----------------------------------------------------------------------

void ae::chart::v2::lispmds_serum_name_decode(std::string_view aSource, ae::virus::Name& aName, ae::virus::Reassortant& aReassortant, Annotations& aAnnotations, SerumId& aSerumId)
{
    if (strain_name_encoded(aSource)) {
        const bool v1 = std::tolower(aSource.back()) == 'a';
        const size_t sep_len = v1 ? 2 : 3;
        const std::string stage1 = lispmds_decode(aSource);
        auto sep_pos = find_sep(stage1, v1);
        if (!sep_pos.empty()) {
            aName = ae::virus::Name{stage1.substr(0, sep_pos[0])};
            for (size_t sep_no = 0; sep_no < sep_pos.size(); ++sep_no) {
                const size_t chunk_len = (sep_no + 1) < sep_pos.size() ? (sep_pos[sep_no + 1] - sep_pos[sep_no] - sep_len) : std::string::npos;
                switch (std::tolower(stage1[sep_pos[sep_no] + 1])) {
                  case 'r':
                      aReassortant = ae::virus::Reassortant{stage1.substr(sep_pos[sep_no] + sep_len, chunk_len)};
                      break;
                  case 'i':
                      aSerumId = SerumId{stage1.substr(sep_pos[sep_no] + sep_len, chunk_len)};
                      break;
                  case 'a':
                      aAnnotations.push_back(stage1.substr(sep_pos[sep_no] + sep_len, chunk_len));
                      break;
                }
            }
        }
        else
            aName = ae::virus::Name{stage1};
    }
    else
        aName = ae::virus::Name{aSource};

} // ae::chart::v2::lispmds_serum_name_decode

// ----------------------------------------------------------------------
