#pragma once
#include "database.hpp"

namespace fb
{

// https://docwiki.embarcadero.com/InterBase/2020/en/Blob_Functions
struct blob
{
    blob(transaction& tr, blob_id_t id) noexcept
    : _context(std::make_shared<context_t>(tr, id))
    { }

    blob(database& db, blob_id_t id) noexcept
    : blob(db.default_transaction(), id)
    { }

    // Get chunk of data.
    // Return number of bytes actually read (0 if no more data available).
    uint16_t read(char* buf, uint16_t buf_length) const
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

private:
    #if 0
    static void check_error(const ISC_STATUS* status)
    {
        if (status[0] == 1 && status[1])
            throw fb::exception(status);
    }
    #endif

    struct context_t
    {
        // Read constructor
        context_t(transaction& tr, blob_id_t id) noexcept
        : _id(id)
        {
            // Blob Parameter Buffer (BPB) not provided
            invoke_except(isc_open_blob2,
                tr.db().handle(), tr.handle(), &_handle, &id, 0, nullptr);
        }

        // TODO write constructor

        ~context_t() noexcept
        { close(); }

        void close() noexcept
        { _handle && (invoke_noexcept(isc_close_blob, &_handle), true); }

        isc_blob_handle _handle = 0;
        blob_id_t _id;
    };

    std::shared_ptr<context_t> _context;
};

} // namespace fb

inline std::ostream& operator<<(std::ostream& os, const fb::blob& b)
{
    // Recommended size for segment is 80
    char buf[80];
    for (uint16_t len; (len = b.read(buf, sizeof(buf)));)
        os << std::string_view(buf, len);
    return os;
}

