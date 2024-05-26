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


} // namespace fb
