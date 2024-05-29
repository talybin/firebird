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


// Convertor to any type
struct any_type
{
    template <class T>
    constexpr operator T(); // non explicit
};


// Experimental traits from std TS
// https://en.cppreference.com/w/cpp/experimental/is_detected

template<class Default, class AlwaysVoid, template<class...> class Op, class... Args>
struct detector
{
    using value_t = std::false_type;
    using type = Default;
};

template<class Default, template<class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
{
    using value_t = std::true_type;
    using type = Op<Args...>;
};

template<template<class...> class Op, class... Args>
using is_detected = typename detector<any_type, void, Op, Args...>::value_t;

template<template<class...> class Op, class... Args>
using detected_t = typename detector<any_type, void, Op, Args...>::type;

template<class Default, template<class...> class Op, class... Args>
using detected_or = detector<Default, void, Op, Args...>;


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

} // namespace fb::detail

