#pragma once

#include <stdexcept>

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    enum class Verify { None, All };

    class import_error : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    class export_error : public std::runtime_error { public: using std::runtime_error::runtime_error; };

} // namespace ae::chart::v2

// ----------------------------------------------------------------------
