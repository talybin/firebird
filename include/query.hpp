/// \file query.hpp

#pragma once
#include "database.hpp"
#include "sqlda.hpp"
#include "blob.hpp"

namespace fb
{

/// Executes SQL query and retrieves data.
struct query
{
    /// Rows iterator.
    struct iterator;

    /// Construct query for given transaction.
    ///
    /// @param[in] tr - Transaction.
    /// @param[in] sql - SQL query to be executed.
    ///
    query(transaction tr, std::string_view sql) noexcept
    : _context(std::make_shared<context_t>(tr, sql))
    { }

    /// Construct query to be used in default transaction
    /// for given database.
    ///
    /// @param[in] db - Database.
    /// @param[in] sql - SQL query to be executed.
    ///
    query(database db, std::string_view sql) noexcept
    : query(db.default_transaction(), sql)
    { }

    /// Access input parameters for reading and writing.
    /// Should be called before execute() method.
    ///
    /// For the first use of this method, query will try to
    /// detect number of parameters needed. If you know how
    /// many parameters required you can hint the size.
    /// This will prevent reallocation of memory if required
    /// number of parameters must be increased.
    ///
    /// @param[in] hint_size - Number of parameters to reserve
    ///                        (optional, default is 1).
    ///
    /// \return Parameter list.
    /// \throw fb::exception
    ///
    sqlda& params(size_t hint_size = 1);

    /// Access output fields (result of SELECT) for reading
    /// only. Should be called after execute() method.
    ///
    /// \return Field list.
    ///
    const sqlda& fields() const noexcept
    { return _context->_fields; }

    /// Prepare query and buffer for receiving data (if not
    /// prepared yet).
    ///
    /// Normally there is no need to call this method.
    /// It is called by other methods as needed.
    ///
    /// \throw fb::exception
    ///
    void prepare();

    /// Execute query with parameters (optional). If parameters
    /// required, but not given here, previously created
    /// parameters (via params() method) apply. It is also
    /// possible to mix this two methods, see example. Use
    /// fb::skip to set only previously unset ones.
    ///
    /// \code{.cpp}
    ///     fb::query q(db, "select * from sales where a = ? and b = ?);
    ///     q.params()["a"] = 42;
    ///     q.execute(fb::skip, "shipped"); // "a" is already set
    ///     // ... later ...
    ///     q.params()["b"] = "open";
    ///     q.execute(); // using previously set "a" and "b"
    /// \endcode
    ///
    /// @param[in] args - Parameters to pass to this query
    ///                   (optional). Number of arguments
    ///                   must match required number or none.
    ///
    /// \return Reference to this query.
    /// \throw fb::exception
    ///
    template <class... Args>
    query& execute(const Args&... args);

    /// Row begin iterator.
    iterator begin() const noexcept;
    /// Row end iterator.
    iterator end() const noexcept;

    /// Get column names.
    std::vector<std::string_view> column_names() const noexcept;

    /// Iterate result of SELECT query by calling callback for
    /// each row.
    ///
    /// \code{.cpp}
    ///     fb::query q(db, "select * from country");
    ///     q.execute().foreach([](auto country, auto currency)
    ///     {
    ///         std::cout << country.name() << ": "
    ///                   << country.value_or("null") << std::endl;
    ///         std::cout << currency.name() << ": "
    ///                   << currency.value_or("null") << std::endl;
    ///     });
    /// \endcode
    ///
    /// \throw fb::exception
    ///
    template <class F>
    void foreach(F&& cb) const
    {
        context_t* c = _context.get();
        for (; c->_is_data_available; c->fetch())
            c->_fields.visit(std::forward<F>(cb));
    }

private:
    /// Query internal data
    struct context_t
    {
        /// Construct query context.
        context_t(transaction& tr, std::string_view sql) noexcept
        : _trans(tr)
        , _sql(sql)
        // Initially expect to receive 5 columns
        , _fields(5)
        { }

        /// Free query on destruct.
        ~context_t() noexcept
        { close(DSQL_drop); }

        /// Release query handle.
        ///
        /// @param[in] op - Operation:
        ///                 DSQL_close - Close and ready to execute again,
        ///                 DSQL_drop - Release query (can't be used anymore).
        ///
        void close(uint16_t op = DSQL_close) noexcept
        { invoke_noexcept(isc_dsql_free_statement, &_handle, op); }

