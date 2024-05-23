#pragma once
#include "exception.hpp"

#include <variant>
#include <string_view>
#include <ctime>
#include <charconv>
#include <iomanip>

namespace fb
{

namespace detail
{
    // Reflection
    template <class T>
    inline constexpr std::string_view type_name()
    {
        std::string_view sv = __PRETTY_FUNCTION__;
        sv.remove_prefix(sv.find('=') + 2);
        return { sv.data(), sv.find(';') };
    }

    // Helper type for the visitor
    template <class... Ts>
    struct overloaded : Ts... { using Ts::operator()...; };
    // Explicit deduction guide (not needed as of C++20)
    template <class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

} // namespace detail

// Tag to skip parameter
struct skip_t { };
inline constexpr skip_t skip;

// Methods to convert timestamp
struct timestamp_t : ISC_TIMESTAMP
{
    // Convert timestamp_t to time_t.
    // Note, this will cut all dates below 1970-01-01, use to_tm() instead.
    // 40587 is GDS epoch start in days (since 1858-11-17, time_t starts at 1970).
    time_t to_time_t() const
    { return std::max(int(timestamp_date) - 40587, 0) * 86400 + timestamp_time / 10'000; }

    std::tm* to_tm(std::tm* t) const {
        isc_decode_timestamp(const_cast<timestamp_t*>(this), t);
        return t;
    }

    static timestamp_t from_time_t(time_t t) {
        timestamp_t ret;
        ret.timestamp_date = t / 86400 + 40587;
        ret.timestamp_time = (t % 86400) * 10'000;
        return ret;
    }

    static timestamp_t from_tm(std::tm* t) {
        timestamp_t ret;
        isc_encode_timestamp(t, &ret);
        return ret;
    }

    static timestamp_t now()
    { return from_time_t(time(0)); }

    size_t ms() const
    { return (timestamp_time / 10) % 1000; }
};

template <class T>
struct scaled_integer
{
    T _value;
    short _scale;

    explicit scaled_integer(T value, short scale = 0) noexcept
    : _value(value)
    , _scale(scale)
    { }

    template <class U = T>
    U get() const noexcept
    {
        U val = _value;
    // TODO check overflow
        for (auto x = 0; x < _scale; ++x)
            val *= 10;
        for (auto x = _scale; x < 0; ++x)
            val /= 10;
        return val;
    }

    // TODO add to_string()
};

#if 0
template <class T>
struct is_scaled_integer : std::false_type
{ };

template <class T>
struct is_scaled_integer<scaled_integer<T>> : std::true_type
{ };

template <class T>
inline constexpr bool is_scaled_integer_v = is_scaled_integer<T>::value;
#endif


using blob_id_t = ISC_QUAD;

using field_t = std::variant<
    std::nullptr_t,
    std::string_view,
    scaled_integer<int16_t>,
    scaled_integer<int32_t>,
    scaled_integer<int64_t>,
    float,
    double,
    timestamp_t,
    blob_id_t
>;


// Visitors with type conversion.
// Where T is requested type and argument to call operator
// is field_t value.
// Note, these visitors called only for non-null values, e.g.
// no need to check for nullptr_t.

template <class T, class = void>
struct type_converter;

// T is integral type or floating point
template <class T>
struct type_converter<T, std::enable_if_t<std::is_arithmetic_v<T>> >
{
    template <class U>
    T operator()(scaled_integer<U> val) const
    { return val.template get<T>(); }

    // Convert string to arithmetic
    T operator()(std::string_view val) const
    {
        T ret{};
        auto [ptr, ec] = std::from_chars(val.data(), val.data() + val.size(), ret);
        if (ec == std::errc())
            return ret;
        else if (ec == std::errc::invalid_argument)
            throw fb::exception() << std::quoted(val) << " is not a number";
        else if (ec == std::errc::result_out_of_range)
            throw fb::exception("number \"") << val << "\" is larger than "
                                  << detail::type_name<T>();
        else
            throw fb::exception("can't convert string \"") << val << "\" to "
                                  << detail::type_name<T>();
    }
};

// T is std::string_view
template <>
struct type_converter<std::string_view>
{
    // Only string_view types can be returned as string_view
    std::string_view operator()(std::string_view val) const
    { return val; }
};

// T is std::string
template <>
struct type_converter<std::string>
{
    // std::string_view
    auto operator()(std::string_view val) const
    { return std::string(val);}

    // float, double
    template <class U>
    auto operator()(U val) const -> decltype(std::to_string(val))
    { return std::to_string(val); }

    // scaled_integer
    template <class U>
    auto operator()(scaled_integer<U> val) const
    { return std::to_string(val.template get()); }
};


// Helper methods
// Details
// namespace detail {

// Extract run-time value as a compile-time constant
template <class F, size_t... I>
inline bool to_const(size_t value, F&& fn, std::index_sequence<I...>)
{ return ((value == I && (fn(std::integral_constant<size_t, I>{}), true)) || ...); }

template <size_t N, class F>
inline bool to_const(size_t value, F&& fn)
{ return to_const(value, fn, std::make_index_sequence<N>{}); }

// Detect a type list
template <class>
struct is_tuple : std::false_type { };

template <class... Args>
struct is_tuple<std::tuple<Args...>> : std::true_type { };

template <class T>
inline constexpr bool is_tuple_v = is_tuple<T>::value;

} // namespace fb

