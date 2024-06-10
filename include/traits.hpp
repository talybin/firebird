/// \file traits.hpp
/// This file contains utility templates and type traits for
/// various purposes including type reflection and detection.

#pragma once
#include <type_traits>

namespace fb
{

/// Get the type name as a string.
/// This function provides a way to get the name
/// of a type as a string view.
///
/// \tparam T - The type to get the name of.
///
/// \return A string view representing the name of the type.
///
template <class T>
inline constexpr std::string_view type_name() noexcept
{
    std::string_view sv = __PRETTY_FUNCTION__;
    sv.remove_prefix(sv.find('=') + 2);
    return { sv.data(), sv.find(';') };
}

/// Combine multiple functors into one, useful for the visitor pattern.
///
/// \tparam Ts... - The types of the functors.
///
/// \see https://en.cppreference.com/w/cpp/utility/variant/visit
///
template <class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

/// Explicit deduction guide for overloaded (not needed as of C++20).
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

/// Template argument used to replace unknown types.
struct any_type
{
    /// Conversion operator to any type.
    /// This allows 'any_type' to be implicitly converted to any type.
    ///
    /// \note This operator is not definined anywhere. Used in
    ///       template traits only.
    ///
    /// \tparam T - The type to convert to.
    ///
    template <class T>
    constexpr operator T(); // non explicit
};

// Experimental traits from std TS
// \see https://en.cppreference.com/w/cpp/experimental/is_detected

/// \namespace detail
/// Namespace for detail implementations of experimental traits.
namespace detail
{
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

} // namespace detail

/// Gets the detected type or a default type.
template<class Default, template<class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

/// Alias to get the type of detected_or
template< class Default, template<class...> class Op, class... Args >
using detected_or_t = typename detected_or<Default, Op, Args...>::type;

/// Checks if a type is a tuple. Returns false by default.
template <class>
struct is_tuple : std::false_type { };

/// This specialization returns true for std::tuple types.
template <class... Args>
struct is_tuple<std::tuple<Args...>> : std::true_type { };

/// Shorter alias for check if type is a tuple.
template <class T>
inline constexpr bool is_tuple_v = is_tuple<T>::value;

/// Use the same type for any index (usable in iteration of an index sequence).
template <size_t N, class T>
using index_type = T;

// Concepts

/// This concept checks if a type can be statically cast to another type.
template <class F, class T,
    class = std::void_t<decltype(static_cast<T>(std::declval<F>()))>>
using static_castable = F;

/// This concept checks if a type can be cast to a size_t.
template <class F>
using index_castable = static_castable<F, size_t>;

/// Concept for types constructible by std::string_view.
template <class T, class = std::enable_if_t<std::is_constructible_v<std::string_view, T>> >
using string_like = T;

} // namespace fb

