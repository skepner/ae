#pragma once

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdocumentation-unknown-command"
#pragma GCC diagnostic ignored "-Wdocumentation"
#pragma GCC diagnostic ignored "-Wreserved-identifier"
// #pragma GCC diagnostic ignored "-Wsuggest-destructor-override"
#pragma GCC diagnostic ignored "-Wpadded"
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wnewline-eof"
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wextra-semi-stmt"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#pragma GCC diagnostic ignored "-Wambiguous-reversed-operator"
#pragma GCC diagnostic ignored "-Wweak-vtables"
#pragma GCC diagnostic ignored "-Wunused-template"
#pragma GCC diagnostic ignored "-Wunused-member-function"
// #pragma GCC diagnostic ignored ""
// #pragma GCC diagnostic ignored ""
#endif

#include "simdjson.h"

#pragma GCC diagnostic pop

#include "utils/file.hh"

// ----------------------------------------------------------------------

namespace ae::simdjson
{
    using namespace ::simdjson;

    class Parser
    {
      public:
        Parser(const std::filesystem::path& filename)                    //
            : parser_{},                                                 //
              json_{file::read(filename, ::simdjson::SIMDJSON_PADDING)}, //
              doc_{parser_.iterate(json_, json_.size() + ::simdjson::SIMDJSON_PADDING)}
        {
        }

        constexpr auto& doc() { return doc_; }

        size_t current_location_offset() { return doc_.current_location().value() - json_.data(); }
        std::string_view current_location_snippet(size_t size) { return std::string_view(doc_.current_location().value(), size); }

      private:
        ::simdjson::ondemand::parser parser_;
        std::string json_;
        decltype(parser_.iterate(json_, json_.capacity())) doc_;
    };
} // namespace ae::simdjson

// ----------------------------------------------------------------------
