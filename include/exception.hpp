#pragma once
#include <ibase.h>
#include <stdexcept>
#include <sstream>

namespace fb
{

// Get error message from ISC_STATUS_ARRAY
inline std::string to_string(const ISC_STATUS* status)
{
    char buf[128];
    fb_interpret(buf, sizeof(buf), &status);
    return buf;
}


// Common exception for this library
struct exception : std::exception
{
    exception() = default;

    exception(std::string_view err)
    : _err(err)
    { }

    exception(const ISC_STATUS* status)
    : _err(to_string(status))
    { }

    template <class T>
    exception& operator<<(const T& arg)
    {
        std::ostringstream oss;
        oss << arg;
        _err.append(oss.str());
        return *this;
    }

    const char* what() const noexcept override
    { return _err.c_str(); }

private:
    std::string _err;
};


// Run API method and ignore status result
template <class F, class... Args>
inline int invoke_noexcept(F&& fn, Args&&... args) noexcept
{
    ISC_STATUS_ARRAY st;
    return fn(st, std::forward<Args>(args)...);
}

template <class F, class... Args>
inline ISC_STATUS
invoke_except_impl(std::string_view fn_name, F&& fn, Args&&... args)
{
    ISC_STATUS_ARRAY st;
    ISC_STATUS ret = fn(st, std::forward<Args>(args)...);
    if (st[0] == 1 && st[1])
        throw fb::exception(fn_name) << ": " << fb::to_string(st);
    return ret;
}

#ifndef invoke_except
#define invoke_except(cb, ...) \
    invoke_except_impl(#cb, cb, __VA_ARGS__)
#endif

} // namespace fb
