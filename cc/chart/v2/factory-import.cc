#include "utils/file.hh"
#include "chart/v2/factory-import.hh"
#include "chart/v2/ace-import.hh"
#include "chart/v2/acd1-import.hh"
#include "chart/v2/lispmds-import.hh"

// ----------------------------------------------------------------------

ae::chart::v2::ChartP ae::chart::v2::import_from_decompressed_data(std::string aData, Verify aVerify)
{
    const std::string_view data_view(aData);
    if (is_ace(data_view))
        return ace_import(data_view, aVerify);
    if (is_acd1(data_view))
        return acd1_import(data_view, aVerify);
    if (is_lispmds(data_view))
        return lispmds_import(data_view, aVerify);
    throw import_error{"[ae::chart::v2::import_from_data]: unrecognized file content"};

} // ae::chart::v2::import_from_decompressed_data

// ----------------------------------------------------------------------

ae::chart::v2::ChartP ae::chart::v2::import_from_data(std::string_view aData, Verify aVerify)
{
    return import_from_decompressed_data(ae::file::decompress_if_necessary(aData), aVerify);

} // ae::chart::v2::import_from_data

// ----------------------------------------------------------------------

ae::chart::v2::ChartP ae::chart::v2::import_from_data(std::string aData, Verify aVerify)
{
    return import_from_decompressed_data(ae::file::decompress_if_necessary(aData), aVerify);

} // ae::chart::v2::import_from_data

// ----------------------------------------------------------------------

ae::chart::v2::ChartP ae::chart::v2::import_from_file(std::string aFilename, Verify aVerify)
{
    try {
        return import_from_decompressed_data(ae::file::read(aFilename), aVerify);
    }
    catch (ae::file::not_found&) {
        throw import_error{fmt::format("[ae::chart::v2::import_from_file]: file not found: \"{}\"", aFilename)};
    }
    catch (std::exception& err) {
        throw import_error{fmt::format("[ae::chart::v2::import_from_file]: unrecognized file content: \"{}\": {}", aFilename, err.what())};
    }

} // ae::chart::v2::import_from_file

// ----------------------------------------------------------------------
