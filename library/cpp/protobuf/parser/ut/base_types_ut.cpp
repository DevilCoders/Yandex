#include "base_types.h"

#include <library/cpp/testing/unittest/registar.h>

#include <limits>

class TBaseTypesTest: public TTestBase {
    UNIT_TEST_SUITE(TBaseTypesTest);
    UNIT_TEST(FloatToIntTest);
    UNIT_TEST_SUITE_END();

public:
    void FloatToIntTest() {
        using namespace NProtoParser::NBaseTypesConvertor;
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double infinity = std::numeric_limits<double>::infinity();
        const float negInfinity = -std::numeric_limits<float>::infinity();
        UNIT_ASSERT_EXCEPTION(SafeFloatToIntCast<i64>(nan), TBadCastException);
        UNIT_ASSERT_EXCEPTION(SafeFloatToIntCast<i64>(infinity), TBadCastException);
        UNIT_ASSERT_EXCEPTION(SafeFloatToIntCast<i64>(negInfinity), TBadCastException);
        UNIT_ASSERT_EXCEPTION(SafeFloatToIntCast<i64>(static_cast<double>(Max<ui64>()) + 1500000.0), TBadCastException);
        UNIT_ASSERT_EXCEPTION(SafeFloatToIntCast<i64>(static_cast<double>(Max<ui64>()) - 1500000.0), TBadCastException);
        UNIT_ASSERT_EXCEPTION(SafeFloatToIntCast<i64>(static_cast<double>(Max<ui64>() - 3000)), TBadCastException);
        UNIT_ASSERT_EXCEPTION(SafeFloatToIntCast<i32>(static_cast<double>(Min<i64>() + 300)), TBadCastException);

        UNIT_ASSERT_EQUAL(100, SafeFloatToIntCast<i64>(100.25));
    }
};

UNIT_TEST_SUITE_REGISTRATION(TBaseTypesTest);
