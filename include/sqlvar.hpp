#pragma once
#include "types.hpp"
#include "exception.hpp"

#include <cstdlib>
#include <stdexcept>

#include <ostream>
#include <map>

namespace fb
{

// This is only a view of XSQLVAR. Be carefull with expired values.
// See https://docwiki.embarcadero.com/InterBase/2020/en/XSQLVAR_Field_Descriptions
struct sqlvar
{
    using type = XSQLVAR;
    using pointer = std::add_pointer_t<type>;

    explicit sqlvar(pointer var) : _ptr(var) { }

    // Set methods

    // TODO make enum of stype
    void set(int stype, const void* sdata, size_t slen)
    {
        _ptr->sqltype = stype;
        _ptr->sqldata = (char*)sdata;
        _ptr->sqllen = slen;
    }

    // Skip method
    void set(skip_t) { }

    void set(const float& val)
    { set(SQL_FLOAT, &val, sizeof(float)); }

    void set(const double& val)
    { set(SQL_DOUBLE, &val, sizeof(double)); }

    void set(const int16_t& val)
    { set(SQL_SHORT, &val, 2); }

    void set(const int32_t& val)
    { set(SQL_LONG, &val, 4); }

    void set(const int64_t& val)
    { set(SQL_INT64, &val, 8); }

    void set(std::string_view val)
    { set(SQL_TEXT, val.data(), val.size()); }

    void set(const timestamp_t& val)
    { set(SQL_TIMESTAMP, &val, sizeof(timestamp_t)); }

    void set(std::nullptr_t)
    { _ptr->sqltype = SQL_NULL; }

    // Set by assigning
    template <class T>
    auto operator=(const T& val) -> decltype(set(val), *this)
    { return (set(val), *this); }

    // Get methods

    // As variant
    field_t get() const;

    // As type
    template <class T>
    T get() const
    { return std::visit(type_converter<T>(), get()); }

    // Return by assigning
    template <class T>
    operator T() const
    { return get<T>(); }

    // For debug purpose
    friend std::ostream& operator<<(std::ostream& os, const sqlvar&);

    // Get column name
    std::string_view name() const
    { return std::string_view(_ptr->sqlname, _ptr->sqlname_length); }

    // This will show actual max column size
    // before set() destroys it
    size_t size() const
    { return _ptr->sqllen; }

private:
    pointer _ptr;

    // Get scaled value of base 10
    size_t scale_tens() const
    {
        int scale = std::abs(_ptr->sqlscale);
        int tens = 1;
        for (; scale > 0; --scale)
            tens *= 10;
        return tens;
    }
};


// Get value variant
field_t sqlvar::get() const
{
    bool is_null = (_ptr->sqltype & 1) && (*_ptr->sqlind < 0);
    if (is_null)
        return field_t(std::in_place_type<std::nullptr_t>, nullptr);

    short dtype = _ptr->sqltype & ~1;
    char* data = _ptr->sqldata;

    switch (dtype) {

    case SQL_TEXT:
        return field_t(std::in_place_type<std::string_view>, data, _ptr->sqllen);

    case SQL_VARYING:
    {
        // While sqllen set to max column size, vary_length
        // is actual length of this field
        auto pv = (PARAMVARY*)data;
        return field_t(
            std::in_place_type<std::string_view>,
            (const char*)pv->vary_string, pv->vary_length);
    }

    case SQL_SHORT:
    {
        int16_t val = *(int16_t*)data;
        if (_ptr->sqlscale < 0)
            return field_t(std::in_place_type<float>, float(val) / scale_tens());
        else if (_ptr->sqlscale)
            return field_t(std::in_place_type<int32_t>, int32_t(val) * scale_tens());
        else
            return field_t(std::in_place_type<int16_t>, val);
    }

    case SQL_LONG:
    {
        int32_t val = *(int32_t*)data;
        if (_ptr->sqlscale < 0)
            return field_t(std::in_place_type<float>, float(val) / scale_tens());
        else if (_ptr->sqlscale)
            return field_t(std::in_place_type<int64_t>, int64_t(val) * scale_tens());
        else
            return field_t(std::in_place_type<int32_t>, val);
    }

    case SQL_INT64:
    {
        int64_t val = *(int64_t*)data;
        if (_ptr->sqlscale < 0)
            return field_t(std::in_place_type<double>, double(val) / scale_tens());
        else if (_ptr->sqlscale)
            return field_t(std::in_place_type<double>, double(val) * scale_tens());
        else
            return field_t(std::in_place_type<int64_t>, val);
    }

    case SQL_FLOAT:
        return field_t(std::in_place_type<float>, *(float*)data);

    case SQL_DOUBLE:
        return field_t(std::in_place_type<double>, *(double*)data);

    case SQL_TIMESTAMP:
        return field_t(std::in_place_type<timestamp_t>, *(timestamp_t*)data);

    case SQL_TYPE_DATE:
    {
        timestamp_t t;
        t.timestamp_date = *(ISC_DATE*)data;
        return field_t(std::in_place_type<timestamp_t>, t);
    }

    case SQL_TYPE_TIME:
    {
        timestamp_t t;
        t.timestamp_time = *(ISC_TIME*)data;
        return field_t(std::in_place_type<timestamp_t>, t);
    }

    case SQL_BLOB:
    case SQL_ARRAY:
        return field_t(std::in_place_type<blob_id_t>, *(ISC_QUAD*)data);

    default:
        throw fb::exception("type (") << dtype << ") not implemented";
    }
}

std::ostream& operator<<(std::ostream& os, const sqlvar& v)
{
    static std::map<int, std::string_view> types =
    {
        { SQL_TEXT, "SQL_TEXT" },
        { SQL_VARYING, "SQL_VARYING" },
        { SQL_SHORT, "SQL_SHORT" },
        { SQL_LONG, "SQL_LONG" },
        { SQL_INT64, "SQL_INT64" },
        { SQL_FLOAT, "SQL_FLOAT" },
        { SQL_DOUBLE, "SQL_DOUBLE" },
        { SQL_TIMESTAMP, "SQL_TIMESTAMP" },
        { SQL_TYPE_DATE, "SQL_TYPE_DATE" },
        { SQL_TYPE_TIME, "SQL_TYPE_TIME" },
        { SQL_BLOB, "SQL_BLOB" },
        { SQL_ARRAY, "SQL_ARRAY" },
    };

    bool is_null = (v._ptr->sqltype & 1) &&
        v._ptr->sqlind && (*v._ptr->sqlind < 0);
    short dtype = v._ptr->sqltype & ~1;

    auto it = types.find(dtype);
    if (it != types.end())
        os << types[dtype];
    else
        os << "unknown type (" << dtype << ")";

    os << ": len: " << v._ptr->sqllen;
    if (is_null)
        os << ", null";
    os << std::endl;

    return os;
}

} // namespace fb

