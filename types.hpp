#pragma once
#include "ibase.h"

#include <variant>
#include <string_view>
#include <ctime>

namespace fb
{

// Tag to skip parameter
struct skip_t { };
constexpr const skip_t skip;

// Methods to convert timestamp
struct timestamp_t : ISC_TIMESTAMP
{
    // Convert timestamp_t to time_t.
    // Where 40587 is GDS epoch start in days (year ~1859, time_t starts at 1970).
    time_t to_time_t() const
    { return std::max(int(timestamp_date) - 40587, 0) * 86400 + timestamp_time / 10'000; }

    std::tm* to_tm() const {
        time_t epoch = to_time_t();
        return std::gmtime(&epoch);
    }

    size_t ms() const
    { return (timestamp_time / 10) % 1000; }
};

using blob_id_t = ISC_QUAD;

using field_t = std::variant<
    std::nullptr_t,
    std::string_view,
    int16_t,
    int32_t,
    int64_t,
    float,
    double,
    timestamp_t,
    blob_id_t
>;

} // namespace fb

