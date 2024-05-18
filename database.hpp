#pragma once
#include "ibase.h"
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
    void disconnect();

    // Default transaction used for simple queries. Starts on connect.
    transaction& default_transaction();

    // Methods of default transaction
    void commit();
    void rollback();

    // Raw handle
    isc_db_handle* handle();

private:
    struct context_t;
    std::shared_ptr<context_t> _context;
    std::shared_ptr<transaction> _def_trans;
};


struct transaction
{
    transaction(database& db)
    : _context(std::make_shared<context_t>(db))
    { }

    void start()
    {
        context_t* c = _context.get();
        if (!c->_handle)
            invoke_except(isc_start_transaction, &c->_handle, 1, c->_db.handle(), 0, nullptr);
    }

    void commit()
    { invoke_except(isc_commit_transaction, &_context->_handle); }

    void rollback()
    { invoke_except(isc_rollback_transaction, &_context->_handle); }

    isc_tr_handle* handle()
    { return &_context->_handle; }

    database db()
    { return _context->_db; }

private:
    struct context_t
    {
        context_t(database& db)
        : _db(db)
        { }

        isc_tr_handle _handle = 0;
        database _db;
    };

    std::shared_ptr<context_t> _context;
};


// database methods

// https://docwiki.embarcadero.com/InterBase/2020/en/DPB_Parameters
struct database::params
{
    params()
    { _dpb.reserve(256); }

    // Name is isc_dpb_ constant
    void add(int name)
    { _dpb.push_back(name); }

    // Add string parameter
    void add(int name, std::string_view value)
    {
        add(name);
        _dpb.push_back(value.size());
        std::copy(value.begin(), value.end(), std::back_inserter(_dpb));
    }

    std::vector<char> _dpb;
};


struct database::context_t
{
    //context_t(database& parent, std::string_view path)
    context_t(std::string_view path)
    : _path(path)
    { }

    // Disconnect here since it is shared context
    ~context_t()
    { disconnect(); }

    // isc_detach_database will set _handle to 0 on success
    void disconnect()
    { invoke_noexcept(isc_detach_database, &_handle); }

    params _params;
    std::string _path;
    isc_db_handle _handle = 0;
};


database::database(
    std::string_view path, std::string_view user, std::string_view passwd)
: _context(std::make_shared<context_t>(path))
, _def_trans(std::make_shared<transaction>(*this))
{
    params& p = _context->_params;
    // Fill database parameter buffer
    p.add(isc_dpb_version1);
    p.add(isc_dpb_user_name, user);
    p.add(isc_dpb_password, passwd);
}


void database::connect()
{
    context_t* c = _context.get();
    auto& dpb = c->_params._dpb;

    invoke_except(isc_attach_database,
        0, c->_path.c_str(), &c->_handle, dpb.size(), dpb.data());
}


void database::disconnect()
{ _context->disconnect(); }


transaction& database::default_transaction()
{ return *_def_trans.get(); }


void database::commit()
{ _def_trans->commit(); }


void database::rollback()
{ _def_trans->rollback(); }


isc_db_handle* database::handle()
{ return &_context->_handle; }

} // namespace fb
