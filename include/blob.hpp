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
    // Return number of bytes read and true if it was last chunk.
    std::pair<uint16_t, bool>
    get_segment(char* buf, uint16_t buf_length) const
    {
        ISC_STATUS_ARRAY status;
        uint16_t nr_read = 0;

        ISC_STATUS get_status = isc_get_segment(
            status, &_context->_handle, &nr_read, buf_length, buf);

        // isc_segstr_eof is not actually an error
        if (get_status == isc_segstr_eof)
            return { nr_read, true };
        // Throw on error
        if (status[0] == 1 && status[1])
            throw fb::exception(status);

        return { nr_read, false };
    }

private:
    struct context_t
    {
        context_t(transaction& tr, blob_id_t id) noexcept
        : _id(id)
        {
            // Blob Parameter Buffer (BPB) not provided
            invoke_except(isc_open_blob2,
                tr.db().handle(), tr.handle(), &_handle, &id, 0, nullptr);
        }

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
    for (;;) {
        auto [len, is_last] = b.get_segment(buf, sizeof(buf));
        if (len)
            os << std::string_view(buf, len);
        if (is_last)
            break;
    }
    return os;
}

