#pragma once

#include <vector>

#include "ext/fmt.hh"
#include "ext/range-v3.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    namespace detail
    {
        template <typename Key, typename Value> class map_base_t
        {
          public:
            using entry_type = std::pair<Key, Value>;
            using const_iterator = typename std::vector<entry_type>::const_iterator;

            map_base_t() = default;
            map_base_t(const map_base_t&) = default;
            map_base_t(map_base_t&&) = default;
            virtual ~map_base_t() = default;
            map_base_t& operator=(map_base_t&&) = default;
            map_base_t& operator=(const map_base_t&) = default;

            bool empty() const noexcept { return data_.empty(); }
            constexpr const auto& data() const noexcept { return data_; }
            constexpr auto begin() const { return data_.begin(); }
            constexpr auto end() const { return data_.end(); }

            template <typename Range> void collect(Range&& rng, bool check_result = true)
            {
                data_ = rng | ranges::to<std::vector>;
                sorted_ = false;
                if (check_result && data_.size() > 1)
                    check();
            }

            template <typename EKey, typename ... V> auto& emplace(EKey&& key, V&& ... value)
            {
                sorted_ = false;
                return data_.emplace_back(std::forward<EKey>(key), Value{std::forward<V>(value) ...});
            }

            // public to allow forcing sorting in the mutli-threaded app (seqdb3 -> acmacs-api)
            void sort() const noexcept
            {
                if (!sorted_) {
                    ranges::sort(data_, [](const auto& e1, const auto& e2) { return e1.first < e2.first; });
                    sorted_ = true;
                }
            }

            virtual void check() const {}

          protected:
            constexpr auto& data() noexcept { return data_; }
            // constexpr bool sorted() const noexcept { return sorted_; }

            template <typename FindKey> const_iterator find_first(const FindKey& key) const noexcept
            {
                sort();
                return std::lower_bound(std::begin(data_), std::end(data_), key, [](const auto& e1, const auto& k2) { return e1.first < k2; });
            }

          private:
            mutable std::vector<entry_type> data_;
            mutable bool sorted_{false};

        }; // class map_base_t
    } // namespace detail

    // ----------------------------------------------------------------------

    // see seqdb.cc Seqdb::hash_index() for sample usage
    template <typename Key, typename Value> class map_with_duplicating_keys_t : public detail::map_base_t<Key, Value>
    {
      public:
        using const_iterator = typename detail::map_base_t<Key, Value>::const_iterator;

        template <typename FindKey> std::pair<const_iterator, const_iterator> find(const FindKey& key) const noexcept
        {
            const auto first = this->find_first(key);
            // first may point to the wrong key, if key is not in the map
            return {first, std::find_if(first, std::end(this->data()), [&key](const auto& en) { return en.first != key; })};
        }

    };

    // ----------------------------------------------------------------------

    class map_with_unique_keys_error : public std::runtime_error
    {
      public:
        using std::runtime_error::runtime_error;
    };

    // see seqdb.cc Seqdb::seq_id_index() for sample usage
    template <typename Key, typename Value> class map_with_unique_keys_t : public detail::map_base_t<Key, Value>
    {
      public:
        using const_iterator = typename detail::map_base_t<Key, Value>::const_iterator;

        template <typename FindKey> const Value* find(const FindKey& key) const noexcept
        {
            if (const auto first = this->find_first(key); first->first == key)
                return &first->second;
            else
                return nullptr;
        }

        void check() const override
        {
            this->sort();
            for (auto cur = std::next(std::begin(this->data())); cur != std::end(this->data()); ++cur) {
                if (cur->first == std::prev(cur)->first)
                    throw map_with_unique_keys_error{"duplicating keys within map_with_unique_keys_t"};
            }
        }

    };

    // ----------------------------------------------------------------------

    template <typename Key, typename Value> class small_map_with_unique_keys_t
    {
      public:
        using entry_type = std::pair<Key, Value>;
        using iterator = typename std::vector<entry_type>::iterator;
        using const_iterator = typename std::vector<entry_type>::const_iterator;

        small_map_with_unique_keys_t() = default;
        // template <typename Iter> small_map_with_unique_keys_t(Iter first, Iter last) : data_(first, last) {}
        small_map_with_unique_keys_t(std::initializer_list<entry_type> init) : data_{init} {}

        constexpr const auto& data() const noexcept { return data_; }
        auto begin() const noexcept { return data_.begin(); }
        auto end() const noexcept { return data_.end(); }
        auto begin() noexcept { return data_.begin(); }
        auto end() noexcept { return data_.end(); }
        auto empty() const noexcept { return data_.empty(); }
        auto size() const noexcept { return data_.size(); }

        template <typename K> const_iterator find(const K& key) const noexcept
        {
            return std::find_if(std::begin(data_), std::end(data_), [&key](const auto& en) { return en.first == key; });
        }

        template <typename K> iterator find(const K& key) noexcept
        {
            return std::find_if(std::begin(data_), std::end(data_), [&key](const auto& en) { return en.first == key; });
        }

        template <typename K, typename Callback> void find_then(const K& key, Callback callback) const noexcept
        {
            if (const auto& en = find(key); en != std::end(data_))
                callback(en->second);
        }

        template <typename K, typename CallbackThen, typename CallbackElse> void find_then_else(const K& key, CallbackThen callback_then, CallbackElse callback_else) noexcept
        {
            if (const auto& en = find(key); en != std::end(data_))
                callback_then(en->second);
            else
                callback_else(*this);
        }

        template <typename K> Value& get(const K& key)
        {
            if (const auto found = find(key); found != std::end(data_))
                return found->second;
            throw std::out_of_range{fmt::format("acmacs::small_map_with_unique_keys_t::at(): no key: {}", key)};
        }

        template <typename K> const Value& get(const K& key) const
        {
            if (const auto found = find(key); found != std::end(data_))
                return found->second;
            throw std::out_of_range{fmt::format("acmacs::small_map_with_unique_keys_t::at(): no key: {}", key)};
        }

        template <typename K, typename V> const Value& get_or(const K& key, const V& dflt) const
        {
            if (const auto found = find(key); found != std::end(data_))
                return found->second;
            else
                return dflt;
        }

        template <typename K, typename ... V> auto& emplace_or_replace(const K& key, V&& ... value)
        {
            if (auto found = find(key); found != end()) {
                found->second = Value{std::forward<V>(value) ...};
                return *found;
            }
            else
                return data_.emplace_back(Key{key}, Value{std::forward<V>(value) ...});
        }

        template <typename K, typename ... V> auto& emplace_not_replace(const K& key, V&& ... value)
        {
            if (auto found = find(key); found != end())
                return *found;
            else
                return data_.emplace_back(Key{key}, Value{std::forward<V>(value) ...});
        }

        template <typename Order> void sort(Order&& order)
        {
            std::sort(std::begin(data_), std::end(data_), std::forward<Order>(order));
        }

      private:
        std::vector<entry_type> data_{};

    }; // small_map_with_unique_keys_t<Key, Value>

} // namespace acmacs

// ----------------------------------------------------------------------

namespace ae::fmt_helper
{
    template <typename Map> struct map_formatter : fmt::formatter<default_formatter>
    {
        template <typename FormatCtx> auto format(const Map& map, FormatCtx& ctx)
        {
            fmt::format_to(ctx.out(), "{{");
            bool first{true};
            for (const auto& [key, value] : map) {
                if (!first)
                    fmt::format_to(ctx.out(), ", ");
                else
                    first = false;
                using key_t = std::decay_t<decltype(key)>;
                if constexpr (std::is_same_v<key_t, std::string_view> || std::is_same_v<key_t, std::string>)
                    fmt::format_to(ctx.out(), "\"{}\": {}", key, value);
                else
                    fmt::format_to(ctx.out(), "{}: {}", key, value);
            }
            return fmt::format_to(ctx.out(), "}}");
        }
    };
}

// template <> struct fmt::formatter<acmacs::detail::map_base_t<std::string, std::string>> : acmacs::fmt_helper::map_formatter<acmacs::detail::map_base_t<std::string, std::string>> {};

// ----------------------------------------------------------------------
