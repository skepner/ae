#include <array>

#include "utils/named-vector.hh"

// ----------------------------------------------------------------------

int main(int /*argc*/, const char* const* /*argv*/)
{
    using VS = ae::named_vector_t<std::string, struct VS_tag>;

    try {
        VS vs;
        vs.insert("aaa");
        vs.insert_if_not_present("aaa");
        vs.insert_if_not_present(std::string_view{"aaa"});
        vs.insert_if_not_present(std::string_view{"bbb"});
        vs.insert_if_not_present(std::string_view{"ccc"});
        fmt::print(">>>> {}\n", vs);
        vs.remove("aaa");
        fmt::print(">>>> {}\n", vs);
        vs.remove(std::string_view{"aaa"});
        vs.remove(std::string_view{"bbb"});
        fmt::print(">>>> {}\n", vs);
    }
    catch (std::exception& err) {
        fmt::print("> {}\n", err.what());
    }
    return 0;
}

// ----------------------------------------------------------------------
