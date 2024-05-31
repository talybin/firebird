#pragma once
#include "database.hpp"

namespace fb
{

struct blob
{
    blob(transaction tr, blob_t blob_id) noexcept
    : _context(std::make_shared<context_t>(tr, blob_id))
    { }

    blob(database db, blob_t blob_id) noexcept
    : blob(db.default_transaction(), blob_id)
    { }

    void get()
    {
        context_t* c = _context.get();

        // Open first if not already
        if (!c->_handle) {
            // Blob Parameter Buffer (BPB) not provided
            invoke_except(isc_open_blob2,
                c->_trans.db().handle(), c->_trans.handle(), &c->_handle, &c->_id, 0, nullptr);
        }
    }

private:
    struct context_t
    {
        context_t(transaction& tr, blob_t blob_id) noexcept
        : _id(blob_id)
        , _trans(tr)
        { }

        ~context_t() noexcept
        { close(); }

        void close() noexcept
        {
            if (_handle)
                invoke_noexcept(isc_close_blob, &_handle);
        }

        isc_blob_handle _handle = 0;
        blob_t _id;
        transaction _trans;
    };

    std::shared_ptr<context_t> _context;
};

} // namespace fb

