#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "types.hpp"

template <class T>
using si_t = fb::scaled_integer<T>;

template <class T>
constexpr auto check_type = std::is_same_v<decltype(std::declval<si_t<T>>().get()), T>;


TEST_CASE("testing overflow")
{
    // Reminder! short: [-32768 ... 32767]
    using si = si_t<short>;

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
    CHECK_NOTHROW   (si(-32'768, 0).get<short>());

    // It should not throw on lower value in range
    CHECK_NOTHROW   (si(-32'768, -1).get<short>());

    // It should always throw on lower type
    CHECK_THROWS    (si(0, 0).get<char>());
    CHECK_THROWS    (si(0, 1).get<char>());
    CHECK_THROWS    (si(0, -1).get<char>());
}


TEST_CASE("testing scaled values")
{
    using si = si_t<int32_t>;

    // Return of get() without specifying return type
    // should be the same as containing type
    CHECK   (check_type<int16_t>);
    CHECK   (check_type<int32_t>);
    CHECK   (check_type<int64_t>);

    // Test scale

    // Scale 0
    CHECK   (si(42, 0).get() == 42);
    CHECK   (si(-42, 0).get() == -42);

    // Positive scale
    CHECK   (si(42, 1).get() == 420);
    CHECK   (si(-42, 1).get() == -420);
    CHECK   (si(42, 2).get() == 4'200);
    CHECK   (si(-42, 2).get() == -4'200);
    CHECK   (si(42, 3).get() == 42'000);
    CHECK   (si(-42, 3).get() == -42'000);

    // Negative scale
    CHECK   (si(42, -1).get() == 4);
    CHECK   (si(42, -2).get() == 0);
    CHECK   (si(42, -3).get() == 0);
    CHECK   (si(12'345, -1).get() == 1'234);
    CHECK   (si(12'345, -2).get() == 123);
    CHECK   (si(12'345, -3).get() == 12);

    // And the one with last digit over 5.
    // Testing that value is not rounded up.
    CHECK   (si(1579, -1).get() == 157);
}


TEST_CASE("testing to_string")
{
    // TODO add tests here
}

