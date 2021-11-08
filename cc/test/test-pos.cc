#include "ext/fmt.hh"
#include "sequences/pos.hh"

// ======================================================================

int main(int /*argc*/, const char* const* /*argv*/)
{
    try {
        using namespace ae::sequences;

        const size_t raw{161};
        const pos1_t p1{raw};
        const pos0_t p0{p1};
        fmt::print("raw:{} pos1(raw):{} pos0(pos1):{}\n", raw, p1, p0);
    }
    catch (std::exception& err) {
        fmt::print("> {}\n", err.what());
        return 1967;
    }
}

// ======================================================================
