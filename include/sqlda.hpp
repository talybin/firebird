#pragma once
#include "sqlvar.hpp"
#include "exception.hpp"

#include <memory>
#include <cstdlib>
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

    explicit sqlda(size_t nr_cols = 0)
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

    void resize(size_t nr_cols) noexcept
    { _ptr = alloc(nr_cols); }

    // Access by index (returning sqlvar)
    sqlvar operator[](size_t pos) const
    {
        if (pos >= size())
            throw fb::exception("index out of range, index ") << pos << " >= size " << size();
        return sqlvar(&_ptr->sqlvar[pos]);
    }

    // Allocate aligned space for incoming data
    void alloc_data() noexcept;

    // std::tuple<...> tup
    // get(tup, std::index_sequence<0, 2, 4>{})
    template <class... Args, size_t... I>
    std::tuple<Args...>&
    get(std::tuple<Args...>& t, std::index_sequence<I...>) const
    {
        ((std::get<I>(t) = sqlvar(&_ptr->sqlvar[I]).get<Args>()), ...);
        return t;
    }

    // std::tuple<...> tup
    // get(tup)
    template <class... Args>
    std::tuple<Args...>& get(std::tuple<Args...>& tup) const
    { return get(tup, std::index_sequence_for<Args...>{}); }

    // Build a reduced tuple of selected indexes.
    // Ex:
    //   using from_t = std::tuple<int, std::string, float>;
    //   std::tuple<int, float> t = select<from_t, 0, 2>();
    template <class T, size_t... I, class = std::enable_if_t<is_tuple_v<T>> >
    auto select() const
    {
        return std::make_tuple(
            sqlvar(&_ptr->sqlvar[I]).get<std::tuple_element_t<I, T>>()...);
    }

    // get<std::tuple<...>>()
    template <class T>
    std::enable_if_t<is_tuple_v<T>, T>
    get() const
    {
        T tup;
        return get(tup);
    }

    // Set input parameters.
    // Note! This creates a view to arguments and XSQLVARs
    //       are valid as long as args do not expire.
    template <class... Args>
    std::enable_if_t<(sizeof...(Args) > 0)>
    set(const Args&... args);

    // Set without values, do nothing
    void set() noexcept { }

    template <class F>
    void visit(F&& cb) const
    {
        to_const<50>(size(), [&](auto I) {
            visit(cb, std::make_index_sequence<I>{});
        });
    }

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

    // Dummy index expansion of field_t
    template <size_t>
    using field_index_t = field_t;

    // Call callback with expanded column values
    template <class F, size_t... I>
    auto visit(F&& cb, std::index_sequence<I...>) const -> decltype(std::visit(cb, field_index_t<I>()...))
    { std::visit(std::forward<F>(cb), sqlvar(&_ptr->sqlvar[I]).get()...); }

    // Take care of callbacks with wrong number of arguments (nr args != nr cols)
    void visit(...) const
    { throw fb::exception("visit: wrong number of arguments"); }
};


struct sqlda::iterator
{
    using value_type = sqlvar;
    using pointer = value_type*;
    using reference = value_type&;

    iterator(value_type::pointer var)
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

