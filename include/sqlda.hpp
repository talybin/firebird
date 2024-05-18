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
    using type = XSQLDA;
    using pointer = std::add_pointer_t<type>;
    using iterator = std::add_pointer_t<sqlvar::type>;

    using ptr_t = std::unique_ptr<type, decltype(&free)>;
    using data_buffer_t = std::vector<char>;

    explicit sqlda(size_t nr_cols = 0)
    : _ptr(alloc(nr_cols))
    { }

    operator pointer() const
    { return get(); }

    pointer get() const
    { return _ptr.get(); }

    pointer operator->() const
    { return get(); }

    iterator begin() const
    { return _ptr ? _ptr->sqlvar : nullptr; }

    iterator end() const
    { return _ptr ? &_ptr->sqlvar[std::min(size(), capacity())] : nullptr; }

    // Number of used columns
    size_t size() const
    { return _ptr ? _ptr->sqld : 0; }

    // Number of columns allocated
    size_t capacity() const
    { return _ptr ? _ptr->sqln : 0; }

    void resize(size_t nr_cols)
    { _ptr = alloc(nr_cols); }

    // Access by index (returning sqlvar)
    sqlvar operator[](size_t pos) const
    {
        if (pos >= size())
            throw fb::exception("index out of range, index ") << pos << " >= size " << size();
        return sqlvar(&_ptr->sqlvar[pos]);
    }

    // Allocate aligned space for incoming data
    void alloc_data();

    template <class... Args>
    std::tuple<Args...>& get(std::tuple<Args...>& tup) const
    {
        auto setter = [this]<size_t... I>(auto& t, std::index_sequence<I...>)
        {
            (std::visit(
                [&t](auto val) { try_assign(std::get<I>(t), val); },
                sqlvar(&_ptr->sqlvar[I]).get()), ...);
        };
        setter(tup, std::index_sequence_for<Args...>{});
        return tup;
    }

    /*
    template <template<class...> class T, class... Args>
    std::enable_if_t<is_tuple_v<T<Args...>>, std::tuple<Args...>>
    as_tuple() const
//    std::tuple<Args...> get() const
    {
        return {};
    }*/

    // Set input parameters.
    // Note! This creates a view to arguments and XSQLVARs
    //       are valid as long as args do not expire.
    template <class... Args>
    std::enable_if_t<(sizeof...(Args) > 0)>
    set(const Args&... args)
    {
        constexpr size_t cnt = sizeof...(Args);
        if (size() != cnt)
            throw fb::exception(
                "set: wrong number of parameters (should be ")
                << size() << ", called with " << cnt << ")";

        auto it = begin();
        ((sqlvar(it).set(args), ++it), ...);
    }

    // Set without values, do nothing
    void set() { }

    template <class F>
    void visit(F&& cb) const
    {
        to_const<50>(size(), [&](auto I) {
            visit(cb, std::make_index_sequence<I>{});
        });
    }

    // For debug purpose
    friend std::ostream& operator<<(std::ostream& os, const sqlda&);

private:
    ptr_t _ptr;
    data_buffer_t _data_buffer;

    // Allocate buffer enough to hold given number of columns
    static ptr_t alloc(size_t nr_cols)
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


void sqlda::alloc_data()
{
    // Calculate data size and offsets
    size_t offset = 0;
    for (auto it = begin(); it != end(); ++it)
    {
        short dtype = it->sqltype & ~1;
        size_t size = it->sqllen;

        if (dtype == SQL_VARYING)
            size += sizeof(short);

        it->sqldata = (char*)offset;
        // Align size to sizeof(short)
        offset += (size + 1) & ~1;

        it->sqlind = (short*)offset;
        offset += sizeof(short);
    }

    // Allocate storage
    _data_buffer.reserve(offset);

    // Apply offsets to new storage
    char* buf = _data_buffer.data();
    for (auto it = begin(); it != end(); ++it) {
        it->sqldata = buf + (size_t)it->sqldata;
        it->sqlind = (short*)(buf + (size_t)it->sqlind);
    }
}


std::ostream& operator<<(std::ostream& os, const sqlda& v)
{
    if (v._ptr) {
        os << "sqln (cols allocated): " << v._ptr->sqln << std::endl;
        os << "sqld (cols used): " << v._ptr->sqld << std::endl;

        size_t cnt = 0;
        for (auto it = v.begin(); it != v.end(); ++it, ++cnt) {
            os << "--- sqlvar: " << cnt << " ---" << std::endl;
            os << sqlvar(it);
        }
    }
    else
        os << "sqlda: nullptr" << std::endl;

    os << "------------------------------" << std::endl;
    return os;
}


} // namespace fb

