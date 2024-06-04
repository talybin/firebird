#pragma once
#include <memory>
#include <string_view>

namespace fb
{

struct database
{
    // Database Parameter Buffer
    struct params;

    // Constructor
    database(
        std::string_view path,
        std::string_view user = "sysdba",
        std::string_view passwd = "masterkey") noexcept;

    // Connect. Throw exeption on error.
    void connect();

    // Disconnect from database
    void disconnect() noexcept;

    // Execute query once and discard it (see transaction::execute_immediate)
    template <class... Args>
    void execute_immediate(std::string_view sql, const Args&... params);

    // Create new database. The passed query must begin with
    // CREATE DATABASE ...
    // Example:
    //   fb::database new_db = fb::database::create(
    //      "CREATE DATABASE 'localhost/3053:/firebird/data/mydb.ib' "
    //      "USER 'SYSDBA' PASSWORD 'masterkey' "
    //      "PAGE_SIZE 8192 DEFAULT CHARACTER SET UTF8");
    static database create(std::string_view sql);

    // Native handle
    isc_db_handle* handle() const noexcept;

    // Default transaction used for simple queries. Starts on connect.
    transaction& default_transaction() noexcept;

    // Methods of default transaction
    void commit();
    void rollback();

private:
    struct context_t;
    std::shared_ptr<context_t> _context;
    transaction _trans;

    // Construct database from handle
    database(isc_db_handle) noexcept;
};

} // namespace fb

