/// \file types.hpp
/// This file contains the definitions of various structures and types
/// used for handling SQL data, timestamps, and type conversions.

#pragma once
#include "exception.hpp"
#include "traits.hpp"

#include <ibase.h>
#include <variant>
#include <string_view>
#include <ctime>
#include <charconv>
#include <iomanip>
#include <limits>

namespace fb
{

/// Implementation of ISC_TIMESTAMP structure.
/// It provides various methods to convert between ISC_TIMESTAMP
/// and 'time_t' or 'std::tm'.
struct timestamp_t : ISC_TIMESTAMP
{
    /// Convert timestamp_t to time_t.
    ///
    /// \note This will cut all dates below 1970-01-01, use to_tm() instead.
    ///
    /// \return Equivalent time_t value.
    ///
    time_t to_time_t() const noexcept
    // 40587 is GDS epoch start in days (since 1858-11-17, time_t starts at 1970).
    { return std::max(int(timestamp_date) - 40587, 0) * 86400 + timestamp_time / 10'000; }

    /// Convert timestamp_t to std::tm.
    ///
    /// \param[in,out] t - Pointer to std::tm structure.
    ///
    /// \return Pointer to the updated std::tm structure.
    ///
    std::tm* to_tm(std::tm* t) const noexcept {
        isc_decode_timestamp(const_cast<timestamp_t*>(this), t);
        return t;
    }

    /// Create timestamp_t from time_t.
    ///
    /// \param[in] t - time_t value.
    ///
    /// \return Equivalent timestamp_t value.
    ///
    static timestamp_t from_time_t(time_t t) noexcept
    {
        timestamp_t ret;
        ret.timestamp_date = t / 86400 + 40587;
        ret.timestamp_time = (t % 86400) * 10'000;
        return ret;
    }

    /// Create timestamp_t from std::tm.
    ///
    /// \param[in] t - Pointer to std::tm structure.
    ///
    /// \return Equivalent timestamp_t value.
    ///
    static timestamp_t from_tm(std::tm* t) noexcept
    {
        timestamp_t ret;
        isc_encode_timestamp(t, &ret);
        return ret;
    }

    /// Get the current timestamp.
    static timestamp_t now() noexcept
    { return from_time_t(time(0)); }

    /// Get milliseconds from the timestamp.
    size_t ms() const noexcept
    { return (timestamp_time / 10) % 1000; }
};

/// SQL integer type with scaling.
///
/// \tparam T - Underlying integer type.
///
template <class T>
struct scaled_integer
{
    /// Value of the integer.
    T _value;
    /// Scale factor.
    short _scale;

    /// Constructs a scaled_integer with a value and scale.
    ///
    /// \param[in] value - Integer value.
    /// \param[in] scale - Scale factor (optional, default is 0).
    ///
    explicit scaled_integer(T value, short scale = 0) noexcept
    : _value(value)
    , _scale(scale)
    { }

    /// Get the value with scale applied. Throws if value do
    /// not fit in given type.
    ///
    /// \tparam U - Type to return the value as.
    ///
    /// \return Value adjusted by the scale.
    /// \throw fb::exception
    ///
    template <class U = T>
    U get() const
    {
        // Do not allow types that may not fit the value
        if constexpr (sizeof(U) < sizeof(T)) {
            // Not using static_assert here because std::variant
            // will generate all posibilites of call to this
            // method and it will false trigger
            error<U>();
        }
        else {
            constexpr const U umax = std::numeric_limits<U>::max() / 10;
            constexpr const U umin = std::numeric_limits<U>::min() / 10;

            U val = _value;
            for (auto x = 0; x < _scale; ++x) {
                // Check for overflow
                if (val > umax || val < umin) error<U>();
                val *= 10;
            }
            for (auto x = _scale; x < 0; ++x)
                val /= 10;
            return val;
        }
    }

    /// Convert the scaled integer to a string. Throws if
    /// buffer is too small.
    ///
    /// \param[in] buf - Buffer to store the string.
    /// \param[in] size - Size of the buffer.
    ///
    /// \return String view of the scaled integer.
    /// \throw fb::exception
    ///
    std::string_view to_string(char* buf, size_t size) const;

