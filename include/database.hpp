#pragma once
#include <memory>
#include <string_view>
#include <optional>

namespace fb
{

struct database
{
    // Database Parameter Buffer
    struct params;

    // Constructor
    database(std::string_view path,
        std::string_view user = "sysdba", std::string_view passwd = "masterkey") noexcept;

    // Connect. Throw exeption on error.
    void connect();

    // Disconnect from database
    void disconnect() noexcept;

    // Raw handle
    isc_db_handle* handle() const noexcept;

    // Default transaction used for simple queries. Starts on connect.
    transaction& default_transaction() noexcept;

    // Methods of default transaction
    void commit();
    void rollback();

private:
    struct context_t;
    std::shared_ptr<context_t> _context;
    std::optional<transaction> _trans;
};

} // namespace fb

