#pragma once
#include "database.hpp"
#include "sqlda.hpp"

namespace fb
{

struct query
{
    struct iterator;

    query(transaction tr, std::string_view sql)
    : _context(std::make_shared<context_t>(tr, sql))
    { }

    query(database db, std::string_view sql)
    : query(db.default_transaction(), sql)
    { }

    // Input parameters
    sqlda& params();

    // Prepare query and buffer for receiving data
    void prepare();

    // Run query
    template <class... Args>
    query& execute(const Args&... args);

    iterator begin() const;
    iterator end() const;

    // Get column names
    std::vector<std::string_view> column_names() const;

    // Get row values
    template <class F>
    void foreach(F&& cb) const
    {
        context_t* c = _context.get();
        for (; c->_is_data_available; c->fetch())
            c->_fields.visit(std::forward<F>(cb));
    }

private:
    struct context_t
    {
        context_t(transaction& tr, std::string_view sql)
        : _trans(tr)
        , _sql(sql)
        // Initially expect to receive 5 columns
        , _fields(5)
        { }

        // Free query on destruct
        ~context_t()
        { close(DSQL_drop); }

        void close(uint16_t op = DSQL_close)
        { invoke_noexcept(isc_dsql_free_statement, &_handle, op); }

        // Fetch next row
        bool fetch()
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

        sqlda _params;
        sqlda _fields;
    };

    std::shared_ptr<context_t> _context;
};


// query methods

struct query::iterator
{
    using iterator_category = std::forward_iterator_tag;
    using value_type = sqlda;
    using pointer = value_type*;
    using reference = value_type&;

    iterator() = default;

    iterator(query::context_t* ctx)
    : _ctx(ctx)
    { }

    reference operator*() const
    { return _ctx->_fields; }

    pointer operator->() const
    { return &_ctx->_fields; }

    // Prefix increment (++x)
    iterator& operator++()
    {
        if (!_ctx->fetch())
            _ctx = nullptr;
        return *this;
    }

    // Postfix increment (x++)
    // TODO: This is broken! How to return current
    //       value and then increment
    iterator operator++(int) {
        auto cur = _ctx;
        this->operator++();
        return cur;
    }

    bool operator==(const iterator& rhs) const
    { return _ctx == rhs._ctx; };

    bool operator!=(const iterator& rhs) const
    { return _ctx != rhs._ctx; };

private:
    query::context_t* _ctx = nullptr;
};


query::iterator query::begin() const
{ return _context->_is_data_available ? _context.get() : end(); }


query::iterator query::end() const
{ return {}; }


sqlda& query::params()
{
    context_t* c = _context.get();

    // For first use allocate required number of parameters
    if (!c->_params.get())
    {
        // First we need to call prepare and allocate atleast
        // one parameter to get the rest
        prepare();
        c->_params.resize(1);

        // Prepare input parameters
        invoke_except(isc_dsql_describe_bind, &c->_handle, SQL_DIALECT_V6, c->_params);
        if (c->_params.capacity() < c->_params.size()) {
std::cout << "increasing params:\n" << c->_params;
            c->_params.resize(c->_params.size());
            // Reread prepared description
            invoke_except(isc_dsql_describe_bind, &c->_handle, SQL_DIALECT_V6, c->_params);
        }
std::cout << "required params:\n" << c->_params;
    }
    return c->_params;
}


// Prepare query and buffer for receiving data
void query::prepare()
{
    context_t* c = _context.get();

    // Run only once
    if (c->_is_prepared)
        return;
std::cout << "query::prepare()" << std::endl;

    // Start transaction if not already
    c->_trans.start();

    // Allocate handle
    invoke_except(isc_dsql_allocate_statement, c->_trans.db().handle(), &c->_handle);
    // Prepare query
    invoke_except(isc_dsql_prepare,
        c->_trans.handle(), &c->_handle, 0, c->_sql.c_str(), SQL_DIALECT_V6, c->_fields);

    #if 0 // Not needed yet
    // Prepare input parameters
    invoke_except(isc_dsql_describe_bind, &c->_handle, SQL_DIALECT_V6, c->_params);
    if (c->_params.capacity() < c->_params.size()) {
        c->_params.resize(c->_params.size());
        // Reread prepared description
        invoke_except(isc_dsql_describe_bind, &c->_handle, SQL_DIALECT_V6, c->_params);
    }

    std::cout << "required params:\n" << c->_params;
    #endif

    // Prepare output fields
    if (c->_fields.capacity() < c->_fields.size()) {
        c->_fields.resize(c->_fields.size());
        // Reread prepared description
        invoke_except(isc_dsql_describe, &c->_handle, SQL_DIALECT_V6, c->_fields);
    }
    // Allocate buffer for receiving data
    if (c->_fields.size())
        c->_fields.alloc_data();

    std::cout << "out fields:\n" << c->_fields;

    c->_is_prepared = true;
}


// Run query
template <class... Args>
query& query::execute(const Args&... args)
{
    context_t* c = _context.get();

    // All queries must be prepared before execution.
    // Note, prepare runs only once.
    prepare();

    // Apply input parameters (if any)
    c->_params.set(args...);
std::cout << "execute params:\n" << c->_params;

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


// Get column names
std::vector<std::string_view> query::column_names() const
{
    std::vector<std::string_view> view;
    view.reserve(_context->_fields.size());

    for (auto& var : _context->_fields)
        view.emplace_back(var.sqlname, var.sqlname_length);
    return view;
}

} // namespace fb
