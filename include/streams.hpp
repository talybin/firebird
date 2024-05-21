#pragma once
#include "types.hpp"
#include <iomanip>

inline std::ostream& operator<<(std::ostream& os, const fb::timestamp_t& t)
{
    std::tm tm;
    return os << std::put_time(t.to_tm(&tm), "%Y-%m-%d %X")
        << "." << std::setfill('0') << std::setw(3) << t.ms()
        << " (" << t.timestamp_date << ", " << t.timestamp_time << ")";
}

inline std::ostream& operator<<(std::ostream& os, const fb::blob_id_t& t)
{ return os; }

