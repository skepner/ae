#include "utils/collection.hh"
#include "utils/collection-json.hh"

// ----------------------------------------------------------------------

namespace ae
{
    class DynamicCollectionJsonLoader
    {
      public:
        static void load(DynamicCollection& collection, simdjson::ondemand::value source) { load(collection.data_, source); }

        static void load(ae::dynamic::value& target, simdjson::ondemand::value source)
        {
            switch (source.type()) {
                case simdjson::ondemand::json_type::array:
                    break;
                case simdjson::ondemand::json_type::object:
                    break;
                case simdjson::ondemand::json_type::number:
                    if (source.is_integer())
                        target = source.get_int64();
                    else
                        target = source.get_double();
                    break;
                case simdjson::ondemand::json_type::string:
                    target = std::string{static_cast<std::string_view>(source.get_string())};
                    break;
                case simdjson::ondemand::json_type::boolean:
                    target = source.get_bool();
                    break;
                case simdjson::ondemand::json_type::null:
                    target = ae::dynamic::null{};
                    break;
            }
        }
    };

} // namespace ae

// ----------------------------------------------------------------------

void ae::load(DynamicCollection& collection, simdjson::ondemand::value source)
{
    DynamicCollectionJsonLoader::load(collection, source);

} // ae::load

// ----------------------------------------------------------------------
