#pragma once
#include "exception.hpp"
#include "sqlda.hpp"

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

/// Construct and attach database object.
transaction::transaction(database& db) noexcept
: _context(std::make_shared<context_t>(db))
{ }

/// Start transaction (if not started yet).
void transaction::start()
{
    context_t* c = _context.get();
    if (!c->_handle)
        invoke_except(isc_start_transaction, &c->_handle, 1, c->_db.handle(), 0, nullptr);
}

/// Commit (apply) pending changes.
void transaction::commit()
{ invoke_except(isc_commit_transaction, &_context->_handle); }

/// Rollback (cancel) pending changes.
void transaction::rollback()
{ invoke_except(isc_rollback_transaction, &_context->_handle); }

/// Get internal pointer to isc_tr_handle.
isc_tr_handle* transaction::handle() const noexcept
{ return &_context->_handle; }

/// Get database attached to this transaction.
database& transaction::db() const noexcept
{ return _context->_db; }

/// Prepares the DSQL statement, executes it once, and discards it.
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

