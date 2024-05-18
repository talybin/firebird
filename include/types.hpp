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
    // Where 40587 is GDS epoch start in days (year ~1859, time_t starts at 1970).
    time_t to_time_t() const
    { return std::max(int(timestamp_date) - 40587, 0) * 86400 + timestamp_time / 10'000; }

    std::tm* to_tm() const {
        time_t epoch = to_time_t();
        return std::gmtime(&epoch);
    }

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


// Assign if possible (used to traverse field_t)
template <class T, class U>
inline auto try_assign(T& t, U&& u) -> decltype(t = u, void())
{ t = std::forward<U>(u); }

// Fallback, do nothing
inline void try_assign(...)
{ throw fb::exception("can't assign, types differs too much"); }

// Detect a type list
template <class>
struct is_tuple : std::false_type { };

template <class... Args>
struct is_tuple<std::tuple<Args...>> : std::true_type { };

template <class T>
inline constexpr bool is_tuple_v = is_tuple<T>::value;

} // namespace fb

