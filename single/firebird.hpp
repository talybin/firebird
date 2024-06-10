// MIT License
// 
// Copyright (c) 2024 Vladimir Talybin
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// This file was generated with a script.
// Generated 2024-06-10 09:52:12.196869+00:00 UTC
#pragma once

// beginning of include/firebird.hpp

/// \file firebird.hpp

// beginning of include/transaction.hpp

/// \file transaction.hpp

#include <ibase.h>
#include <memory>
#include <string_view>

namespace fb
{

struct database;

struct transaction
{
    /// Construct empty transaction and attach database later.
    /// Should not be used directly. Used by database to create
    /// default transaction.
    ///
    transaction() noexcept = default;

    /// Construct and attach database object.
    ///
    /// \param[in] db - Reference to database object.
    ///
    transaction(database& db) noexcept;

    /// Start transaction (if not started yet).
    ///
    /// \note Normally there is no need to call this method. Most
    ///       of the functions in this library will automatically
    ///       start attached transaction when necessary.
    ///
    /// \throw fb::exception
    ///
    void start();

    /// Commit (apply) pending changes.
    ///
    /// \throw fb::exception
    ///
    void commit();

    /// Rollback (cancel) pending changes.
    ///
    /// \throw fb::exception
    ///
    void rollback();

    /// Prepares the DSQL statement, executes it once, and
    /// discards it. The statement must not be one that
    /// returns data (that is, it must not be a SELECT or
    /// EXECUTE PROCEDURE statement, use fb::query for this).
    ///
    /// \param[in] sql - SQL query to execute.
    /// \param[in] params - Parameters to be set in SQL query (optional, if any).
    ///
    /// \throw fb::exception
    ///
    template <class... Args>
    void execute_immediate(std::string_view sql, const Args&... params);

    /// Get internal pointer to isc_tr_handle.
    isc_tr_handle* handle() const noexcept;

    /// Get database attached to this transaction.
    database& db() const noexcept;

private:
    struct context_t;
    std::shared_ptr<context_t> _context;
};

} // namespace fb

// end of include/transaction.hpp

// beginning of include/database.hpp

/// \file database.hpp

#include <variant>
#include <vector>

namespace fb
{

struct database
{
    /// Database connection parameter.
    /// A set of these parameters build a Database Parameter Buffer (DPB)
    /// that passed in connection request.
    ///
    /// \see https://docwiki.embarcadero.com/InterBase/2020/en/DPB_Parameters
    ///
    struct param
    {
        /// Type that do not hold any value.
        struct none_t { };

        /// Union of parameter value types.
        using value_type = std::variant<
            none_t,
            std::string_view,
            int
        >;

        /// Construct parameter.
        ///
        /// \param[in] name - Parameter name.
        /// \param[in] value - Parameter value. Optional, default is none.
        ///
        param(int name, value_type value = none_t{}) noexcept
        : _name(name)
        , _value(std::move(value))
        { }

        /// Pack parameter into DPB structure.
        ///
        /// \param[in] dpb - Vector to hold DPB data.
        ///
        void pack(std::vector<char>& dpb) const noexcept;

    private:
        /// Parameter name
        int _name;
        /// Parameter value (if any)
        value_type _value;
    };

    /// Construct database with connection parameters.
    ///
    /// \param[in] path - DSN connection string.
    /// \param[in] params - List of parameters.
    ///
    database(std::string_view path,
        std::initializer_list<param> params) noexcept;

    /// Construct database for connection with
    /// username and password.
    ///
    /// \param[in] path - DSN connection string.
    /// \param[in] user_name - User name, optional, default is "sysdba".
    /// \param[in] passwd - Password, optional, default is "masterkey".
    ///
    database(
        std::string_view path,
        std::string_view user_name = "sysdba",
        std::string_view passwd = "masterkey"
    ) noexcept
    : database(path, {
        { isc_dpb_user_name, user_name },
        { isc_dpb_password, passwd },
    })
    { }

    /// Connect to database using parameters provided in constructor.
    ///
    /// \throw fb::exception
    ///
    void connect();

    /// Disconnect from database.
    void disconnect() noexcept;

    /// Execute query once and discard it. Calls execute_immediate() of
    /// default transaction.
    ///
    /// \param[in] sql - SQL query to execute.
    /// \param[in] params - Parameters to be set in SQL query (optional, if any).
    ///
    /// \throw fb::exception
    /// \see transaction::execute_immediate
    ///
    template <class... Args>
    void execute_immediate(std::string_view sql, const Args&... params);

    /// Create new database.
    ///
    /// \code{.cpp}
    ///     fb::database new_db = fb::database::create(
    ///         "CREATE DATABASE 'localhost/3053:/firebird/data/mydb.ib' "
    ///         "USER 'SYSDBA' PASSWORD 'masterkey' "
    ///         "PAGE_SIZE 8192 DEFAULT CHARACTER SET UTF8");
    /// \endcode
    ///
    /// \param[in] sql - CREATE DATABASE query.
    ///
    /// \return Database object connected to newly created database.
    /// \throw fb::exception
    ///
    static database create(std::string_view sql);

    /// Get native internal handle.
    isc_db_handle* handle() const noexcept;

    /// Default transaction can be used to minimize written code by passing
    /// database object to fb::query directly instead of instantiate new
    /// transaction for each database connection.
    ///
    /// \code{.cpp}
    ///     fb::database db("employee");
    ///     fb::query(db, "select * from country");
    /// \endcode
    ///
    /// \return Transaction object.
    ///
    transaction& default_transaction() noexcept;

    /// Commit default transaction.
    ///
    /// \throw fb::exception
    ///
    void commit();

    /// Rollback default transaction.
    ///
    /// \throw fb::exception
    ///
    void rollback();

private:
    struct context_t;
    std::shared_ptr<context_t> _context;

    /// Default transaction.
    transaction _trans;

    /// Construct database from handle.
    database(isc_db_handle) noexcept;
};

} // namespace fb

// end of include/database.hpp

// beginning of include/transaction.tcc

// beginning of include/exception.hpp

/// \file exception.hpp
/// This file contains utility functions and structures
/// for error handling in the library.

#include <stdexcept>
#include <sstream>

