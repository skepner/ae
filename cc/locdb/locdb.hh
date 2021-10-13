#include <string_view>

// ----------------------------------------------------------------------

namespace ae::locationdb::inline v1
{
    class Db
    {
      private:
        Db(std::string_view path);

        friend const Db& get();
    };

    const Db& get();
}

// ----------------------------------------------------------------------
