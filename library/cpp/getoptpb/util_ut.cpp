#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/getoptpb/ut/testconf.pb.h>

#include "util.h"

using namespace NGetoptPb;

class TGetoptPbUtilTest: public TTestBase {
    UNIT_TEST_SUITE(TGetoptPbUtilTest);
    UNIT_TEST(TestGetEnumValue);
    UNIT_TEST_SUITE_END();

private:
    void TestGetEnumValue();
};

UNIT_TEST_SUITE_REGISTRATION(TGetoptPbUtilTest)

void TGetoptPbUtilTest::TestGetEnumValue() {
    UNIT_ASSERT_EQUAL(GetEnumValue(TTestConfig::TE_ONE), "one");
    UNIT_ASSERT(GetEnumValue(TTestConfig::TE_TWO));
    UNIT_ASSERT(!GetEnumValue(TTestConfig::TE_WITHOUT_VAL));
}
