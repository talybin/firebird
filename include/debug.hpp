#pragma once
#include "firebird.hpp"
#include <iomanip>
#include <ostream>
#include <map>

// Stream types for debugging purposes

inline std::ostream& operator<<(std::ostream& os, const fb::timestamp_t& t)
{
    std::tm tm;
    return os << std::put_time(t.to_tm(&tm), "%Y-%m-%d %X")
        << "." << std::setfill('0') << std::setw(3) << t.ms()
        << " (" << t.timestamp_date << ", " << t.timestamp_time << ")";
}

inline std::ostream& operator<<(std::ostream& os, const fb::blob_id_t& t)
{ return os; }

inline std::ostream& operator<<(std::ostream& os, const fb::sqlvar& v)
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

    auto ptr = v.handle();
    bool is_null = (ptr->sqltype & 1) &&
        ptr->sqlind && (*ptr->sqlind < 0);
    short dtype = ptr->sqltype & ~1;

    auto it = types.find(dtype);
    if (it != types.end())
        os << types[dtype];
    else
        os << "unknown type (" << dtype << ")";

    os << ": len: " << ptr->sqllen;
    if (is_null)
        os << ", null";
    os << std::endl;

    return os;
}

std::ostream& operator<<(std::ostream& os, const fb::sqlda& v)
{
    auto ptr = v.get();
    if (ptr) {
        os << "sqln (cols allocated): " << ptr->sqln << std::endl;
        os << "sqld (cols used): " << ptr->sqld << std::endl;

        size_t cnt = 0;
        for (auto it = v.begin(); it != v.end(); ++it, ++cnt) {
            os << "--- sqlvar: " << cnt << " ---" << std::endl;
            os << *it;
        }
    }
    else
        os << "sqlda: nullptr" << std::endl;

    os << "------------------------------" << std::endl;
    return os;
}

