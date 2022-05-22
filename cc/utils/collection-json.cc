#include "utils/collection.hh"
#include "utils/collection-json.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

ae::dynamic::value_null ae::dynamic::static_null{};

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

namespace ae::dynamic
{
    class DynamicCollectionJsonLoader
    {
      public:
        static value load(simdjson::ondemand::value source)
        {
            switch (source.type()) {
                case simdjson::ondemand::json_type::array: {
                    array target{};
                    for (simdjson::ondemand::value elt : source.get_array())
                        target.add(load(elt));
                    return target;
                }
                case simdjson::ondemand::json_type::object: {
                    object target{};
                    for (auto field : source.get_object())
                        target.add(static_cast<std::string_view>(field.unescaped_key()), load(field.value()));
                    return target;
                }
                case simdjson::ondemand::json_type::number:
                    if (source.is_integer())
                        return source.get_int64();
                    else
                        return source.get_double();
                case simdjson::ondemand::json_type::string:
                    return static_cast<std::string_view>(source.get_string());
                case simdjson::ondemand::json_type::boolean:
                    return source.get_bool();
                case simdjson::ondemand::json_type::null:
                    return null{};
            }
            return null{};
        }
    };

} // namespace ae

// ----------------------------------------------------------------------

void ae::load(dynamic::value& collection, simdjson::ondemand::value source)
{
    collection = dynamic::DynamicCollectionJsonLoader::load(source);

} // ae::load

// ----------------------------------------------------------------------
