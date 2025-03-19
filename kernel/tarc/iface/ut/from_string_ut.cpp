#include <kernel/tarc/iface/tarcface.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TTarcFromStringTest) {

    Y_UNIT_TEST(TestFromStringI64) {
        i64 x = FromString("123");
        UNIT_ASSERT_EQUAL(x, 123);
    }

    Y_UNIT_TEST(TestFromStringEArchiveZone) {
        EArchiveZone x = FromString("123");
        UNIT_ASSERT_EQUAL(x, AZ_COUNT);

        EArchiveZone y = FromString("title");
        UNIT_ASSERT_EQUAL(y, AZ_TITLE);
    }
}
