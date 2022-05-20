#pragma once

#include "ext/simdjson.hh"

// ----------------------------------------------------------------------

namespace ae
{
    class DynamicCollection;

    void load(DynamicCollection& collection, simdjson::ondemand::value source);

}

// ----------------------------------------------------------------------