        /// Fetch next row.
        ///
        /// \return false if no more rows available.
        ///
        bool fetch() noexcept
        {
            _is_data_available =
                invoke_noexcept(isc_dsql_fetch, &_handle, SQL_DIALECT_V6, _fields) == 0;
            // Note! The cursor must be closed before next execution
            return _is_data_available || (close(), false);
        }

        isc_stmt_handle _handle = 0;
        bool _is_prepared = false;
        bool _is_data_available = false;
        transaction _trans;
        std::string _sql;

        /// Input parameters
        sqlda _params;
        /// Output fields
        sqlda _fields;
    };

    std::shared_ptr<context_t> _context;
};

/// Row iterator
struct query::iterator
{
    using iterator_category = std::forward_iterator_tag;
    using value_type = sqlda;
    using pointer = value_type*;
    using reference = value_type&;

    /// Construct empty (end) iterator.
    iterator() noexcept = default;

    iterator(query::context_t* ctx) noexcept
    : _ctx(ctx)
    { }

    reference operator*() const noexcept
    { return _ctx->_fields; }

    pointer operator->() const noexcept
    { return &_ctx->_fields; }

    // Prefix increment (++x)
    iterator& operator++() noexcept
    {
        if (!_ctx->fetch())
            _ctx = nullptr;
        return *this;
    }

    // Postfix increment (x++)
    // TODO: This is broken! How to return current
    //       value and then increment
    iterator operator++(int) noexcept {
        auto cur = _ctx;
        this->operator++();
        return cur;
    }

    bool operator==(const iterator& rhs) const noexcept
    { return _ctx == rhs._ctx; };

    bool operator!=(const iterator& rhs) const noexcept
    { return _ctx != rhs._ctx; };

private:
    query::context_t* _ctx = nullptr;
};

// Row begin iterator.
query::iterator query::begin() const noexcept
{ return _context->_is_data_available ? _context.get() : end(); }

// Row end iterator.
query::iterator query::end() const noexcept
{ return {}; }

// Access input parameters for reading and writing.
sqlda& query::params(size_t hint_size)
{
    context_t* c = _context.get();

    // For first use allocate required number of parameters
    if (!c->_params.get())
    {
        // First we need to call prepare and allocate atleast
        // one parameter to get the rest
        prepare();
        c->_params.reserve(std::max(hint_size, size_t(1)));

        // Prepare input parameters
        invoke_except(isc_dsql_describe_bind, &c->_handle, SQL_DIALECT_V6, c->_params);
        if (c->_params.capacity() < c->_params.size()) {
            c->_params.reserve(c->_params.size());
            // Reread prepared description
            invoke_except(isc_dsql_describe_bind, &c->_handle, SQL_DIALECT_V6, c->_params);
        }
    }
    return c->_params;
}

// Prepare query and buffer for receiving data.
void query::prepare()
{
    context_t* c = _context.get();

    // Run only once
    if (c->_is_prepared)
        return;

    // Start transaction if not already
    c->_trans.start();

    // Allocate handle
    invoke_except(isc_dsql_allocate_statement, c->_trans.db().handle(), &c->_handle);
    // Prepare query
    invoke_except(isc_dsql_prepare,
        c->_trans.handle(), &c->_handle, 0, c->_sql.c_str(), SQL_DIALECT_V6, c->_fields);

    // Prepare output fields
    if (c->_fields.capacity() < c->_fields.size()) {
        c->_fields.reserve(c->_fields.size());
        // Reread prepared description
        invoke_except(isc_dsql_describe, &c->_handle, SQL_DIALECT_V6, c->_fields);
    }
    // Allocate buffer for receiving data
    if (c->_fields.size())
        c->_fields.alloc_data();

    c->_is_prepared = true;
}

// Execute query.
template <class... Args>
query& query::execute(const Args&... args)
{
    context_t* c = _context.get();

    // All queries must be prepared before execution.
    // Note, prepare runs only once.
    prepare();

    // Apply input parameters (if any)
    if constexpr (sizeof...(Args) > 0)
        params(sizeof...(Args)).set(args...);

    // Execute
    invoke_except(isc_dsql_execute,
        c->_trans.handle(), &c->_handle, SQL_DIALECT_V6, c->_params);

    // If there data to be read we need to make sure
    // the first entry is present so iterators may
    // start to read from *begin()
    if (c->_fields.size())
        c->fetch();

    return *this;
}

// Get column names.
std::vector<std::string_view> query::column_names() const noexcept
{
    std::vector<std::string_view> view;
    view.reserve(_context->_fields.size());

    for (auto& var : _context->_fields)
        view.push_back(var.name());
    return view;
}

} // namespace fb

