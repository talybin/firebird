/// \file database.hpp

#pragma once
#include <memory>
#include <string_view>
#include <variant>
#include <vector>

namespace fb
{

struct database
{
    /// Database connection parameter.
    /// A set of these parameters build a Database Parameter Buffer (DPB)
    /// that passed in connection request.
    ///
    /// \see https://docwiki.embarcadero.com/InterBase/2020/en/DPB_Parameters
    ///
    struct param
    {
        /// Type that do not hold any value.
        struct none_t { };

        /// Union of parameter value types.
        using value_type = std::variant<
            none_t,
            std::string_view,
            int
        >;

        /// Construct parameter.
        ///
        /// \param[in] name - Parameter name.
        /// \param[in] value - Parameter value. Optional, default is none.
        ///
        param(int name, value_type value = none_t{}) noexcept
        : _name(name)
        , _value(std::move(value))
        { }

        /// Pack parameter into DPB structure.
        ///
        /// \param[in] dpb - Vector to hold DPB data.
        ///
        void pack(std::vector<char>& dpb) const noexcept;

    private:
        /// Parameter name
        int _name;
        /// Parameter value (if any)
        value_type _value;
    };

    /// Construct database with connection parameters.
    ///
    /// \param[in] path - DSN connection string.
    /// \param[in] params - List of parameters.
    ///
    database(std::string_view path,
        std::initializer_list<param> params) noexcept;

    /// Construct database for connection with
    /// username and password.
    ///
    /// \param[in] path - DSN connection string.
    /// \param[in] user_name - User name, optional, default is "sysdba".
    /// \param[in] passwd - Password, optional, default is "masterkey".
    ///
    database(
        std::string_view path,
        std::string_view user_name = "sysdba",
        std::string_view passwd = "masterkey"
    ) noexcept
    : database(path, {
        { isc_dpb_user_name, user_name },
        { isc_dpb_password, passwd },
    })
    { }

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
    /// \return Transaction object.
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

