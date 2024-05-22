#pragma once
#include "exception.hpp"

#include <memory>
#include <string_view>
#include <vector>

namespace fb
{

// Run API method and ignore status result
template <class F, class... Args>
inline int invoke_noexcept(F&& fn, Args&&... args)
{
    ISC_STATUS_ARRAY st;
    return fn(st, std::forward<Args>(args)...);
}

template <class F, class... Args>
inline void invoke_except_impl(std::string_view fn_name, F&& fn, Args&&... args)
{
    ISC_STATUS_ARRAY st;
    if (fn(st, std::forward<Args>(args)...))
        throw fb::exception(fn_name) << ": " << fb::to_string(st);
}

#ifndef invoke_except
#define invoke_except(cb, ...) \
    invoke_except_impl(#cb, cb, __VA_ARGS__)
#endif


struct transaction;

struct database
{
    // Database Parameter Buffer
    struct params;

    // Constructor
    database(std::string_view path,
        std::string_view user = "sysdba", std::string_view passwd = "masterkey");

    // Connect. Throw exeption on error.
    void connect();

    // Disconnect from database
    void disconnect() noexcept;

    // Default transaction used for simple queries. Starts on connect.
    transaction& default_transaction() noexcept;

    // Methods of default transaction
    void commit();
    void rollback();

    // Raw handle
    isc_db_handle* handle() noexcept;

private:
    struct context_t;
    std::shared_ptr<context_t> _context;
    std::shared_ptr<transaction> _def_trans;
};

} // namespace fb

