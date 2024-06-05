/// \file exception.hpp
/// This file contains utility functions and structures
/// for error handling in the library.

#pragma once
#include <ibase.h>
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
    { }

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
///     invoke_noexcept(isc_dsql_fetch, &_handle, SQL_DIALECT_V6, _fields);
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
///     invoke_except(isc_dsql_fetch, &_handle, SQL_DIALECT_V6, _fields);
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
