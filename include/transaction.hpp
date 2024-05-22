#pragma once

namespace fb
{

struct database;

struct transaction
{
    transaction(database& db);

    // Start transaction if not started
    void start();

    void commit();

    void rollback();

    // Internal pointer to isc_tr_handle
    isc_tr_handle* handle() noexcept;

    // Database for this transaction
    database db() noexcept;

private:
    struct context_t;
    std::shared_ptr<context_t> _context;
};

} // namespace fb