namespace fb
{

/// Get error message from ISC_STATUS_ARRAY.
/// This function interprets the status array and returns
/// the corresponding error message as a string.
///
/// \param[in] status - Pointer to the ISC_STATUS array.
///
/// \return Error message as a string.
///
inline std::string to_string(const ISC_STATUS* status)
{
    char buf[128];
    fb_interpret(buf, sizeof(buf), &status);
    return buf;
}

/// Common exception for this library.
/// This class extends std::exception to provide additional
/// context and error handling for Firebird exceptions.
struct exception : std::exception
{
    /// Constructs exception without initial message.
    /// It is expected that the message will be streamed
    /// in right after construction.
    ///
    /// \code{.cpp}
    ///     throw fb::exception() << "error message";
    /// \endcode
    ///
    exception() = default;

    /// Constructs exception with initial message.
    ///
    /// \param[in] err - Error message.
    ///
    exception(std::string_view err)
    : _err(err)
    { }

    /// Constructs an exception from an ISC_STATUS array.
    exception(const ISC_STATUS* status)
    : _err(to_string(status))
    #if 1
    { }
    #else // TODO: Fix so we can see all messages
    { isc_print_status(status); }
    #endif

    /// Appends additional information to the error message.
    ///
    /// \code{.cpp}
    ///     throw fb::exception("err: ") << "value is " << 42;
    /// \endcode
    ///
    /// \tparam T - Type of the additional information.
    /// \param[in] arg - Additional information to append.
    ///
    /// \return Reference to the current exception object
    ///         so more additional information can be added.
    ///
    template <class T>
    exception& operator<<(const T& arg)
    {
        std::ostringstream oss;
        oss << arg;
        _err.append(oss.str());
        return *this;
    }

    /// Returns the error message.
    const char* what() const noexcept override
    { return _err.c_str(); }

private:
    std::string _err;
};

/// Run an API method and ignore status result (do not throw on error).
///
/// \code{.cpp}
///     invoke_noexcept(isc_dsql_fetch, &_handle, SQL_DIALECT_CURRENT, _fields);
/// \endcode
///
/// \param[in] fn - Function to run.
/// \param[in] args... - Function arguments except the first ISC_STATUS_ARRAY.
///
/// \return ISC_STATUS result of the function.
///
template <class F, class... Args>
inline ISC_STATUS invoke_noexcept(F&& fn, Args&&... args) noexcept
{
    ISC_STATUS_ARRAY st;
    return fn(st, std::forward<Args>(args)...);
}

/// Run API method and throw exception on error.
///
/// \code{.cpp}
///     invoke_except(isc_dsql_fetch, &_handle, SQL_DIALECT_CURRENT, _fields);
/// \endcode
///
/// \param[in] fn - Function to run.
/// \param[in] args... - Function arguments except the first ISC_STATUS_ARRAY.
///
/// \return ISC_STATUS result of the function on success.
/// \throw fb::exception
///
template <class F, class... Args>
inline ISC_STATUS
invoke_except(F&& fn, Args&&... args)
{
    ISC_STATUS_ARRAY st;
    ISC_STATUS ret = fn(st, std::forward<Args>(args)...);
    if (st[0] == 1 && st[1])
        throw fb::exception(st);
    return ret;
}

} // namespace fb
// end of include/exception.hpp

// beginning of include/sqlda.hpp

/// \file sqlda.hpp
/// This file contains the definition of the sqlda structure used
/// for transporting data in SQL statements.

// beginning of include/sqlvar.hpp

/// \file sqlvar.hpp
/// This file contains the definition of the sqlvar structure for
/// handling SQL variable data.

// beginning of include/types.hpp

/// \file types.hpp
/// This file contains the definitions of various structures and types
/// used for handling SQL data, timestamps, and type conversions.

// beginning of include/traits.hpp

/// \file traits.hpp
/// This file contains utility templates and type traits for
/// various purposes including type reflection and detection.

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

// end of include/traits.hpp

#include <ctime>
#include <charconv>
#include <iomanip>
#include <limits>
#include <cmath>

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

    /// Convert the scaled integer to a null terminated string.
    /// Throws if buffer is too small.
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

