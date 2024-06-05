/// This file contains the definition of the sqlda structure used
/// for transporting data in SQL statements.

#pragma once
#include "sqlvar.hpp"
#include "exception.hpp"

#include <memory>
#include <cstdlib>
#include <cassert>
#include <vector>

namespace fb
{

/// XSQLDA is a host-language data structure that DSQL uses to transport
/// data to or from a database when processing a SQL statement string.
///
/// \see https://docwiki.embarcadero.com/InterBase/2020/en/XSQLDA_Field_Descriptions_(Embedded_SQL_Guide)
struct sqlda
{
    /// Column iterator.
    struct iterator;

    using type = XSQLDA;
    using pointer = std::add_pointer_t<type>;

    using ptr_t = std::unique_ptr<type, decltype(&free)>;
    using data_buffer_t = std::vector<char>;

    /// Constructs an sqlda object with a specified number of columns.
    ///
    /// \param[in] nr_cols - Number of columns to allocate (optional,
    ///                      default is 0). Zero number of columns
    ///                      will not allocate any memory.
    ///
    explicit sqlda(size_t nr_cols = 0) noexcept
    : _ptr(alloc(nr_cols))
    { }

    /// Conversion operator to pointer.
    ///
    /// \return Pointer to XSQLDA type.
    /// \see get()
    ///
    operator pointer() const noexcept
    { return get(); }

    /// Gets the raw pointer to XSQLDA.
    ///
    /// \return Pointer to XSQLDA type.
    ///
    pointer get() const noexcept
    { return _ptr.get(); }

    /// XSQLDA member access operator.
    ///
    /// \return Pointer to XSQLDA type.
    ///
    pointer operator->() const noexcept
    { return get(); }

    /// Column iterator
    iterator begin() const noexcept;

    /// Column iterator
    iterator end() const noexcept;

    /// Gets the number of used columns.
    ///
    /// \return Number of columns currently in use.
    ///
    size_t size() const noexcept
    { return _ptr ? _ptr->sqld : 0; }

    /// Changes the number of columns stored. If capacity
    /// is below required number, it will be increased.
    ///
    /// \param[in] nr_cols - Number of columns to set.
    ///
    void resize(size_t nr_cols) noexcept
    {
        assert(nr_cols > 0);
        if (nr_cols > capacity())
            reserve(nr_cols);
        if (_ptr)
            _ptr->sqld = nr_cols;
    }

    /// Gets the number of columns allocated.
    ///
    /// \return Number of columns allocated.
    ///
    size_t capacity() const noexcept
    { return _ptr ? _ptr->sqln : 0; }

    /// Resizes capacity to the given number of columns.
    ///
    /// \param[in] nr_cols - Number of columns to allocate.
    ///
    void reserve(size_t nr_cols) noexcept
    { _ptr = alloc(nr_cols); }

    /// Access column by index (with check for out of range).
    ///
    /// \tparam T - Type of index (integral type or enum).
    /// \param[in] pos - Position to access.
    ///
    /// \return sqlvar object at the specified position.
    /// \throw fb::exception
    ///
    template <class T>
    sqlvar at(index_castable<T> pos) const 
    {
        // We need static_cast here in case T is an enum
        size_t index = static_cast<size_t>(pos);
        if (index >= size())
            throw fb::exception("index out of range, index ") << index << " >= size " << size();
        return this->operator[](index);
    }

    /// Access by column name (a bit slower than by index).
    ///
    /// \param[in] pos - Column name.
    ///
    /// \return sqlvar object for the specified column name.
    /// \throw fb::exception
    ///
    sqlvar at(std::string_view pos) const
    {
        auto end = &_ptr->sqlvar[size()];
        for (auto it = _ptr->sqlvar; it != end; ++it) {
            sqlvar v(it);
            if (v.name() == pos)
                return v;
        }
        throw fb::exception() << std::quoted(pos) << " not found";
    }

    /// Access column by index without range check.
    ///
    /// \tparam T - Type of index (integral type or enum).
    /// \param[in] pos - Position to access.
    ///
    /// \return sqlvar object at the specified position.
    ///
    template <class T>
    sqlvar operator[](index_castable<T> pos) const noexcept
    {
        assert(static_cast<size_t>(pos) < size());
        return sqlvar(&_ptr->sqlvar[static_cast<size_t>(pos)]);
    }

