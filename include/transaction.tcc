#pragma once
// Transaction methods

namespace fb
{

struct transaction::context_t
{
    context_t(database& db)
    : _db(db)
    { }

    isc_tr_handle _handle = 0;
    database _db;
};


transaction::transaction(database& db)
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


isc_tr_handle* transaction::handle() noexcept
{ return &_context->_handle; }


database transaction::db() noexcept
{ return _context->_db; }

} // namespace fb

