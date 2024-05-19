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

template <class T>
inline constexpr std::string_view typeid_name()
{
    std::string_view sv = __PRETTY_FUNCTION__;
    sv.remove_prefix(sv.find('=') + 2);
    return { sv.data(), sv.find(';') };
}

#if 0
template <class T>
struct type_converter
{
    template <class F>
    T operator()(F&&) const;

    // Fallback on no match found
    T operator()(...) const
    { throw fb::exception("can't convert to type ") << typeid_name<T>(); }
};

template <class F, class T, class = std::enable_if_t<std::is_convertible_v<F, T>>>
using convertible_t = F;

template <class T>
template <class F>
T type_converter<T>::operator()(convertible_t<F, T>&& f) const
{ return std::forward<F>(f); }


template <class F, class = std::void_t<decltype(std::to_string(F()))>>
using str_convertible_t = F;


template <>
template <class F>
std::string type_converter<std::string>::operator()(str_convertible_t<F>&& f) const
{ return std::to_string(f); }

#else
template <class T>
struct type_converter
{
    template <class F>
    std::enable_if_t<std::is_convertible_v<F, T>, T>
    operator()(F&& f) const
    { return std::forward<F>(f); }

    T operator()(...) const
    { throw fb::exception("can't convert to type ") << typeid_name<T>(); }
};

template <>
struct type_converter<std::string>
{
    template <class F>
    auto operator()(F&& f) const -> decltype(std::to_string(f))
    { return std::to_string(f); }

    std::string operator()(...) const
    { throw fb::exception("can't convert to type std::string"); }
};
#endif




/*
template <>
template <class F>
auto type_converter<std::string>::operator()(F&& f) const -> decltype(std::to_string(f))
{ return std::to_string(f); }
*/

#if 0
template <class T>
struct type_converter
{
    template <class F>
    T hmm(F&&) const;
/*
    template <class F>
    std::enable_if_t<std::is_assignable_v<T, F>, T>
    operator()(F&& f) const
    { return std::forward<F>(f); }
*/
/*
    template <class F>
    T operator()(F&&) const
    { throw fb::exception("hmm"); }
*/
};

template <class T>
template <class F>
std::enable_if_t<std::is_assignable_v<T, F>, T>
type_converter<T>::hmm(F&& f) const
{ return std::forward<F>(f); }
#endif

// Converters to string
/*
template <class F>
auto type_converter<std::string>::operator()(F&& f) const -> decltype(std::to_string(f))
{ return std::to_string(std::forward<F>(f)); }
*/

/*
    template <class F>
    std::enable_if_t<std::is_assignable_v<T, F>, T>
    operator()(F&& f) const
    { return std::forward<F>(f); }
};
*/

/*
// Converters to string
template <>
struct type_converter<std::string>
{
    using T = std::string;

    template <class F>
    auto operator()(F&& f) const -> decltype(std::to_string(f))
    { return std::to_string(std::forward<F>(f)); }

    template <class F>
    std::enable_if_t<std::is_assignable_v<T, F>, T>
    operator()(F&& f) const
    { return std::forward<F>(f); }
};
*/

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

