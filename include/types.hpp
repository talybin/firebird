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
    inline constexpr std::string_view type_name() noexcept
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

    template <class F, class T,
        class = std::void_t<decltype(static_cast<T>(std::declval<F>()))>>
    using static_castable = F;

    // Concept to accept enum values
    template <class F>
    using index_castable = static_castable<F, size_t>;

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
    time_t to_time_t() const noexcept
    { return std::max(int(timestamp_date) - 40587, 0) * 86400 + timestamp_time / 10'000; }

    std::tm* to_tm(std::tm* t) const noexcept {
        isc_decode_timestamp(const_cast<timestamp_t*>(this), t);
        return t;
    }

    static timestamp_t from_time_t(time_t t) noexcept
    {
        timestamp_t ret;
        ret.timestamp_date = t / 86400 + 40587;
        ret.timestamp_time = (t % 86400) * 10'000;
        return ret;
    }

    static timestamp_t from_tm(std::tm* t) noexcept
    {
        timestamp_t ret;
        isc_encode_timestamp(t, &ret);
        return ret;
    }

    static timestamp_t now() noexcept
    { return from_time_t(time(0)); }

    size_t ms() const noexcept
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

    std::string_view to_string(char* buf, size_t size) const
    {
        char* end = buf + size;
        // Reserve space for scale operation
        if (_scale < 0) --end;
        else if (_scale) end -= _scale;

        auto [it, ec] = std::to_chars(buf, end, _value);
        if (ec == std::errc()) {
            // No error
            if (_scale < 0) {
                // Shift scaled digits one step and insert dot
                for (auto x = _scale; x < 0; ++x, --it)
                    *it = it[-1];
                *it = '.';
                it += -_scale + 1;
            }
            // Add postfix zeroes if _scale > 0
            for (auto x = 0; x < _scale; ++x, ++it)
                *it = '0';
            return { buf, size_t(it - buf) };
        }
        throw fb::exception(std::make_error_code(ec).message());
    }

    std::string to_string() const
    {
        char buf[64];
        return std::string(to_string(buf, sizeof(buf)));
    }
};


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
struct type_converter
{
    // All types convertible to T or itself (ex. std::string_view)
    T operator()(T val) const noexcept
    { return val; }
};

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
        auto [ptr, ec] = std::from_chars(val.begin(), val.end(), ret);
        if (ec == std::errc())
            return ret;
        throw fb::exception("can't convert string \"") << val << "\" to "
                            << detail::type_name<T>() << "("
                            << std::make_error_code(ec).message() << ")";
    }
};

// T is std::string
template <>
struct type_converter<std::string>
{
    // std::string_view
    auto operator()(std::string_view val) const noexcept
    { return std::string(val);}

    // float, double
    template <class U>
    auto operator()(U val) const noexcept -> decltype(std::to_string(val))
    { return std::to_string(val); }

    // scaled_integer
    template <class U>
    auto operator()(scaled_integer<U> val) const
    { return val.to_string(); }
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

