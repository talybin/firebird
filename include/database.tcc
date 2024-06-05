#pragma once
#include <vector>

// Database methods

namespace fb
{

/// Database Parameter Buffer (DPB).
///
/// \see https://docwiki.embarcadero.com/InterBase/2020/en/DPB_Parameters
///
struct database::params
{
    params()
    { _dpb.reserve(256); }

    // Name is isc_dpb_ constant
    void add(int name) noexcept
    { _dpb.push_back(name); }

    // Add string parameter
    void add(int name, std::string_view value) noexcept
    {
        add(name);
        _dpb.push_back(value.size());
        std::copy(value.begin(), value.end(), std::back_inserter(_dpb));
    }

    std::vector<char> _dpb;
};

/// Database internal data
struct database::context_t
{
    /// Construct database from handle.
    context_t(isc_db_handle h) noexcept
    : _handle(h)
    { }

    /// Construct database for connection.
    context_t(std::string_view path) noexcept
    : _path(path)
    { }

    /// Disconnect here since it is shared context.
    ~context_t() noexcept
    { disconnect(); }

    /// Disconnect database.
    ///
    /// \note isc_detach_database will set _handle to 0 on success.
    ///
    void disconnect() noexcept
    { invoke_noexcept(isc_detach_database, &_handle); }

    params _params;
    std::string _path;
    isc_db_handle _handle = 0;
};

/// Construct database for connection.
database::database(
    std::string_view path, std::string_view user, std::string_view passwd) noexcept
: _context(std::make_shared<context_t>(path))
{
    _trans = *this;

    params& p = _context->_params;
    // Fill database parameter buffer
    p.add(isc_dpb_version1);
    p.add(isc_dpb_user_name, user);
    p.add(isc_dpb_password, passwd);
}

/// Construct database from handle.
database::database(isc_db_handle h) noexcept
: _context(std::make_shared<context_t>(h))
{
    _trans = *this;
}

/// Connect to database using parameters provided in constructor.
void database::connect()
{
    context_t* c = _context.get();
    auto& dpb = c->_params._dpb;

    invoke_except(isc_attach_database,
        0, c->_path.c_str(), &c->_handle, dpb.size(), dpb.data());
}

/// Disconnect from database.
void database::disconnect() noexcept
{ _context->disconnect(); }

/// Execute query once and discard it.
template <class... Args>
void database::execute_immediate(std::string_view sql, const Args&... params)
{ _trans.execute_immediate(sql, params...); }

/// Create new database.
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
        &db_handle, &tr_handle, 0, sql.data(), SQL_DIALECT_V6, nullptr);

    return db_handle;
}

/// Get native internal handle.
isc_db_handle* database::handle() const noexcept
{ return &_context->_handle; }

/// Get default transaction.
transaction& database::default_transaction() noexcept
{ return _trans; }

/// Commit default transaction.
void database::commit()
{ _trans.commit(); }

/// Rollback default transaction.
void database::rollback()
{ _trans.rollback(); }

} // namespace fb

