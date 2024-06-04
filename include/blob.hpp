#pragma once
#include "database.hpp"

namespace fb
{

// https://docwiki.embarcadero.com/InterBase/2020/en/Blob_Functions
struct blob
{
    // Create a blob for inserting (creates new blob id).
    // Note, for UPDATE and SELECT you must use the same
    // blob id for modifying and reading.
    blob(transaction& tr)
    : _context(std::make_shared<context_t>(tr))
    { }

    // Create a blob for inserting
    blob(database& db)
    : blob(db.default_transaction())
    { }

    // Construct an readable or modifying blob (open)
    blob(transaction& tr, blob_id_t id)
    : _context(std::make_shared<context_t>(tr, id))
    { }

    // Construct an readable or modifying blob
    blob(database& db, blob_id_t id)
    : blob(db.default_transaction(), id)
    { }

    // Get stored blob id
    blob_id_t id() const noexcept
    { return _context->_id; }

    // Get raw handle
    isc_blob_handle handle() const noexcept
    { return _context->_handle; }

    // Convert to blob_id_t
    operator blob_id_t() const noexcept
    { return _context->_id; }

    // Set blob to a string
    blob& set(std::string_view str)
    { return (write_chunk(str.data(), str.size()), *this); }

    // Get chunk of data.
    // Return number of bytes actually read (0 if no more data available).
    uint16_t read_chunk(char* buf, uint16_t buf_length) const;

    // Write chunk of data
    void write_chunk(const char* buf, uint16_t buf_length);

private:
    struct context_t
    {
        // Create constructor
        context_t(transaction& tr)
        {
            // Blob Parameter Buffer (BPB) not supported yet
            invoke_except(isc_create_blob2,
                tr.db().handle(), tr.handle(), &_handle, &_id, 0, nullptr);
        }

        // Read constructor
        context_t(transaction& tr, blob_id_t id)
        : _id(id)
        {
            // Blob Parameter Buffer (BPB) not supported yet
            invoke_except(isc_open_blob2,
                tr.db().handle(), tr.handle(), &_handle, &id, 0, nullptr);
        }

        // Close on destruct
        ~context_t() noexcept
        { close(); }

        void close() noexcept
        { _handle && (invoke_noexcept(isc_close_blob, &_handle), true); }

        isc_blob_handle _handle = 0;
        blob_id_t _id;
    };

    std::shared_ptr<context_t> _context;
};


uint16_t blob::read_chunk(char* buf, uint16_t buf_length) const
{
    ISC_STATUS_ARRAY status;
    uint16_t nr_read = 0;

    ISC_STATUS get_status = isc_get_segment(
        status, &_context->_handle, &nr_read, buf_length, buf);

    // TODO should we close the stream on isc_segstr_eof?

    // isc_segstr_eof is not actually an error
    if (get_status == 0 || get_status == isc_segstr_eof)
        return nr_read;

    throw fb::exception(status);
}


void blob::write_chunk(const char* buf, uint16_t buf_length)
{
    invoke_except(isc_put_segment,
        &_context->_handle, buf_length, const_cast<char*>(buf));
}

} // namespace fb


// Write blob as string to a stream
inline std::ostream& operator<<(std::ostream& os, const fb::blob& b)
{
    // Recommended size for segment is 80
    char buf[80];
    for (uint16_t len; (len = b.read_chunk(buf, sizeof(buf)));)
        os << std::string_view(buf, len);
    return os;
}