    /// Access by column name.
    ///
    /// \param[in] pos - Column name.
    ///
    /// \return sqlvar object for the specified column name.
    /// \throw fb::exception
    ///
    sqlvar operator[](std::string_view pos) const
    { return at(pos); }

    /// Allocate aligned space for incoming data.
    void alloc_data() noexcept;

    /// Construct a tuple of specified indexes.
    ///
    /// \code{.cpp}
    ///     // Get columns 0, 7 and 8
    ///     auto tup = row.as_tuple<0, 7, 8>();
    /// \endcode
    ///
    /// \tparam I... - Indexes.
    ///
    /// \return Tuple containing sqlvar objects.
    ///
    template <auto... I>
    auto as_tuple() const noexcept
    { return std::make_tuple(this->operator[](I)...); }

    /// Sets input parameters.
    ///
    /// \note This creates a view to arguments and XSQLVARs
    ///       are valid as long as args do not expire.
    ///
    /// \tparam Args... - Value types.
    /// \param[in] args... - Any values (numeric, strings, ...).
    ///
    /// \throw fb::exception
    /// \see query::execute()
    ///
    template <class... Args>
    std::enable_if_t<(sizeof...(Args) > 0)>
    set(const Args&... args);

    /// Dummy method that does nothing.
    void set() noexcept { }

    /// Visits columns with a visitor function. Argument
    /// type is sqlvar.
    ///
    /// \code{.cpp}
    ///     // Visit max 15 fields instead of default 10
    ///     row.visit<15>([](auto... columns) {
    ///         std::cout << "row has " << sizeof...(columns) << " columns\n";
    ///     });
    /// \endcode
    ///
    /// \tparam MAX_FIELDS - Maximum number of fields that
    ///                      can be visited. Increase this
    ///                      number if more required.
    ///                      Default is 10 fields.
    /// \tparam F - Visitor function type.
    /// \param[in] cb - Visitor function.
    ///
    /// \return Result of the visitor function.
    /// \throw fb::exception
    ///
    template <size_t MAX_FIELDS = 10, class F>
    constexpr decltype(auto)
    visit(F&& cb) const;

private:
    ptr_t _ptr;
    data_buffer_t _data_buffer;

    // Visit details
    template <class F, class> struct visitor_impl;
    template <class F, size_t N>
    using visitor = visitor_impl<F, std::make_index_sequence<N>>;

    /// Allocate and initializae buffer enough to hold given
    /// number of columns.
    ///
    /// \return nullptr if nr_cols is zero.
    ///
    static ptr_t alloc(size_t nr_cols) noexcept
    {
        pointer ptr = nullptr;
        if (nr_cols) {
            ptr = pointer(calloc(1, XSQLDA_LENGTH(nr_cols)));
            ptr->version = 1;
            ptr->sqln = nr_cols;
        }
        return { ptr, &free };
    }
};

/// Iterator for column traversing
struct sqlda::iterator
{
    /// Iterator value type.
    using value_type = sqlvar;
    /// Alias for pointer to sqlvar.
    using pointer = value_type*;
    /// Alias for reference to sqlvar.
    using reference = value_type&;

    /// Constructs an iterator from given sqlvar pointer.
    iterator(value_type::pointer var) noexcept
    : _var(var)
    { }

    /// Dereference operator.
    reference operator*() noexcept
    { return _var; }

    /// Member access operator.
    pointer operator->() noexcept
    { return &_var; }

    /// Prefix increment (++x)
    iterator& operator++() noexcept {
        _var = value_type(_var.handle() + 1);
        return *this;
    }

    /// Postfix increment (x++)
    iterator operator++(int) noexcept {
        iterator cur(_var.handle());
        this->operator++();
        return cur;
    }

    /// Prefix decrement (--x)
    iterator& operator--() noexcept {
        _var = value_type(_var.handle() - 1);
        return *this;
    }

    /// Postfix decrement (x--)
    iterator operator--(int) noexcept {
        iterator cur(_var.handle());
        this->operator--();
        return cur;
    }