// Convert the scaled integer to a null terminated string
template <class T>
std::string_view scaled_integer<T>::to_string(char* buf, size_t buf_size) const
{
    int64_t val = _value;
    int cnt = 0;

    // Special case for zero value
    if (val == 0)
        cnt = std::snprintf(buf, buf_size, "0");
    else if (_scale < 0) {
        // Making value absolut or remainder will be negative
        auto div = std::div(std::abs(val), std::pow(10, -_scale));
        cnt = std::snprintf(buf, buf_size,
            "%s%li.%0*li", val < 0 ? "-" : "", div.quot, -_scale, div.rem);
    }
    else if (_scale)
        cnt = std::snprintf(buf, buf_size, "%li%0*hi", val, _scale, 0);
    else
        cnt = std::snprintf(buf, buf_size, "%li", val);

    // cnt is a number of characters that would have been written
    // for a sufficiently large buffer if successful (not including
    // the terminating null character), or a negative value if an
    // error occurred. Thus, the (null-terminated) output has been
    // completely written if and only if the returned value is
    // nonnegative and less than buf_size.
    if (cnt < 0)
        throw fb::exception("could not convert to string");
    if (cnt < buf_size)
        return { buf, size_t(cnt) };
    throw fb::exception("too small buffer");
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

// end of include/types.hpp

#include <cstdlib>

namespace fb
{

/// This is a view of XSQLVAR. Be careful with expired values.
///
/// \see https://docwiki.embarcadero.com/InterBase/2020/en/XSQLVAR_Field_Descriptions
///
struct sqlvar
{
    /// Alias for XSQLVAR type.
    using type = XSQLVAR;
    /// Alias for pointer to XSQLVAR type.
    using pointer = std::add_pointer_t<type>;

    /// Constructs an sqlvar object from a given XSQLVAR pointer.
    ///
    /// \param[in] var - Pointer to XSQLVAR.
    ///
    explicit sqlvar(pointer var) noexcept
    : _ptr(var) { }

    // Set methods

    /// Generic set method for sqltype, data, and length.
    ///
    /// \param[in] stype - SQL type.
    /// \param[in] sdata - Pointer to data.
    /// \param[in] slen - Length of data.
    ///
    void set(int stype, const void* sdata, size_t slen) noexcept
    {
        _ptr->sqltype = stype;
        _ptr->sqldata = const_cast<char*>(reinterpret_cast<const char*>(sdata));
        _ptr->sqllen = slen;
    }

    /// Skip method, does nothing.
    void set(skip_t) noexcept { }

    /// Sets the value of the SQL variable to a float.
    void set(const float& val) noexcept
    { set(SQL_FLOAT, &val, sizeof(float)); }

    /// Sets the value of the SQL variable to a double.
    void set(const double& val) noexcept
    { set(SQL_DOUBLE, &val, sizeof(double)); }

    /// Sets the value of the SQL variable to an int16_t.
    void set(const int16_t& val) noexcept
    { set(SQL_SHORT, &val, 2); }

    /// Sets the value of the SQL variable to an int32_t.
    void set(const int32_t& val) noexcept
    { set(SQL_LONG, &val, 4); }

    /// Sets the value of the SQL variable to an int64_t.
    void set(const int64_t& val) noexcept
    { set(SQL_INT64, &val, 8); }

    /// Sets the value of the SQL variable to a string.
    void set(std::string_view val) noexcept
    { set(SQL_TEXT, val.data(), val.size()); }

    /// Sets the value of the SQL variable to a timestamp.
    void set(const timestamp_t& val) noexcept
    { set(SQL_TIMESTAMP, &val, sizeof(timestamp_t)); }

    /// Sets the value of the SQL variable to a blob.
    void set(const blob_id_t& val) noexcept
    { set(SQL_BLOB, &val, sizeof(blob_id_t)); }

    /// Sets the value of the SQL variable to null.
    void set(std::nullptr_t) noexcept
    { _ptr->sqltype = SQL_NULL; }

    /// Set by assigning.
    ///
    /// \tparam T - Type of the value.
    /// \param[in] val - Value to assign.
    ///
    /// \return Reference to this sqlvar object.
    ///
    template <class T>
    auto operator=(const T& val) noexcept -> decltype(set(val), *this)
    { return (set(val), *this); }

    // Get methods

    /// Returns the internal pointer to XSQLVAR (const version).
    const pointer handle() const noexcept
    { return _ptr; }

    /// Returns the internal pointer to XSQLVAR.
    pointer handle() noexcept
    { return _ptr; }

    /// Gets the column name.
    std::string_view name() const noexcept
    { return std::string_view(_ptr->sqlname, _ptr->sqlname_length); }

    /// Gets the table name.
    std::string_view table() const noexcept
    { return std::string_view(_ptr->relname, _ptr->relname_length); }

    /// Gets the SQL data type.
    uint16_t sql_datatype() const noexcept
    { return _ptr->sqltype & ~1; }

    /// Gets the maximum column size (in bytes) as
    /// allocated on firebird server. For example
    /// maximum allowed length of string for this
    /// specific column as given as VARCHAR(size).
    ///
    /// \note set() methods may rewrite this value.
    ///
    size_t size() const noexcept
    { return _ptr->sqllen; }

    /// Checks if the value is null.
    bool is_null() const noexcept
    { return (_ptr->sqltype & 1) && (*_ptr->sqlind < 0); }

    /// Checks for null.
    ///
    /// \code{.cpp}
    ///     if (row["CUSTOMER_ID"]) {
    ///         // This field is not null
    ///     }
    /// \endcode
    ///
    operator bool() const noexcept
    { return !is_null(); }

    /// Return the value by assigning.
    ///
    /// \code{.cpp}
    ///     int cust_id = row["CUSTOMER_ID"];
    /// \endcode
    ///
    /// \tparam T - Type to convert to.
    ///
    /// \return Value of the specified type.
    /// \throw fb::exception
    ///
    template <class T>
    operator T() const
    { return value<T>(); }

    /// Gets the value. Throws if the value is null.
    /// The actual type of the value (as received from
    /// database) may be converted to requested type.
    ///
    /// \code{.cpp}
    ///     // The actual type of field is int32_t, but we want a string here
    ///     auto cust_id = row["CUSTOMER_ID"].value<std::string>();
    /// \endcode
    ///
    /// \tparam T - Type of the value.
    ///
    /// \return Value of the specified type.
    /// \throw fb::exception
    ///
    template <class T>
    T value() const
    {
        if (is_null())
            throw fb::exception("type is null");
        return visit<T>();
    }

    /// Gets the value or default value if null.
    ///
    /// \code{.cpp}
    ///     // Set cust_id to 0 if field is null
    ///     auto cust_id = row["CUSTOMER_ID"].value_or(0);
    /// \endcode
    ///
    /// \tparam T - Type of the value.
    /// \param[in] default_value - Default value to return if null.
    ///
    /// \return Value of the specified type or default value.
    /// \throw fb::exception
    ///
    template <class T>
    T value_or(T default_value) const
    {
        if (is_null())
            return std::move(default_value);
        return visit<T>();
    }

    /// Gets the value as a variant (field_t).
    ///
    /// \return Value as a variant.
    /// \throw fb::exception
    ///
    field_t as_variant() const;

private:
    /// Internal pointer to XSQLVAR
    pointer _ptr;

    /// Extract value from a variant. Value may be
    /// converted to another type by type_converter.
    template <class T>
    T visit() const
    {
        return std::visit(overloaded {
            type_converter<T>{},
            [](...) -> T {
                throw fb::exception("can't convert to type ") << type_name<T>();
            }
        }, as_variant());
    }
};

// Gets the value as a variant (field_t).
field_t sqlvar::as_variant() const
{
    if (is_null())
        return field_t(std::in_place_type<std::nullptr_t>, nullptr);

    // Get data type without null flag
    short dtype = sql_datatype();
    char* data = _ptr->sqldata;

    switch (dtype) {

    case SQL_TEXT:
        return field_t(std::in_place_type<std::string_view>, data, _ptr->sqllen);

    case SQL_VARYING:
    {
        // While sqllen set to max column size, vary_length
        // is actual length of this field
        auto pv = reinterpret_cast<PARAMVARY*>(data);
        return field_t(
            std::in_place_type<std::string_view>,
            (const char*)pv->vary_string, pv->vary_length);
    }

    case SQL_SHORT:
        return field_t(
            std::in_place_type<scaled_integer<int16_t>>,
            *reinterpret_cast<int16_t*>(data), _ptr->sqlscale);

    case SQL_LONG:
        return field_t(
            std::in_place_type<scaled_integer<int32_t>>,
            *reinterpret_cast<int32_t*>(data), _ptr->sqlscale);

    case SQL_INT64:
        return field_t(
            std::in_place_type<scaled_integer<int64_t>>,
            *reinterpret_cast<int64_t*>(data), _ptr->sqlscale);

    case SQL_FLOAT:
        return field_t(std::in_place_type<float>, *reinterpret_cast<float*>(data));

    case SQL_DOUBLE:
        return field_t(std::in_place_type<double>, *reinterpret_cast<double*>(data));

    case SQL_TIMESTAMP:
        return field_t(std::in_place_type<timestamp_t>, *reinterpret_cast<timestamp_t*>(data));

    case SQL_TYPE_DATE:
    {
        timestamp_t t;
        t.timestamp_date = *reinterpret_cast<ISC_DATE*>(data);
        return field_t(std::in_place_type<timestamp_t>, t);
    }

    case SQL_TYPE_TIME:
    {
        timestamp_t t;
        t.timestamp_time = *reinterpret_cast<ISC_TIME*>(data);
        return field_t(std::in_place_type<timestamp_t>, t);
    }

    case SQL_BLOB:
    case SQL_ARRAY:
        return field_t(std::in_place_type<blob_id_t>, *reinterpret_cast<ISC_QUAD*>(data));

    default:
        throw fb::exception("type (") << dtype << ") not implemented";
    }
}

} // namespace fb

// end of include/sqlvar.hpp

#include <cassert>

namespace fb
{

/// XSQLDA is a host-language data structure that DSQL uses to transport
/// data to or from a database when processing a SQL statement string.
///
/// \see https://docwiki.embarcadero.com/InterBase/2020/en/XSQLDA_Field_Descriptions_(Embedded_SQL_Guide)
struct sqlda
{
    /// Column iterator.
    struct iterator;

    using type = XSQLDA;
    using pointer = std::add_pointer_t<type>;

    using ptr_t = std::unique_ptr<type, decltype(&free)>;
    using data_buffer_t = std::vector<char>;

    /// Constructs an sqlda object with a specified number of columns.
    ///
    /// \param[in] nr_cols - Number of columns to allocate (optional,
    ///                      default is 0). Zero number of columns
    ///                      will not allocate any memory.
    ///
    explicit sqlda(size_t nr_cols = 0) noexcept
    : _ptr(alloc(nr_cols))
    { }

    /// Conversion operator to pointer.
    ///
    /// \return Pointer to XSQLDA type.
    /// \see get()
    ///
    operator pointer() const noexcept
    { return get(); }

    /// Gets the raw pointer to XSQLDA.
    ///
    /// \return Pointer to XSQLDA type.
    ///
    pointer get() const noexcept
    { return _ptr.get(); }

    /// XSQLDA member access operator.
    ///
    /// \return Pointer to XSQLDA type.
    ///
    pointer operator->() const noexcept
    { return get(); }

    /// Column iterator
    iterator begin() const noexcept;

    /// Column iterator
    iterator end() const noexcept;

    /// Gets the number of used columns.
    ///
    /// \return Number of columns currently in use.
    ///
    size_t size() const noexcept
    { return _ptr ? _ptr->sqld : 0; }

    /// Changes the number of columns stored. If capacity
    /// is below required number, it will be increased.
    ///
    /// \param[in] nr_cols - Number of columns to set.
    ///
    void resize(size_t nr_cols) noexcept
    {
        assert(nr_cols > 0);
        if (nr_cols > capacity())
            reserve(nr_cols);
        if (_ptr)
            _ptr->sqld = nr_cols;
    }

    /// Gets the number of columns allocated.
    ///
    /// \return Number of columns allocated.
    ///
    size_t capacity() const noexcept
    { return _ptr ? _ptr->sqln : 0; }

    /// Resizes capacity to the given number of columns.
    ///
    /// \param[in] nr_cols - Number of columns to allocate.
    ///
    void reserve(size_t nr_cols) noexcept
    { _ptr = alloc(nr_cols); }

    /// Access column by index (with check for out of range).
    ///
    /// \tparam T - Type of index (integral type or enum).
    /// \param[in] pos - Position to access.
    ///
    /// \return sqlvar object at the specified position.
    /// \throw fb::exception
    ///
    template <class T>
    sqlvar at(index_castable<T> pos) const 
    {
        // We need static_cast here in case T is an enum
        size_t index = static_cast<size_t>(pos);
        if (index >= size())
            throw fb::exception("index out of range, index ") << index << " >= size " << size();
        return this->operator[](index);
    }

    /// Access by column name (a bit slower than by index).
    ///
    /// \param[in] pos - Column name.
    ///
    /// \return sqlvar object for the specified column name.
    /// \throw fb::exception
    ///
    sqlvar at(std::string_view pos) const
    {
        auto end = &_ptr->sqlvar[size()];
        for (auto it = _ptr->sqlvar; it != end; ++it) {
            sqlvar v(it);
            if (v.name() == pos)
                return v;
        }
        throw fb::exception() << std::quoted(pos) << " not found";
    }

    /// Access column by index without range check.
    ///
    /// \tparam T - Type of index (integral type or enum).
    /// \param[in] pos - Position to access.
    ///
    /// \return sqlvar object at the specified position.
    ///
    template <class T>
    sqlvar operator[](index_castable<T> pos) const noexcept
    {
        assert(static_cast<size_t>(pos) < size());
        return sqlvar(&_ptr->sqlvar[static_cast<size_t>(pos)]);
    }

    /// Access by column name.
    ///
    /// \param[in] pos - Column name.
    ///
    /// \return sqlvar object for the specified column name.
    /// \throw fb::exception
    ///
    sqlvar operator[](std::string_view pos) const
    { return at(pos); }

    /// Allocates aligned space for incoming data.
    void alloc_data() noexcept;

    /// Construct a tuple of specified indexes.
    ///
    /// \code{.cpp}
    ///     // Get columns 0, 7 and 8
    ///     auto tup = row.as_tuple<0, 7, 8>();
    /// \endcode
    ///
    /// \tparam I... - Indexes.
    ///
    /// \return Tuple containing sqlvar objects.
    ///
    template <auto... I>
    auto as_tuple() const noexcept
    { return std::make_tuple(this->operator[](I)...); }

    /// Sets input parameters.
    ///
    /// \note This creates a view to arguments and XSQLVARs
    ///       are valid as long as args do not expire.
    ///
    /// \tparam Args... - Value types.
    /// \param[in] args... - Any values (numeric, strings, ...).
    ///
    /// \throw fb::exception
    /// \see query::execute()
    ///
    template <class... Args>
    std::enable_if_t<(sizeof...(Args) > 0)>
    set(const Args&... args);

    /// Dummy method that does nothing.
    void set() noexcept { }

    /// Visits columns with a visitor function. Argument
    /// type is sqlvar.
    ///
    /// \code{.cpp}
    ///     // Visit max 15 fields instead of default 10
    ///     row.visit<15>([](auto... columns) {
    ///         std::cout << "row has " << sizeof...(columns) << " columns\n";
    ///     });
    /// \endcode
    ///
    /// \tparam MAX_FIELDS - Maximum number of fields that
    ///                      can be visited. Increase this
    ///                      number if more required.
    ///                      Default is 10 fields.
    /// \tparam F - Visitor function type.
    /// \param[in] cb - Visitor function.
    ///
    /// \return Result of the visitor function.
    /// \throw fb::exception
    ///
    template <size_t MAX_FIELDS = 10, class F>
    constexpr decltype(auto)
    visit(F&& cb) const;

private:
    ptr_t _ptr;
    data_buffer_t _data_buffer;

    // Visit details
    template <class F, class> struct visitor_impl;
    template <class F, size_t N>
    using visitor = visitor_impl<F, std::make_index_sequence<N>>;

    /// Allocate and initializae buffer enough to hold given
    /// number of columns.
    ///
    /// \return nullptr if nr_cols is zero.
    ///
    static ptr_t alloc(size_t nr_cols) noexcept
    {
        pointer ptr = nullptr;
        if (nr_cols) {
            ptr = pointer(calloc(1, XSQLDA_LENGTH(nr_cols)));
            ptr->version = 1;
            ptr->sqln = nr_cols;
        }
        return { ptr, &free };
    }
};

/// Iterator for column traversing
struct sqlda::iterator
{
    /// Iterator value type.
    using value_type = sqlvar;
    /// Alias for pointer to sqlvar.
    using pointer = value_type*;
    /// Alias for reference to sqlvar.
    using reference = value_type&;

    /// Constructs an iterator from given sqlvar pointer.
    iterator(value_type::pointer var) noexcept
    : _var(var)
    { }

    /// Dereference operator.
    reference operator*() noexcept
    { return _var; }

    /// Member access operator.
    pointer operator->() noexcept
    { return &_var; }

    /// Prefix increment (++x)
    iterator& operator++() noexcept {
        _var = value_type(_var.handle() + 1);
        return *this;
    }

    /// Postfix increment (x++)
    iterator operator++(int) noexcept {
        iterator cur(_var.handle());
        this->operator++();
        return cur;
    }

    /// Prefix decrement (--x)
    iterator& operator--() noexcept {
        _var = value_type(_var.handle() - 1);
        return *this;
    }

    /// Postfix decrement (x--)
    iterator operator--(int) noexcept {
        iterator cur(_var.handle());
        this->operator--();
        return cur;
    }

    /// Equality operator.
    bool operator==(const iterator& rhs) const noexcept
    { return _var.handle() == rhs._var.handle(); };

    /// Inequality operator.
    bool operator!=(const iterator& rhs) const noexcept
    { return _var.handle() != rhs._var.handle(); };

private:
    /// sqlvar object for the iterator.
    value_type _var;
};

// Returns an iterator to the first column.
sqlda::iterator sqlda::begin() const noexcept
{ return _ptr ? _ptr->sqlvar : nullptr; }

// Returns an iterator to the end.
sqlda::iterator sqlda::end() const noexcept
{ return _ptr ? &_ptr->sqlvar[std::min(size(), capacity())] : nullptr; }

// Sets input parameters.
template <class... Args>
std::enable_if_t<(sizeof...(Args) > 0)>
sqlda::set(const Args&... args)
{
    constexpr size_t cnt = sizeof...(Args);
    if (size() != cnt)
        throw fb::exception(
            "set: wrong number of parameters (should be ")
            << size() << ", called with " << cnt << ")";

    auto it = begin();
    ((it->set(args), ++it), ...);
}

// Allocates aligned space for incoming data.
void sqlda::alloc_data() noexcept
{
    // Calculate data size and offsets
    size_t offset = 0;
    for (auto it = begin(); it != end(); ++it)
    {
        auto p = it->handle();
        size_t len = p->sqllen;
        short dtype = p->sqltype & ~1;

        if (dtype == SQL_VARYING)
            len += sizeof(short);

        p->sqldata = (char*)offset;
        // Align size to sizeof(short)
        offset += (len + 1) & ~1;

        p->sqlind = (short*)offset;
        offset += sizeof(short);
    }

    // Allocate storage
    _data_buffer.reserve(offset);

    // Apply offsets to new storage
    char* buf = _data_buffer.data();
    for (auto it = begin(); it != end(); ++it)
    {
        auto p = it->handle();
        p->sqldata = buf + (size_t)p->sqldata;
        p->sqlind = (short*)(buf + (size_t)p->sqlind);
    }
}

// Visit details
template <class F, size_t... I>
struct sqlda::visitor_impl<F, std::index_sequence<I...>>
{
    // Get return type of function if invokable, any_type otherwise
    using ret_type = detected_or_t<
        any_type,
        std::invoke_result_t, F, index_type<I, sqlvar>...
    >;

    template <class R>
    static constexpr R
    visit(F&& f, sqlvar::pointer ptr)
    {
        if constexpr(std::is_invocable_v<F, index_type<I, sqlvar>...>)
            return std::forward<F>(f)(sqlvar(&ptr[I])...);
        else
            throw fb::exception("wrong number of arguments: ") << sizeof...(I);
    }
};

// Visit columns.
template <size_t MAX_FIELDS, class F>
constexpr decltype(auto)
sqlda::visit(F&& cb) const
{
    return []<size_t... I>(
        size_t index, F&& f, sqlvar::pointer ptr, std::index_sequence<I...>)
        -> decltype(auto)
    {
        // Get the return type of given callback function.
        // Fallback (could not detect type) is any_type.
        using detected_ret = detected_or<
            any_type,
            std::common_type_t, typename sqlda::visitor<F, I>::ret_type...
        >;
        static_assert(typename detected_ret::value_t(),
            "visit requires the visitor to have the same return type "
            "for all number of arguments");

        using R = typename detected_ret::type;
        static_assert(
            // Note, this does not detect variadic number of arguments
            // (any number of arguments will return non-any_type).
            !std::is_same_v<R, any_type>,
            "too many fields to visit, increase number of max_fields");

        // Build a virtual function table
        constexpr R (*vtable[])(F&&, sqlvar::pointer) = {
            &sqlda::visitor<F, I>::template visit<R>...
        };
        // Select the one that match number of fields (arguments)
        // or less if index is out of range (call only first few)
        return vtable[std::min(index, MAX_FIELDS)](std::forward<F>(f), ptr);
    }
    (size(), std::forward<F>(cb), _ptr->sqlvar,
        // Including callback with zero arguments (sequence for 0..(MAX_FIELDS + 1))
        std::make_index_sequence<MAX_FIELDS + 1>{});
}

} // namespace fb

// end of include/sqlda.hpp

// Transaction methods

namespace fb
{

/// Transaction internal data.
struct transaction::context_t
{
    context_t(database& db) noexcept
    : _db(db)
    { }

    isc_tr_handle _handle = 0;
    database _db;
};

// Construct and attach database object.
transaction::transaction(database& db) noexcept
: _context(std::make_shared<context_t>(db))
{ }

// Start transaction (if not started yet).
void transaction::start()
{
    context_t* c = _context.get();
    if (!c->_handle)
        invoke_except(isc_start_transaction, &c->_handle, 1, c->_db.handle(), 0, nullptr);
}

// Commit (apply) pending changes.
void transaction::commit()
{ invoke_except(isc_commit_transaction, &_context->_handle); }

// Rollback (cancel) pending changes.
void transaction::rollback()
{ invoke_except(isc_rollback_transaction, &_context->_handle); }

// Get internal pointer to isc_tr_handle.
isc_tr_handle* transaction::handle() const noexcept
{ return &_context->_handle; }

// Get database attached to this transaction.
database& transaction::db() const noexcept
{ return _context->_db; }

// Prepares the DSQL statement, executes it once, and discards it.
template <class... Args>
void transaction::execute_immediate(std::string_view sql, const Args&... args)
{
    constexpr size_t nr_params = sizeof...(Args);

    // Start transaction if not already
    start();

    // Set parameters (if any)
    sqlda params;
    if constexpr (nr_params > 0) {
        params.resize(nr_params);
        params.set(args...);
    }

    // Execute
    invoke_except(isc_dsql_execute_immediate, _context->_db.handle(),
        &_context->_handle, 0, sql.data(), SQL_DIALECT_CURRENT, params.get());
}

} // namespace fb

// end of include/transaction.tcc

// beginning of include/database.tcc

// Database methods

namespace fb
{

// Pack parameter into DPB structure
void database::param::pack(std::vector<char>& dpb) const noexcept
{
    std::visit(overloaded {
        [&](none_t) { dpb.push_back(_name); },
        [&](std::string_view val) {
            dpb.push_back(_name);
            dpb.push_back(val.size());
            std::copy(val.begin(), val.end(), std::back_inserter(dpb));
        },
        [&](int val) {
            dpb.push_back(_name);
            dpb.push_back(val);
        },
    }, _value);
}

/// Database internal data.
struct database::context_t
{
    /// Construct database from handle.
    context_t(isc_db_handle h) noexcept
    : _handle(h)
    { }

    /// Construct database for connection.
    context_t(std::string_view path) noexcept
    : _path(path)
    {
        _params.reserve(64);
    }

    /// Disconnect here since it is shared context.
    ~context_t() noexcept
    { disconnect(); }

    /// Disconnect database.
    ///
    /// \note isc_detach_database will set _handle to 0 on success.
    ///
    void disconnect() noexcept
    { invoke_noexcept(isc_detach_database, &_handle); }

    /// Database Parameter Buffer (DPB).
    std::vector<char> _params;
    /// DSN path.
    std::string _path;
    /// Native internal handle.
    isc_db_handle _handle = 0;
};

// Construct database with connection parameters.
database::database(
    std::string_view path, std::initializer_list<param> params) noexcept
: _context(std::make_shared<context_t>(path))
{
    // Setup default transaction but not use (start) it
    _trans = *this;
    // Append parameters. Begin with a version.
    param(isc_dpb_version1).pack(_context->_params);
    for (auto& p : params)
        p.pack(_context->_params);
}

// Construct database from handle.
database::database(isc_db_handle h) noexcept
: _context(std::make_shared<context_t>(h))
{
    _trans = *this;
}

// Connect to database using parameters provided in constructor.
void database::connect()
{
    context_t* c = _context.get();
    auto& dpb = c->_params;

    invoke_except(isc_attach_database,
        0, c->_path.c_str(), &c->_handle, dpb.size(), dpb.data());
}

// Disconnect from database.
void database::disconnect() noexcept
{ _context->disconnect(); }

// Execute query once and discard it.
template <class... Args>
void database::execute_immediate(std::string_view sql, const Args&... params)
{ _trans.execute_immediate(sql, params...); }

// Create new database.
database database::create(std::string_view sql)
{
    isc_db_handle db_handle = 0;
    isc_tr_handle tr_handle = 0;

    // In the special case where the statement is CREATE DATABASE, there
    // is no transaction, so db_handle and trans_handle must be pointers
    // to handles whose value is NULL. When isc_dsql_execute_immediate()
    // returns, db_handle is a valid handle, just as though you had made
    // a call to isc_attach_database()
    invoke_except(isc_dsql_execute_immediate,
        &db_handle, &tr_handle, 0, sql.data(), SQL_DIALECT_CURRENT, nullptr);

    return db_handle;
}

// Get native internal handle.
isc_db_handle* database::handle() const noexcept
{ return &_context->_handle; }

// Get default transaction.
transaction& database::default_transaction() noexcept
{ return _trans; }

// Commit default transaction.
void database::commit()
{ _trans.commit(); }

// Rollback default transaction.
void database::rollback()
{ _trans.rollback(); }

} // namespace fb

// end of include/database.tcc

// beginning of include/query.hpp

/// \file query.hpp

// beginning of include/blob.hpp

/// \file blob.hpp
/// This file contains the definition of the blob structure for
/// handling BLOB data in Firebird.

namespace fb
{

/// This structure provides methods to create, read, and write BLOB data.
/// \see https://docwiki.embarcadero.com/InterBase/2020/en/Blob_Functions
struct blob
{
    /// Create a blob for inserting (creates new blob id).
    ///
    /// \note For UPDATE and SELECT you must use the same
    ///       blob id for modifying and reading.
    ///
    /// \param[in] tr - Reference to a transaction object.
    ///
    blob(transaction& tr)
    : _context(std::make_shared<context_t>(tr))
    { }

    /// Create a blob for inserting using the default transaction
    /// of a database.
    ///
    /// \param[in] db - Reference to a database object.
    ///
    blob(database& db)
    : blob(db.default_transaction())
    { }

    /// Construct an readable or modifying blob (open)
    ///
    /// \param[in] tr - Reference to a transaction object.
    /// \param[in] id - Blob id.
    ///
    blob(transaction& tr, blob_id_t id)
    : _context(std::make_shared<context_t>(tr, id))
    { }

    /// Construct a readable or modifying blob using the
    /// default transaction of a database.
    ///
    /// \param[in] db - Reference to a database object.
    /// \param[in] id - Blob id.
    ///
    blob(database& db, blob_id_t id)
    : blob(db.default_transaction(), id)
    { }

    /// Get reference to stored blob id.
    blob_id_t& id() const noexcept
    { return _context->_id; }

    /// Get the raw handle of the blob.
    isc_blob_handle handle() const noexcept
    { return _context->_handle; }

    /// Convert to blob_id_t.
    operator blob_id_t&() const noexcept
    { return _context->_id; }

    /// Set blob from supported types (expandable).
    ///
    /// \code{.cpp}
    ///     // Execute query with blob set to a string
    ///     q.execute(fb::blob(db).set("new value"));
    /// \endcode
    ///
    /// \tparam T - Unspecified type, make overloads to specify.
    /// \param[in] from - Value to set.
    ///
    /// \return Reference to the current blob object so it can
    ///         be passed inline to sqlda::set method.
    ///
    template <class T>
    blob& set(T from);

    /// Get chunk of data.
    ///
    /// \param[in] buf - Buffer to store the data.
    /// \param[in] buf_length - Length of the buffer.
    ///
    /// \return Number of bytes actually read
    ///         (0 if no more data available).
    ///
    uint16_t read_chunk(char* buf, uint16_t buf_length) const;

    /// Write chunk of data.
    ///
    /// \note Blob must be closed after last chunk of data
    ///       before execution of query.
    ///
    /// \param[in] buf - Buffer containing the data.
    /// \param[in] buf_length - Length of the buffer.
    ///
    /// \see close()
    ///
    void write_chunk(const char* buf, uint16_t buf_length);

    /// Close the blob after the last chunk has been written.
    void close() noexcept
    { _context->close(); }

private:
    /// Blob's internal data
    struct context_t
    {
        /// Create constructor
        context_t(transaction& tr)
        {
            // Start transaction if not already
            tr.start();
            invoke_except(isc_create_blob,
                tr.db().handle(), tr.handle(), &_handle, &_id);
        }

        /// Read constructor
        context_t(transaction& tr, blob_id_t id)
        : _id(id)
        {
            invoke_except(isc_open_blob,
                tr.db().handle(), tr.handle(), &_handle, &id);
        }

        /// Close on destruct
        ~context_t() noexcept
        { close(); }

        /// Close the blob handle.
        void close() noexcept
        { _handle && (invoke_noexcept(isc_close_blob, &_handle), true); }

        isc_blob_handle _handle = 0;
        blob_id_t _id;
    };

    std::shared_ptr<context_t> _context;
};

// Read a chunk of data from the blob.
uint16_t blob::read_chunk(char* buf, uint16_t buf_length) const
{
    ISC_STATUS_ARRAY status;
    uint16_t nr_read = 0;

    ISC_STATUS get_status = isc_get_segment(
        status, &_context->_handle, &nr_read, buf_length, buf);

    // TODO should we close the stream on isc_segstr_eof?

    // isc_segstr_eof is not actually an error
    if (get_status == 0 || get_status == isc_segstr_eof)
        return nr_read;

    throw fb::exception(status);
}

// Write a chunk of data to the blob.
void blob::write_chunk(const char* buf, uint16_t buf_length)
{
    invoke_except(isc_put_segment,
        &_context->_handle, buf_length, const_cast<char*>(buf));
}

/// Set blob from a string.
/// Overload of blob::set() method.
/// Create your own overload for type required.
template <class T>
blob& blob::set(string_like<T> value)
{
    std::string_view str(value);
    write_chunk(str.data(), str.size());
    close();
    return *this;
}

} // namespace fb

/// Write blob as string to a stream.
inline std::ostream& operator<<(std::ostream& os, const fb::blob& b)
{
    // Recommended size for segment is 80
    char buf[80];
    for (uint16_t len; (len = b.read_chunk(buf, sizeof(buf)));)
        os << std::string_view(buf, len);
    return os;
}

// end of include/blob.hpp

namespace fb
{

/// Executes SQL query and retrieves data.
struct query
{
    /// Rows iterator.
    struct iterator;

    /// Construct query for given transaction.
    ///
    /// @param[in] tr - Transaction.
    /// @param[in] sql - SQL query to be executed.
    ///
    query(transaction tr, std::string_view sql) noexcept
    : _context(std::make_shared<context_t>(tr, sql))
    { }

    /// Construct query to be used in default transaction
    /// for given database.
    ///
    /// @param[in] db - Database.
    /// @param[in] sql - SQL query to be executed.
    ///
    query(database db, std::string_view sql) noexcept
    : query(db.default_transaction(), sql)
    { }

    /// Access input parameters for reading and writing.
    /// Should be called before execute() method.
    ///
    /// For the first use of this method, query will try to
    /// detect number of parameters needed. If you know how
    /// many parameters required you can hint the size.
    /// This will prevent reallocation of memory if required
    /// number of parameters must be increased.
    ///
    /// @param[in] hint_size - Number of parameters to reserve
    ///                        (optional, default is 1).
    ///
    /// \return Parameter list.
    /// \throw fb::exception
    ///
    sqlda& params(size_t hint_size = 1);

    /// Access output fields (result of SELECT) for reading
    /// only. Should be called after execute() method.
    ///
    /// \return Field list.
    ///
    const sqlda& fields() const noexcept
    { return _context->_fields; }

    /// Prepare query and buffer for receiving data (if not
    /// prepared yet).
    ///
    /// Normally there is no need to call this method.
    /// It is called by other methods as needed.
    ///
    /// \throw fb::exception
    ///
    void prepare();

    /// Execute query with parameters (optional). If parameters
    /// required, but not given here, previously created
    /// parameters (via params() method) apply. It is also
    /// possible to mix this two methods, see example. Use
    /// fb::skip to set only previously unset ones.
    ///
    /// \code{.cpp}
    ///     fb::query q(db, "select * from sales where a = ? and b = ?);
    ///     q.params()["a"] = 42;
    ///     q.execute(fb::skip, "shipped"); // "a" is already set
    ///     // ... later ...
    ///     q.params()["b"] = "open";
    ///     q.execute(); // using previously set "a" and "b"
    /// \endcode
    ///
    /// @param[in] args - Parameters to pass to this query
    ///                   (optional). Number of arguments
    ///                   must match required number or none.
    ///
    /// \return Reference to this query.
    /// \throw fb::exception
    ///
    template <class... Args>
    query& execute(const Args&... args);

    /// Close read cursor. Closing need to be called only
    /// if reading data (from ex. SELECT) need to be cancelled
    /// and new execute invoked.
    void close() noexcept
    { _context->close(); }

    /// Row begin iterator.
    iterator begin() const noexcept;
    /// Row end iterator.
    iterator end() const noexcept;

    /// Get column names.
    std::vector<std::string_view> column_names() const noexcept;

    /// Iterate result of SELECT query by calling callback for
    /// each row.
    ///
    /// \code{.cpp}
    ///     fb::query q(db, "select * from country");
    ///     q.execute().foreach([](auto country, auto currency)
    ///     {
    ///         std::cout << country.name() << ": "
    ///                   << country.value_or("null") << std::endl;
    ///         std::cout << currency.name() << ": "
    ///                   << currency.value_or("null") << std::endl;
    ///     });
    /// \endcode
    ///
    /// \throw fb::exception
    ///
    template <class F>
    void foreach(F&& cb) const
    {
        context_t* c = _context.get();
        for (; c->_is_data_available; c->fetch())
            c->_fields.visit(std::forward<F>(cb));
    }

private:
    /// Query internal data
    struct context_t
    {
        /// Construct query context.
        context_t(transaction& tr, std::string_view sql) noexcept
        : _trans(tr)
        , _sql(sql)
        // Initially expect to receive 5 columns
        , _fields(5)
        { }

        /// Free query on destruct.
        ~context_t() noexcept
        { close(DSQL_drop); }

        /// Release query handle.
        ///
        /// @param[in] op - Operation:
        ///                 DSQL_close - Close and ready to execute again,
        ///                 DSQL_drop - Release query (can't be used anymore).
        ///
        void close(uint16_t op = DSQL_close) noexcept
        { invoke_noexcept(isc_dsql_free_statement, &_handle, op); }

        /// Fetch next row.
        ///
        /// \return false if no more rows available.
        ///
        bool fetch() noexcept
        {
            _is_data_available =
                invoke_noexcept(isc_dsql_fetch, &_handle, SQL_DIALECT_CURRENT, _fields) == 0;
            // Note! The cursor must be closed before next execution
            return _is_data_available || (close(), false);
        }

        isc_stmt_handle _handle = 0;
        bool _is_prepared = false;
        bool _is_data_available = false;
        transaction _trans;
        std::string _sql;

        /// Input parameters
        sqlda _params;
        /// Output fields
        sqlda _fields;
    };

    std::shared_ptr<context_t> _context;
};

/// Row iterator
struct query::iterator
{
    using iterator_category = std::forward_iterator_tag;
    using value_type = sqlda;
    using pointer = value_type*;
    using reference = value_type&;

    /// Construct empty (end) iterator.
    iterator() noexcept = default;

    iterator(query::context_t* ctx) noexcept
    : _ctx(ctx)
    { }

    reference operator*() const noexcept
    { return _ctx->_fields; }

    pointer operator->() const noexcept
    { return &_ctx->_fields; }

    // Prefix increment (++x)
    iterator& operator++() noexcept
    {
        if (!_ctx->fetch())
            _ctx = nullptr;
        return *this;
    }

    // Postfix increment (x++)
    // TODO: This is broken! How to return current
    //       value and then increment
    iterator operator++(int) noexcept {
        auto cur = _ctx;
        this->operator++();
        return cur;
    }

    bool operator==(const iterator& rhs) const noexcept
    { return _ctx == rhs._ctx; };

    bool operator!=(const iterator& rhs) const noexcept
    { return _ctx != rhs._ctx; };

private:
    query::context_t* _ctx = nullptr;
};

// Row begin iterator.
query::iterator query::begin() const noexcept
{ return _context->_is_data_available ? _context.get() : end(); }

// Row end iterator.
query::iterator query::end() const noexcept
{ return {}; }

// Access input parameters for reading and writing.
sqlda& query::params(size_t hint_size)
{
    context_t* c = _context.get();

    // For first use allocate required number of parameters
    if (!c->_params.get())
    {
        // First we need to call prepare and allocate atleast
        // one parameter to get the rest
        prepare();
        c->_params.reserve(std::max(hint_size, size_t(1)));

        // Prepare input parameters
        invoke_except(isc_dsql_describe_bind, &c->_handle, SQL_DIALECT_CURRENT, c->_params);
        if (c->_params.capacity() < c->_params.size()) {
            c->_params.reserve(c->_params.size());
            // Reread prepared description
            invoke_except(isc_dsql_describe_bind, &c->_handle, SQL_DIALECT_CURRENT, c->_params);
        }
    }
    return c->_params;
}

// Prepare query and buffer for receiving data.
void query::prepare()
{
    context_t* c = _context.get();

    // Run only once
    if (c->_is_prepared)
        return;

    // Start transaction if not already
    c->_trans.start();

    // Allocate handle
    invoke_except(isc_dsql_allocate_statement, c->_trans.db().handle(), &c->_handle);
    // Prepare query
    invoke_except(isc_dsql_prepare,
        c->_trans.handle(), &c->_handle, 0, c->_sql.c_str(), SQL_DIALECT_CURRENT, c->_fields);

    // Prepare output fields
    if (c->_fields.capacity() < c->_fields.size()) {
        c->_fields.reserve(c->_fields.size());
        // Reread prepared description
        invoke_except(isc_dsql_describe, &c->_handle, SQL_DIALECT_CURRENT, c->_fields);
    }
    // Allocate buffer for receiving data
    if (c->_fields.size())
        c->_fields.alloc_data();

    c->_is_prepared = true;
}

// Execute query.
template <class... Args>
query& query::execute(const Args&... args)
{
    context_t* c = _context.get();

    // All queries must be prepared before execution.
    // Note, prepare runs only once.
    prepare();

    // Apply input parameters (if any)
    if constexpr (sizeof...(Args) > 0)
        params(sizeof...(Args)).set(args...);

    // Execute
    invoke_except(isc_dsql_execute,
        c->_trans.handle(), &c->_handle, SQL_DIALECT_CURRENT, c->_params);

    // If there data to be read we need to make sure
    // the first entry is present so iterators may
    // start to read from *begin()
    if (c->_fields.size())
        c->fetch();

    return *this;
}

// Get column names.
std::vector<std::string_view> query::column_names() const noexcept
{
    std::vector<std::string_view> view;
    view.reserve(_context->_fields.size());

    for (auto& var : _context->_fields)
        view.push_back(var.name());
    return view;
}

} // namespace fb

// end of include/query.hpp

// end of include/firebird.hpp

