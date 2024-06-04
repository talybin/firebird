#pragma once
#include "exception.hpp"
#include "sqlda.hpp"

// Transaction methods

namespace fb
{

struct transaction::context_t
{
    context_t(database& db) noexcept
    : _db(db)
    { }

    isc_tr_handle _handle = 0;
    database _db;
};


transaction::transaction(database& db) noexcept
: _context(std::make_shared<context_t>(db))
{ }


void transaction::start()
{
    context_t* c = _context.get();
    if (!c->_handle)
        invoke_except(isc_start_transaction, &c->_handle, 1, c->_db.handle(), 0, nullptr);
}


void transaction::commit()
{ invoke_except(isc_commit_transaction, &_context->_handle); }


void transaction::rollback()
{ invoke_except(isc_rollback_transaction, &_context->_handle); }


isc_tr_handle* transaction::handle() const noexcept
{ return &_context->_handle; }


database& transaction::db() const noexcept
{ return _context->_db; }


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
        &_context->_handle, 0, sql.data(), SQL_DIALECT_V6, params.get());
}

} // namespace fb

