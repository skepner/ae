#pragma once

#include "ext/simdjson.hh"

// ----------------------------------------------------------------------

namespace ae
{
    namespace dynamic
    {
        class value;
    }

    void load(dynamic::value& collection, simdjson::ondemand::value source);

}

// ----------------------------------------------------------------------
