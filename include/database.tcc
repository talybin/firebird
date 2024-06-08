#pragma once
#include "traits.hpp"

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

