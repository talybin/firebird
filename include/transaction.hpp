#pragma once
#include <ibase.h>
#include <memory>
#include <string_view>

namespace fb
{

struct database;

struct transaction
{
    /// Construct empty transaction and attach database later.
    /// Should not be used directly. Used by database to create
    /// default transaction.
    ///
    transaction() noexcept = default;

    /// Construct and attach database object.
    ///
    /// @param[in] db - Reference to database object.
    ///
    transaction(database& db) noexcept;

    /// Start transaction (if not started yet).
    ///
    /// \note Normally there is no need to call this method. Most
    ///       of the functions in this library will automatically
    ///       start attached transaction when necessary.
    ///
    /// \throw fb::exception
    ///
    void start();

    /// Commit (apply) pending changes.
    ///
    /// \throw fb::exception
    ///
    void commit();

    /// Rollback (cancel) pending changes.
    ///
    /// \throw fb::exception
    ///
    void rollback();

    /// Prepares the DSQL statement, executes it once, and
    /// discards it. The statement must not be one that
    /// returns data (that is, it must not be a SELECT or
    /// EXECUTE PROCEDURE statement, use fb::query for this).
    ///
    /// @param[in] sql - SQL query to execute.
    /// @param[in] params - Parameters to be set in SQL query (optional, if any).
    ///
    /// \throw fb::exception
    ///
    template <class... Args>
    void execute_immediate(std::string_view sql, const Args&... params);

    /// Get internal pointer to isc_tr_handle.
    isc_tr_handle* handle() const noexcept;

    /// Get database attached to this transaction.
    database& db() const noexcept;

private:
    struct context_t;
    std::shared_ptr<context_t> _context;
};

} // namespace fb

