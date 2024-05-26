#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "types.hpp"

template <class T>
using si_t = fb::scaled_integer<T>;


TEST_CASE("testing overflow")
{
    // Reminder! short: [-32768 ... 32767]
    using si = si_t<int32_t>;

    // Test positive values:

    // Expected value should be 327'670, should not fit
    CHECK_THROWS    (si(32'767, 1).get<short>());
    // But should fit in int32_t
    CHECK_NOTHROW   (si(32'767, 1).get<int32_t>());

    // Lower value should fit in
    CHECK_NOTHROW   (si(3'276, 1).get<short>());
    // But not with scale 2 (327'600)
    CHECK_THROWS    (si(3'276, 2).get<short>());

    // Test negative values:

    CHECK_THROWS    (si(-32'768, 1).get<short>());
    CHECK_NOTHROW   (si(-32'768, 1).get<int32_t>());

    CHECK_NOTHROW   (si(-3'276, 1).get<short>());
    CHECK_THROWS    (si(-3'276, 2).get<short>());

    // Test min and max:

    CHECK_NOTHROW   (si(32'767, 0).get<short>());
    CHECK_THROWS    (si(32'768, 0).get<short>());

    CHECK_NOTHROW   (si(-32'768, 0).get<short>());
    CHECK_THROWS    (si(-32'769, 0).get<short>());

    // Test assign of downgraded value
    CHECK_THROWS    (si(-32'769, -1).get<short>());
    // But it should not throw on lower value in range
    CHECK_NOTHROW   (si(-32'768, -1).get<short>());
}


TEST_CASE("testing scaled values")
{
}


TEST_CASE("testing to_string")
{
}

