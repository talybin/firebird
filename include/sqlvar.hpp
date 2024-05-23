#pragma once
#include "types.hpp"
#include "exception.hpp"

#include <cstdlib>
#include <stdexcept>


namespace fb
{

struct sqlvar;

template <class T>
struct sql_value
{
    T get(const sqlvar* v) const;
};

// This is only a view of XSQLVAR. Be carefull with expired values.
// See https://docwiki.embarcadero.com/InterBase/2020/en/XSQLVAR_Field_Descriptions
struct sqlvar
{
    using type = XSQLVAR;
    using pointer = std::add_pointer_t<type>;

    explicit sqlvar(pointer var) : _ptr(var) { }
/*

    // Set methods

    // Generic set method for sqltype, data, and length
    void set(int stype, const void* sdata, size_t slen) noexcept
    {
        _ptr->sqltype = stype;
        _ptr->sqldata = const_cast<char*>(reinterpret_cast<const char*>(sdata));
        _ptr->sqllen = slen;
    }

    // Skip method, does nothing
    void set(skip_t) noexcept { }

    void set(const float& val) noexcept
    { set(SQL_FLOAT, &val, sizeof(float)); }

    void set(const double& val) noexcept
    { set(SQL_DOUBLE, &val, sizeof(double)); }

    void set(const int16_t& val) noexcept
    { set(SQL_SHORT, &val, 2); }

    void set(const int32_t& val) noexcept
    { set(SQL_LONG, &val, 4); }

    void set(const int64_t& val) noexcept
    { set(SQL_INT64, &val, 8); }

    void set(std::string_view val) noexcept
    { set(SQL_TEXT, val.data(), val.size()); }

    void set(const timestamp_t& val) noexcept
    { set(SQL_TIMESTAMP, &val, sizeof(timestamp_t)); }

    void set(std::nullptr_t) noexcept
    { _ptr->sqltype = SQL_NULL; }

    // Set by assigning
    template <class T>
    auto operator=(const T& val) noexcept -> decltype(set(val), *this)
    { return (set(val), *this); }

    // Get methods

    // Get the value as a variant (field_t)
    field_t get() const;

    // Get the value as a specific type
    template <class T>
    T get() const
    {
        return std::visit(detail::overloaded {
            type_converter<T>{}, [](...) -> T
            { throw fb::exception("can't convert to type ") << detail::type_name<T>(); }
        }, get());
    }

    // Return by assigning
    template <class T>
    operator T() const
    { return get<T>(); }
*/

    // Return the internal pointer to XSQLVAR
    const pointer handle() const noexcept
    { return _ptr; }

    pointer handle() noexcept
    { return _ptr; }

    // Get column name
    std::string_view name() const noexcept
    { return std::string_view(_ptr->sqlname, _ptr->sqlname_length); }

    // Get table name
    std::string_view table() const noexcept
    { return std::string_view(_ptr->relname, _ptr->relname_length); }

    uint16_t sql_datatype() const noexcept
    { return _ptr->sqltype & ~1; }

    // This will show actual max column size before set() destroys it
    size_t size() const noexcept
    { return _ptr->sqllen; }

    bool is_null() const noexcept
    { return (_ptr->sqltype & 1) && (*_ptr->sqlind < 0); }

    operator bool() const noexcept
    { return !is_null(); }


    template <class T>
    T value() const
    {
        if (is_null())
            throw fb::exception("type is null");
        return get_value<T>();
    }

    template <class T>
    T value_or(T default_value) const
    {
        if (is_null())
            return default_value;
        return get_value<T>();
    }

private:
    // Internal pointer to XSQLVAR
    pointer _ptr;

    template <class T>
    T get_value() const
    {
        return std::visit(detail::overloaded {
            type_converter<T>{},
            [](...) -> T {
                throw fb::exception("can't convert to type ") << detail::type_name<T>();
            }
        }, get());
    }

    field_t get() const;
};


// Get value variant (field_t) based on the type of SQL data
field_t sqlvar::get() const
{
    if (is_null())
        return field_t(std::in_place_type<std::nullptr_t>, nullptr);

    // Get data type without null flag
    short dtype = _ptr->sqltype & ~1;
    char* data = _ptr->sqldata;

    switch (dtype) {

    case SQL_TEXT:
        return field_t(std::in_place_type<std::string_view>, data, _ptr->sqllen);

    case SQL_VARYING:
    {
        // While sqllen set to max column size, vary_length
        // is actual length of this field
        auto pv = reinterpret_cast<PARAMVARY*>(data);
        return field_t(
            std::in_place_type<std::string_view>,
            (const char*)pv->vary_string, pv->vary_length);
    }

    case SQL_SHORT:
    #if 1
        return field_t(
            std::in_place_type<scaled_integer<int16_t>>,
            *reinterpret_cast<int16_t*>(data), _ptr->sqlscale);
    #else
    {
        int16_t val = *reinterpret_cast<int16_t*>(data);
        if (_ptr->sqlscale < 0)
            return field_t(std::in_place_type<float>, float(val) / scale_tens());
        else if (_ptr->sqlscale)
            return field_t(std::in_place_type<int32_t>, int32_t(val) * scale_tens());
        else
            return field_t(std::in_place_type<int16_t>, val);
    }
    #endif

    case SQL_LONG:
    #if 1
        return field_t(
            std::in_place_type<scaled_integer<int32_t>>,
            *reinterpret_cast<int32_t*>(data), _ptr->sqlscale);
    #else
    {
        int32_t val = *reinterpret_cast<int32_t*>(data);
        if (_ptr->sqlscale < 0)
            return field_t(std::in_place_type<float>, float(val) / scale_tens());
        else if (_ptr->sqlscale)
            return field_t(std::in_place_type<int64_t>, int64_t(val) * scale_tens());
        else
            return field_t(std::in_place_type<int32_t>, val);
    }
    #endif

    case SQL_INT64:
    #if 1
        return field_t(
            std::in_place_type<scaled_integer<int64_t>>,
            *reinterpret_cast<int64_t*>(data), _ptr->sqlscale);
    #else
    {
        int64_t val = *reinterpret_cast<int64_t*>(data);
        if (_ptr->sqlscale < 0)
            return field_t(std::in_place_type<double>, double(val) / scale_tens());
        else if (_ptr->sqlscale)
            return field_t(std::in_place_type<double>, double(val) * scale_tens());
        else
            return field_t(std::in_place_type<int64_t>, val);
    }
    #endif

    case SQL_FLOAT:
        return field_t(std::in_place_type<float>, *reinterpret_cast<float*>(data));

    case SQL_DOUBLE:
        return field_t(std::in_place_type<double>, *reinterpret_cast<double*>(data));

    case SQL_TIMESTAMP:
        return field_t(std::in_place_type<timestamp_t>, *reinterpret_cast<timestamp_t*>(data));

    case SQL_TYPE_DATE:
    {
        timestamp_t t;
        t.timestamp_date = *reinterpret_cast<ISC_DATE*>(data);
        return field_t(std::in_place_type<timestamp_t>, t);
    }

    case SQL_TYPE_TIME:
    {
        timestamp_t t;
        t.timestamp_time = *reinterpret_cast<ISC_TIME*>(data);
        return field_t(std::in_place_type<timestamp_t>, t);
    }

    case SQL_BLOB:
    case SQL_ARRAY:
        return field_t(std::in_place_type<blob_id_t>, *reinterpret_cast<ISC_QUAD*>(data));

    default:
        throw fb::exception("type (") << dtype << ") not implemented";
    }
}


} // namespace fb

