/// \file sqlvar.hpp
/// This file contains the definition of the sqlvar structure for
/// handling SQL variable data.

#pragma once
#include "types.hpp"
#include "exception.hpp"

#include <cstdlib>
#include <stdexcept>

namespace fb
{

/// This is a view of XSQLVAR. Be careful with expired values.
///
/// \see https://docwiki.embarcadero.com/InterBase/2020/en/XSQLVAR_Field_Descriptions
///
struct sqlvar
{
    /// Alias for XSQLVAR type.
    using type = XSQLVAR;
    /// Alias for pointer to XSQLVAR type.
    using pointer = std::add_pointer_t<type>;

    /// Constructs an sqlvar object from a given XSQLVAR pointer.
    ///
    /// \param[in] var - Pointer to XSQLVAR.
    ///
    explicit sqlvar(pointer var) noexcept
    : _ptr(var) { }

    // Set methods

    /// Generic set method for sqltype, data, and length.
    ///
    /// \param[in] stype - SQL type.
    /// \param[in] sdata - Pointer to data.
    /// \param[in] slen - Length of data.
    ///
    void set(int stype, const void* sdata, size_t slen) noexcept
    {
        _ptr->sqltype = stype;
        _ptr->sqldata = const_cast<char*>(reinterpret_cast<const char*>(sdata));
        _ptr->sqllen = slen;
    }

    /// Skip method, does nothing.
    void set(skip_t) noexcept { }

    /// Sets the value of the SQL variable to a float.
    void set(const float& val) noexcept
    { set(SQL_FLOAT, &val, sizeof(float)); }

    /// Sets the value of the SQL variable to a double.
    void set(const double& val) noexcept
    { set(SQL_DOUBLE, &val, sizeof(double)); }

    /// Sets the value of the SQL variable to an int16_t.
    void set(const int16_t& val) noexcept
    { set(SQL_SHORT, &val, 2); }

    /// Sets the value of the SQL variable to an int32_t.
    void set(const int32_t& val) noexcept
    { set(SQL_LONG, &val, 4); }

    /// Sets the value of the SQL variable to an int64_t.
    void set(const int64_t& val) noexcept
    { set(SQL_INT64, &val, 8); }

    /// Sets the value of the SQL variable to a string.
    void set(std::string_view val) noexcept
    { set(SQL_TEXT, val.data(), val.size()); }

    /// Sets the value of the SQL variable to a timestamp.
    void set(const timestamp_t& val) noexcept
    { set(SQL_TIMESTAMP, &val, sizeof(timestamp_t)); }

    /// Sets the value of the SQL variable to a blob.
    void set(const blob_id_t& val) noexcept
    { set(SQL_BLOB, &val, sizeof(blob_id_t)); }

    /// Sets the value of the SQL variable to null.
    void set(std::nullptr_t) noexcept
    { _ptr->sqltype = SQL_NULL; }

    /// Set by assigning.
    ///
    /// \tparam T - Type of the value.
    /// \param[in] val - Value to assign.
    ///
    /// \return Reference to this sqlvar object.
    ///
    template <class T>
    auto operator=(const T& val) noexcept -> decltype(set(val), *this)
    { return (set(val), *this); }

    // Get methods

    /// Returns the internal pointer to XSQLVAR (const version).
    const pointer handle() const noexcept
    { return _ptr; }

    /// Returns the internal pointer to XSQLVAR.
    pointer handle() noexcept
    { return _ptr; }

    /// Gets the column name.
    std::string_view name() const noexcept
    { return std::string_view(_ptr->sqlname, _ptr->sqlname_length); }

    /// Gets the table name.
    std::string_view table() const noexcept
    { return std::string_view(_ptr->relname, _ptr->relname_length); }

    /// Gets the SQL data type.
    uint16_t sql_datatype() const noexcept
    { return _ptr->sqltype & ~1; }

    /// Gets the maximum column size (in bytes) as
    /// allocated on firebird server. For example
    /// maximum allowed length of string for this
    /// specific column as given as VARCHAR(size).
    ///
    /// \note set() methods may rewrite this value.
    ///
    size_t size() const noexcept
    { return _ptr->sqllen; }

    /// Checks if the value is null.
    bool is_null() const noexcept
    { return (_ptr->sqltype & 1) && (*_ptr->sqlind < 0); }

    /// Checks for null.
    ///
    /// \code{.cpp}
    ///     if (row["CUSTOMER_ID"]) {
    ///         // This field is not null
    ///     }
    /// \endcode
    ///
    operator bool() const noexcept
    { return !is_null(); }

    /// Return the value by assigning.
    ///
    /// \code{.cpp}
    ///     int cust_id = row["CUSTOMER_ID"];
    /// \endcode
    ///
    /// \tparam T - Type to convert to.
    ///
    /// \return Value of the specified type.
    /// \throw fb::exception
    ///
    template <class T>
    operator T() const
    { return value<T>(); }

    /// Gets the value. Throws if the value is null.
    /// The actual type of the value (as received from
    /// database) may be converted to requested type.
    ///
    /// \code{.cpp}
    ///     // The actual type of field is int32_t, but we want a string here
    ///     auto cust_id = row["CUSTOMER_ID"].value<std::string>();
    /// \endcode
    ///
    /// \tparam T - Type of the value.
    ///
    /// \return Value of the specified type.
    /// \throw fb::exception
    ///
    template <class T>
    T value() const
    {
        if (is_null())
            throw fb::exception("type is null");
        return visit<T>();
    }

    /// Gets the value or default value if null.
    ///
    /// \code{.cpp}
    ///     // Set cust_id to 0 if field is null
    ///     auto cust_id = row["CUSTOMER_ID"].value_or(0);
    /// \endcode
    ///
    /// \tparam T - Type of the value.
    /// \param[in] default_value - Default value to return if null.
    ///
    /// \return Value of the specified type or default value.
    /// \throw fb::exception
    ///
    template <class T>
    T value_or(T default_value) const
    {
        if (is_null())
            return std::move(default_value);
        return visit<T>();
    }

    /// Gets the value as a variant (field_t).
    ///
    /// \return Value as a variant.
    /// \throw fb::exception
    ///
    field_t as_variant() const;

private:
    /// Internal pointer to XSQLVAR
    pointer _ptr;

    /// Extract value from a variant. Value may be
    /// converted to another type by type_converter.
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

// Gets the value as a variant (field_t).
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

