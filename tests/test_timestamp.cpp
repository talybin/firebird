#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "types.hpp"

using timest = fb::timestamp_t;

TEST_CASE("testing std::tm")
{
    std::tm tm = { 0 };
    timest{ 0, 0 }.to_tm(&tm);

    // SQL epoch starts with 1858-11-17 00:00:00
    CHECK   (tm.tm_year + 1900 == 1858);
    CHECK   (tm.tm_mon + 1 == 11);
    CHECK   (tm.tm_mday == 17);
    CHECK   (tm.tm_hour == 0);
    CHECK   (tm.tm_min == 0);
    CHECK   (tm.tm_sec == 0);
    CHECK   (tm.tm_wday == 3);
    CHECK   (tm.tm_yday == 320);

    // Convert back
    CHECK   (timest::from_tm(&tm).timestamp_date == 0);
    CHECK   (timest::from_tm(&tm).timestamp_time == 0);
}

TEST_CASE("testing to_time_t")
{
    // time_t epoch starts with 1970-01-01,
    // much later than SQL epoch, to_time_t
    // should return 0 in this case.
    CHECK   (timest{ 0, 0 }.to_time_t() == 0);

    // Test a date that fits (2024-06-07 22:06:10 UTC)
    CHECK   (timest{ 60'468,
                (22 * 3600 + 06 * 60 + 10) * 10'000 }.to_time_t() == 1'717'797'970);
}

TEST_CASE("testing from_time_t")
{
    // Test some date, 2024-06-07 22:06:10 UTC
    CHECK   (timest::from_time_t(1'717'797'970).timestamp_date == 60'468);
    CHECK   (timest::from_time_t(1'717'797'970)
                .timestamp_time == (22 * 3600 + 06 * 60 + 10) * 10'000);

    // Test epoch (40'587 days since 1858-11-17)
    CHECK   (timest::from_time_t(0).timestamp_date == 40'587);
    CHECK   (timest::from_time_t(0).timestamp_time == 0);
}

