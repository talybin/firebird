/// \file blob.hpp
/// This file contains the definition of the blob structure for
/// handling BLOB data in Firebird.

#pragma once
#include "database.hpp"

namespace fb
{

/// This structure provides methods to create, read, and write BLOB data.
/// \see https://docwiki.embarcadero.com/InterBase/2020/en/Blob_Functions
struct blob
{
    /// Create a blob for inserting (creates new blob id).
    ///
    /// \note For UPDATE and SELECT you must use the same
    ///       blob id for modifying and reading.
    ///
    /// \param[in] tr - Reference to a transaction object.
    ///
    blob(transaction& tr)
    : _context(std::make_shared<context_t>(tr))
    { }

    /// Create a blob for inserting using the default transaction
    /// of a database.
    ///
    /// \param[in] db - Reference to a database object.
    ///
    blob(database& db)
    : blob(db.default_transaction())
    { }

    /// Construct an readable or modifying blob (open)
    ///
    /// \param[in] tr - Reference to a transaction object.
    /// \param[in] id - Blob id.
    ///
    blob(transaction& tr, blob_id_t id)
    : _context(std::make_shared<context_t>(tr, id))
    { }

    /// Construct a readable or modifying blob using the
    /// default transaction of a database.
    ///
    /// \param[in] db - Reference to a database object.
    /// \param[in] id - Blob id.
    ///
    blob(database& db, blob_id_t id)
    : blob(db.default_transaction(), id)
    { }

    /// Get reference to stored blob id.
    blob_id_t& id() const noexcept
    { return _context->_id; }

    /// Get the raw handle of the blob.
    isc_blob_handle handle() const noexcept
    { return _context->_handle; }

    /// Convert to blob_id_t.
    operator blob_id_t&() const noexcept
    { return _context->_id; }

    /// Set blob from supported types (expandable).
    ///
    /// \code{.cpp}
    ///     // Execute query with blob set to a string
    ///     q.execute(fb::blob(db).set("new value"));
    /// \endcode
    ///
    /// \tparam T - Unspecified type, make overloads to specify.
    /// \param[in] from - Value to set.
    ///
    /// \return Reference to the current blob object so it can
    ///         be passed inline to sqlda::set method.
    ///
    template <class T>
    blob& set(T from);

    /// Get chunk of data.
    ///
    /// \param[in] buf - Buffer to store the data.
    /// \param[in] buf_length - Length of the buffer.
    ///
    /// \return Number of bytes actually read
    ///         (0 if no more data available).
    ///
    uint16_t read_chunk(char* buf, uint16_t buf_length) const;

    /// Write chunk of data.
    ///
    /// \note Blob must be closed after last chunk of data
    ///       before execution of query.
    ///
    /// \param[in] buf - Buffer containing the data.
    /// \param[in] buf_length - Length of the buffer.
    ///
    /// \see close()
    ///
    void write_chunk(const char* buf, uint16_t buf_length);

    /// Close the blob after the last chunk has been written.
    void close() noexcept
    { _context->close(); }

private:
    /// Blob's internal data
    struct context_t
    {
        /// Create constructor
        context_t(transaction& tr)
        {
            // Start transaction if not already
            tr.start();
            invoke_except(isc_create_blob,
                tr.db().handle(), tr.handle(), &_handle, &_id);
        }

        /// Read constructor
        context_t(transaction& tr, blob_id_t id)
        : _id(id)
        {
            invoke_except(isc_open_blob,
                tr.db().handle(), tr.handle(), &_handle, &id);
        }

        /// Close on destruct
        ~context_t() noexcept
        { close(); }

        /// Close the blob handle.
        void close() noexcept
        { _handle && (invoke_noexcept(isc_close_blob, &_handle), true); }

        isc_blob_handle _handle = 0;
        blob_id_t _id;
    };

    std::shared_ptr<context_t> _context;
};

// Read a chunk of data from the blob.
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

// Write a chunk of data to the blob.
void blob::write_chunk(const char* buf, uint16_t buf_length)
{
    invoke_except(isc_put_segment,
        &_context->_handle, buf_length, const_cast<char*>(buf));
}

/// Set blob from a string.
/// Overload of blob::set() method.
/// Create your own overload for type required.
template <class T>
blob& blob::set(string_like<T> value)
{
    std::string_view str(value);
    write_chunk(str.data(), str.size());
    close();
    return *this;
}

} // namespace fb

/// Write blob as string to a stream.
inline std::ostream& operator<<(std::ostream& os, const fb::blob& b)
{
    // Recommended size for segment is 80
    char buf[80];
    for (uint16_t len; (len = b.read_chunk(buf, sizeof(buf)));)
        os << std::string_view(buf, len);
    return os;
}

