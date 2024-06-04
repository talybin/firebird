#pragma once
#include <ibase.h>
#include <memory>
#include <string_view>

namespace fb
{

struct database;

struct transaction
{
    // Construct empty transaction and attach database later
    transaction() noexcept = default;

    // Construct with attached database
    transaction(database& db) noexcept;

    // Start transaction if not started
    void start();

    // Commit pending queries
    void commit();

    // Cancel pending queries
    void rollback();

    // execute_immediate prepares the DSQL statement, executes
    // it once, and discards it. The statement must not be one
    // that returns data (that is, it must not be a SELECT or
    // EXECUTE PROCEDURE statement, use fb::query for this).
    template <class... Args>
    void execute_immediate(std::string_view sql, const Args&... params);

    // Internal pointer to isc_tr_handle
    isc_tr_handle* handle() const noexcept;

    // Database for this transaction
    database& db() const noexcept;

private:
    struct context_t;
    std::shared_ptr<context_t> _context;
};

} // namespace fb

