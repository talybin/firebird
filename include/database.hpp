#pragma once
#include <memory>
#include <string_view>

namespace fb
{

struct database
{
    /// Database Parameter Buffer (DPB).
    struct params;

    /// Construct database for connection.
    ///
    /// \param[in] path - DSN connection string.
    /// \param[in] user - Username. Optional, default is "sysdba".
    /// \param[in] passwd - Password. Optional, default is "masterkey".
    ///
    database(
        std::string_view path,
        std::string_view user = "sysdba",
        std::string_view passwd = "masterkey") noexcept;

    /// Connect to database using parameters provided in constructor.
    ///
    /// \throw fb::exception
    ///
    void connect();

    /// Disconnect from database.
    void disconnect() noexcept;

    /// Execute query once and discard it. Calls execute_immediate() of
    /// default transaction.
    ///
    /// \param[in] sql - SQL query to execute.
    /// \param[in] params - Parameters to be set in SQL query (optional, if any).
    ///
    /// \throw fb::exception
    /// \see transaction::execute_immediate
    ///
    template <class... Args>
    void execute_immediate(std::string_view sql, const Args&... params);

    /// Create new database.
    ///
    /// \code{.cpp}
    ///     fb::database new_db = fb::database::create(
    ///         "CREATE DATABASE 'localhost/3053:/firebird/data/mydb.ib' "
    ///         "USER 'SYSDBA' PASSWORD 'masterkey' "
    ///         "PAGE_SIZE 8192 DEFAULT CHARACTER SET UTF8");
    /// \endcode
    ///
    /// \param[in] sql - CREATE DATABASE query.
    ///
    /// \return Database object connected to newly created database.
    /// \throw fb::exception
    ///
    static database create(std::string_view sql);

    /// Get native internal handle.
    isc_db_handle* handle() const noexcept;

    /// Default transaction can be used to minimize written code by passing
    /// database object to fb::query directly instead of instantiate new
    /// transaction for each database connection.
    ///
    /// \code{.cpp}
    ///     fb::database db("employee");
    ///     fb::query(db, "select * from country");
    /// \endcode
    ///
    transaction& default_transaction() noexcept;

    /// Commit default transaction.
    ///
    /// \throw fb::exception
    ///
    void commit();

    /// Rollback default transaction.
    ///
    /// \throw fb::exception
    ///
    void rollback();

private:
    struct context_t;
    std::shared_ptr<context_t> _context;

    /// Default transaction.
    transaction _trans;

    /// Construct database from handle.
    database(isc_db_handle) noexcept;
};

} // namespace fb