    /// Equality operator.
    bool operator==(const iterator& rhs) const noexcept
    { return _var.handle() == rhs._var.handle(); };

    /// Inequality operator.
    bool operator!=(const iterator& rhs) const noexcept
    { return _var.handle() != rhs._var.handle(); };

private:
    /// sqlvar object for the iterator.
    value_type _var;
};

/// Returns an iterator to the first column.
sqlda::iterator sqlda::begin() const noexcept
{ return _ptr ? _ptr->sqlvar : nullptr; }

/// Returns an iterator to the end.
sqlda::iterator sqlda::end() const noexcept
{ return _ptr ? &_ptr->sqlvar[std::min(size(), capacity())] : nullptr; }

/// Sets input parameters.
template <class... Args>
std::enable_if_t<(sizeof...(Args) > 0)>
sqlda::set(const Args&... args)
{
    constexpr size_t cnt = sizeof...(Args);
    if (size() != cnt)
        throw fb::exception(
            "set: wrong number of parameters (should be ")
            << size() << ", called with " << cnt << ")";

    auto it = begin();
    ((it->set(args), ++it), ...);
}

/// Allocates aligned space for incoming data.
void sqlda::alloc_data() noexcept
{
    // Calculate data size and offsets
    size_t offset = 0;
    for (auto it = begin(); it != end(); ++it)
    {
        auto p = it->handle();
        size_t len = p->sqllen;
        short dtype = p->sqltype & ~1;

        if (dtype == SQL_VARYING)
            len += sizeof(short);

        p->sqldata = (char*)offset;
        // Align size to sizeof(short)
        offset += (len + 1) & ~1;

        p->sqlind = (short*)offset;
        offset += sizeof(short);
    }

    // Allocate storage
    _data_buffer.reserve(offset);

    // Apply offsets to new storage
    char* buf = _data_buffer.data();
    for (auto it = begin(); it != end(); ++it)
    {
        auto p = it->handle();
        p->sqldata = buf + (size_t)p->sqldata;
        p->sqlind = (short*)(buf + (size_t)p->sqlind);
    }
}

/// Visit details
template <class F, size_t... I>
struct sqlda::visitor_impl<F, std::index_sequence<I...>>
{
    using ret_type = detected_t<
        std::invoke_result_t, F, index_type<I, sqlvar>...
    >;

    template <class R>
    static constexpr R
    visit(F&& f, sqlvar::pointer ptr)
    {
        if constexpr(std::is_invocable_v<F, index_type<I, sqlvar>...>)
            return std::forward<F>(f)(sqlvar(&ptr[I])...);
        else
            throw fb::exception("wrong number of arguments: ") << sizeof...(I);
    }
};

/// Visit columns.
template <size_t MAX_FIELDS, class F>
constexpr decltype(auto)
sqlda::visit(F&& cb) const
{
    // Default return type of detected traits
    using not_found = any_type;

    return []<size_t... I>(
        size_t index, F&& f, sqlvar::pointer ptr, std::index_sequence<I...>)
        -> decltype(auto)
    {
        // Get the return type of given callback function
        using detected_ret = detected_or<
            not_found,
            std::common_type_t, typename sqlda::visitor<F, I>::ret_type...
        >;
        static_assert(typename detected_ret::value_t(),
            "visit requires the visitor to have the same return type "
            "for all number of arguments");

        using R = typename detected_ret::type;
        static_assert(
            // Note, this does not detect variadic number of arguments
            !std::is_same_v<R, not_found>,
            "too many fields to visit, increase number of max_fields");

        // Build a virtual function table
        constexpr R (*vtable[])(F&&, sqlvar::pointer) = {
            &sqlda::visitor<F, I>::template visit<R>...
        };
        // Select the one that match number of fields (arguments)
        // or less if index is out of range (call only first few)
        return vtable[std::min(index, MAX_FIELDS)](std::forward<F>(f), ptr);
    }
    (size(), std::forward<F>(cb), _ptr->sqlvar,
        // Including callback with zero arguments (sequence for 0..(MAX_FIELDS + 1))
        std::make_index_sequence<MAX_FIELDS + 1>{});
}

} // namespace fb

