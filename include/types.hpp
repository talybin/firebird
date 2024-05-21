#pragma once
#include "exception.hpp"

#include <variant>
#include <string_view>
#include <ctime>

namespace fb
{

// Tag to skip parameter
struct skip_t { };
constexpr const skip_t skip;

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

using blob_id_t = ISC_QUAD;

using field_t = std::variant<
    std::nullptr_t,
    std::string_view,
    int16_t,
    int32_t,
    int64_t,
    float,
    double,
    timestamp_t,
    blob_id_t
>;


// Visitor with type conversion
template <class T>
struct type_converter
{
    template <class F>
    std::enable_if_t<std::is_convertible_v<F, T>, T>
    operator()(F&& f) const
    { return std::forward<F>(f); }
};

// Visitor for string as resultat
template <>
struct type_converter<std::string>
{
    using T = std::string;

    // Can be constructed from
    template <class F>
    std::enable_if_t<std::is_constructible_v<T, F>, T>
    operator()(F&& f) const
    { return T(std::forward<F>(f)); }

    // Integer and floating point types
    template <class F>
    auto operator()(F&& f) const -> decltype(std::to_string(f))
    { return std::to_string(f); }
};


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

/*
// Assign if possible (used to traverse field_t)
template <class T, class U>
inline auto try_assign(T& t, U&& u) -> decltype(t = u, void())
{ t = std::forward<U>(u); }

// Fallback, do nothing
inline void try_assign(...)
{ throw fb::exception("can't assign, types differs too much"); }

#if 0
template <class T, class U>
inline auto try_convert(U&& u) -> decltype(T() = u, T())
{ return std::forward<U>(u); }

// Fallback, do nothing
inline void try_convert(...)
{ throw fb::exception("can't assign, types differs too much"); }
#endif
*/

// Detect a type list
template <class>
struct is_tuple : std::false_type { };

template <class... Args>
struct is_tuple<std::tuple<Args...>> : std::true_type { };

template <class T>
inline constexpr bool is_tuple_v = is_tuple<T>::value;

} // namespace fb

