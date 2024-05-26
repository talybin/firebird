#pragma once
#include "sqlvar.hpp"
#include "exception.hpp"
#include "concepts.hpp"

#include <memory>
#include <cstdlib>
#include <cassert>
#include <vector>

namespace fb
{

// XSQLDA is a host-language data structure that DSQL uses to transport
// data to or from a database when processing a SQL statement string.
// See https://docwiki.embarcadero.com/InterBase/2020/en/XSQLDA_Field_Descriptions_(Embedded_SQL_Guide)
struct sqlda
{
    struct iterator;

    using type = XSQLDA;
    using pointer = std::add_pointer_t<type>;

    using ptr_t = std::unique_ptr<type, decltype(&free)>;
    using data_buffer_t = std::vector<char>;

    explicit sqlda(size_t nr_cols = 0) noexcept
    : _ptr(alloc(nr_cols))
    { }

    operator pointer() const noexcept
    { return get(); }

    pointer get() const noexcept
    { return _ptr.get(); }

    pointer operator->() const noexcept
    { return get(); }

    iterator begin() const noexcept;
    iterator end() const noexcept;

    // Number of used columns
    size_t size() const noexcept
    { return _ptr ? _ptr->sqld : 0; }

    // Number of columns allocated
    size_t capacity() const noexcept
    { return _ptr ? _ptr->sqln : 0; }

    // Resize to given number of columns/params
    void resize(size_t nr_cols) noexcept
    { _ptr = alloc(nr_cols); }

    // Access by index (with check for out of range)
    template <class T>
    sqlvar at(index_castable<T> pos) const 
    {
        // We need static_cast here in case T is an enum
        size_t index = static_cast<size_t>(pos);
        if (index >= size())
            throw fb::exception("index out of range, index ") << index << " >= size " << size();
        return this->operator[](index);
    }

    // Access by column name (a bit slower than by index)
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

    // Access by index (without check for out of range)
    template <class T>
    sqlvar operator[](index_castable<T> pos) const noexcept
    {
        assert(static_cast<size_t>(pos) < size());
        return sqlvar(&_ptr->sqlvar[static_cast<size_t>(pos)]);
    }

    // Access by column name
    sqlvar operator[](std::string_view pos) const
    { return at(pos); }

    // Allocate aligned space for incoming data
    void alloc_data() noexcept;

    // Return tuple
    template <auto... I>
    auto as_tuple() const noexcept
    { return std::make_tuple(this->operator[](I)...); }

    // Set input parameters.
    // Note! This creates a view to arguments and XSQLVARs
    //       are valid as long as args do not expire.
    template <class... Args>
    std::enable_if_t<(sizeof...(Args) > 0)>
    set(const Args&... args);

    // Set without values, do nothing
    void set() noexcept { }

    // Visit columns/params where
    // F is visitor accepting types of field_t
    template <class F>
    void visit(F&& cb) const;

private:
    ptr_t _ptr;
    data_buffer_t _data_buffer;

    // Allocate buffer enough to hold given number of columns
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


// Iterator for column traversing
struct sqlda::iterator
{
    using value_type = sqlvar;
    using pointer = value_type*;
    using reference = value_type&;

    iterator(value_type::pointer var) noexcept
    : _var(var)
    { }

    reference operator*() noexcept
    { return _var; }

    pointer operator->() noexcept
    { return &_var; }

    // Prefix increment (++x)
    iterator& operator++() noexcept {
        _var = value_type(_var.handle() + 1);
        return *this;
    }

    // Postfix increment (x++)
    iterator operator++(int) noexcept {
        iterator cur(_var.handle());
        this->operator++();
        return cur;
    }

    // Prefix decrement (--x)
    iterator& operator--() noexcept {
        _var = value_type(_var.handle() - 1);
        return *this;
    }

    // Postfix decrement (x--)
    iterator operator--(int) noexcept {
        iterator cur(_var.handle());
        this->operator--();
        return cur;
    }

    bool operator==(const iterator& rhs) const noexcept
    { return _var.handle() == rhs._var.handle(); };

    bool operator!=(const iterator& rhs) const noexcept
    { return _var.handle() != rhs._var.handle(); };

private:
    value_type _var;
};


sqlda::iterator sqlda::begin() const noexcept
{ return _ptr ? _ptr->sqlvar : nullptr; }


sqlda::iterator sqlda::end() const noexcept
{ return _ptr ? &_ptr->sqlvar[std::min(size(), capacity())] : nullptr; }


// Set input parameters.
// Note! This creates a view to arguments and XSQLVARs
//       are valid as long as args do not expire.
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


// Visit columns/params where
// F is visitor accepting types of field_t
template <class F>
void sqlda::visit(F&& cb) const
{
    auto visit_helper = [&]<size_t... I>(std::index_sequence<I...>)
    {
        // Testing this way in case F's arguments are optional or just "auto..."
        if constexpr(std::is_invocable_v<F, detail::index_type<I, sqlvar>...>)
            cb(sqlvar(&_ptr->sqlvar[I])...);
        else
            throw fb::exception("visit: wrong number of arguments, expecting: ") << sizeof...(I);
    };
    // Max supported fields is 50
    bool fits_in = detail::to_const<50>(size(), [&visit_helper](auto I) {
        visit_helper(std::make_index_sequence<I>{});
    });
    if (!fits_in)
        throw fb::exception("visit: not enough supported fields");
}


// Allocate aligned space for incoming data
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

} // namespace fb

