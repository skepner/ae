#include <string_view>

// ----------------------------------------------------------------------

namespace ae::locationdb::inline v1
{
    class Db
    {
      private:
        Db(const std::string& path);

        friend const Db& get();
    };

    const Db& get();
}

// ----------------------------------------------------------------------