    /// Convert the scaled integer to a string.
    std::string to_string() const
    {
        char buf[64];
        return std::string(to_string(buf, sizeof(buf)));
    }

private:
    /// Throws an error for invalid type conversions.
    template <class U>
    [[noreturn]] void error() const
    {
        throw fb::exception("scaled_integer of type \"")
              << type_name<T>()
              << "\" and scale " << _scale << " do not fit into \""
              << type_name<U>() << "\" type";
    }
};

// Convert the scaled integer to a string. Throws if
// buffer is too small.
template <class T>
std::string_view scaled_integer<T>::to_string(char* buf, size_t size) const
{
    auto check_range = [&buf](char* end) {
        if (end < buf)
            throw fb::exception("too small buffer");
        return end;
    };
    auto end = buf + size;

    // Special case for zero value
    if (_value == 0) {
        check_range(end - 1);
        *buf = '0';
        return { buf, 1 };
    }

    if (_scale < 0) {
        // Making value absolut or remainder will be negative
        T val = std::abs(_value);
        auto it = end;

        // Insert scaled digits
        for (auto x = _scale; x < 0; ++x, val /= 10)
            *check_range(--it) = (val % 10) + '0';
        // Add dot
        *check_range(--it) = '.';
        // Insert the rest
        for (; val; val /= 10)
            *check_range(--it) = (val % 10) + '0';
        // If last char is a dot, prepend '0'
        if (*it == '.')
            *check_range(--it) = '0';
        // Finally add sign for negative value
        if (_value < 0)
            *check_range(--it) = '-';

        return { it, size_t(end - it) };
    }
    else if (_scale)
        end -= _scale; // number of zeroes

    auto [it, ec] = std::to_chars(buf, end, _value);
    if (ec == std::errc{}) {
        // Add postfix zeroes if _scale > 0
        for (auto x = 0; x < _scale; ++x, ++it)
            *it = '0';
        return { buf, size_t(it - buf) };
    }
    throw fb::exception(std::make_error_code(ec).message());
}

/// BLOB id type.
using blob_id_t = ISC_QUAD;

/// Variant of SQL types.
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

/// Expandable type converter. Capable to convert from
/// a SQL type to any type defined here.
///
/// \tparam T - Requested (desination) type.
///
template <class T, class = void>
struct type_converter
{
    /// All types convertible to T or itself (ex. std::string_view).
    T operator()(T val) const noexcept
    { return val; }
};

/// Specialization for arithmetic (integral and floating-point) types.
template <class T>
struct type_converter<T, std::enable_if_t<std::is_arithmetic_v<T>> >
{
    /// Convert scaled_integer to the requested type.
    ///
    /// \tparam U - Underlying type of scaled_integer.
    /// \param[in] val - Scaled integer value to convert.
    ///
    /// \return Converted value.
    /// \throw fb::exception
    ///
    template <class U>
    T operator()(scaled_integer<U> val) const
    { return val.template get<T>(); }

    /// Convert string to arithmetic type.
    ///
    /// \param[in] val - String view to convert.
    ///
    /// \return Converted value.
    /// \throw fb::exception
    ///
    T operator()(std::string_view val) const
    {
        T ret{};
        auto [ptr, ec] = std::from_chars(val.begin(), val.end(), ret);
        if (ec == std::errc())
            return ret;
        throw fb::exception("can't convert string \"") << val << "\" to "
                            << type_name<T>() << "("
                            << std::make_error_code(ec).message() << ")";
    }
};

/// Specialization for std::string type.
template <>
struct type_converter<std::string>
{
    /// Convert string view to std::string.
    auto operator()(std::string_view val) const noexcept
    { return std::string(val);}

    /// Convert float or double to std::string.
    template <class U>
    auto operator()(U val) const noexcept -> decltype(std::to_string(val))
    { return std::to_string(val); }

    /// Convert scaled_integer to std::string.
    ///
    /// \tparam U - Underlying type of scaled_integer.
    /// \param[in] val - Scaled integer value to convert.
    ///
    /// \return Converted std::string.
    /// \throw fb::exception
    ///
    template <class U>
    auto operator()(scaled_integer<U> val) const
    { return val.to_string(); }
};

/// Tag to skip parameter.
///
/// \code{.cpp}
///     // Execute query with parameters, but send only second one (skip first)
///     qry.execute(fb::skip, "second");
/// \endcode
///
struct skip_t { };
inline constexpr skip_t skip;

} // namespace fb

