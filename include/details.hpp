#pragma once

namespace fb::detail
{

// Get type as string (reflection)
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


// Extract run-time value as a compile-time constant
template <class F, size_t... I>
inline bool to_const(size_t value, F&& fn, std::index_sequence<I...>)
{ return ((value == I && (fn(std::integral_constant<size_t, I>{}), true)) || ...); }

template <size_t N, class F>
inline bool to_const(size_t value, F&& fn)
{ return to_const(value, fn, std::make_index_sequence<N>{}); }


// Detect std::tuple
template <class>
struct is_tuple : std::false_type { };

template <class... Args>
struct is_tuple<std::tuple<Args...>> : std::true_type { };

template <class T>
inline constexpr bool is_tuple_v = is_tuple<T>::value;


// Use same type for any index (usable in iteration of index sequence)
template <size_t N, class T>
using index_type = T;


#if 0 // Not used
// Generate tuple implementation
template <class, class>
struct generate_impl;

template <class T, size_t... I>
struct generate_impl<T, std::index_sequence<I...>>
{
    using type = std::tuple<index_type<I, T>...>;
};

// Generate tuple of size N filled with type T
template <class T, size_t N>
using generate = typename generate_impl<T, std::make_index_sequence<N>>::type;
#endif

} // namespace fb::detail

