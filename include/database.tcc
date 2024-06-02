#pragma once
#include <vector>

// Database methods

namespace fb
{

// https://docwiki.embarcadero.com/InterBase/2020/en/DPB_Parameters
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


struct database::context_t
{
    context_t(std::string_view path) noexcept
    : _path(path)
    { }

    // Disconnect here since it is shared context
    ~context_t()
    { disconnect(); }

    // isc_detach_database will set _handle to 0 on success
    void disconnect() noexcept
    { invoke_noexcept(isc_detach_database, &_handle); }

    params _params;
    std::string _path;
    isc_db_handle _handle = 0;
};


database::database(
    std::string_view path, std::string_view user, std::string_view passwd) noexcept
: _context(std::make_shared<context_t>(path))
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


void database::disconnect() noexcept
{ _context->disconnect(); }


isc_db_handle* database::handle() const noexcept
{ return &_context->_handle; }


transaction& database::default_transaction() noexcept
{ return _trans ? *_trans : _trans.emplace(*this); }


void database::commit()
{ default_transaction().commit(); }


void database::rollback()
{ default_transaction().rollback(); }

} // namespace fb

