#pragma once
#include "types.hpp"
#include "exception.hpp"

#include <cstdlib>
#include <stdexcept>

namespace fb
{

// This is only a view of XSQLVAR. Be carefull with expired values.
// See https://docwiki.embarcadero.com/InterBase/2020/en/XSQLVAR_Field_Descriptions
struct sqlvar
{
    using type = XSQLVAR;
    using pointer = std::add_pointer_t<type>;

    explicit sqlvar(pointer var) noexcept
    : _ptr(var) { }

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

    void set(const blob_id_t& val) noexcept
    { set(SQL_BLOB, &val, sizeof(blob_id_t)); }

    void set(std::nullptr_t) noexcept
    { _ptr->sqltype = SQL_NULL; }

    // Set by assigning
    template <class T>
    auto operator=(const T& val) noexcept -> decltype(set(val), *this)
    { return (set(val), *this); }

    // Get methods

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

    // Check for null
    operator bool() const noexcept
    { return !is_null(); }

    // Return by assigning
    template <class T>
    operator T() const
    { return value<T>(); }

    // Get value. Throws if value is null.
    template <class T>
    T value() const
    {
        if (is_null())
            throw fb::exception("type is null");
        return visit<T>();
    }

    // Get value or default value if null
    template <class T>
    T value_or(T default_value) const
    {
        if (is_null())
            return std::move(default_value);
        return visit<T>();
    }

    // Get the value as a variant (field_t)
    field_t as_variant() const;

private:
    // Internal pointer to XSQLVAR
    pointer _ptr;

    template <class T>
    T visit() const
    {
        return std::visit(overloaded {
            type_converter<T>{},
            [](...) -> T {
                throw fb::exception("can't convert to type ") << type_name<T>();
            }
        }, as_variant());
    }
};


// Get value variant (field_t) based on the type of SQL data
field_t sqlvar::as_variant() const
{
    if (is_null())
        return field_t(std::in_place_type<std::nullptr_t>, nullptr);

    // Get data type without null flag
    short dtype = sql_datatype();
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
        return field_t(
            std::in_place_type<scaled_integer<int16_t>>,
            *reinterpret_cast<int16_t*>(data), _ptr->sqlscale);

    case SQL_LONG:
        return field_t(
            std::in_place_type<scaled_integer<int32_t>>,
            *reinterpret_cast<int32_t*>(data), _ptr->sqlscale);

    case SQL_INT64:
        return field_t(
            std::in_place_type<scaled_integer<int64_t>>,
            *reinterpret_cast<int64_t*>(data), _ptr->sqlscale);

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

