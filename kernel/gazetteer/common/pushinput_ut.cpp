#include "push.h"
#include "str.h"
#include <util/generic/string.h>

#include <library/cpp/testing/unittest/registar.h>


class TPushInputTest: public TTestBase {
        UNIT_TEST_SUITE(TPushInputTest);
            UNIT_TEST(TestPush);
        UNIT_TEST_SUITE_END();
    public:
        void TestPush();
};

UNIT_TEST_SUITE_REGISTRATION(TPushInputTest);


static inline TString LoadString(IInputStream& input, size_t len) {
    TString ret;
    ret.ReserveAndResize(len);
    UNIT_ASSERT_EQUAL(input.Load(ret.begin(), len), len);
    return ret;
}

void TPushInputTest::TestPush() {
    static const TString data = "1234567890";

    TStringInput str(data);
    TPushInput input(&str);
    TString a1 = LoadString(input, 1);
    UNIT_ASSERT_EQUAL(a1, "1");

    input.Push(a1);
    a1 = LoadString(input, 1);
    UNIT_ASSERT_EQUAL(a1, "1");

    TString a23 = LoadString(input, 2);
    UNIT_ASSERT_EQUAL(a23, "23");


    input.Push(a23);
    input.Push(a1);
    TString a123 = LoadString(input, 3);
    UNIT_ASSERT_EQUAL(a123, "123");

    input.Push(a1);
    input.Push(a23);
    TString a231 = LoadString(input, 3);
    UNIT_ASSERT_EQUAL(a231, "231");

    input.Push("");
    input.Push(a23);
    input.Push(a1);
    input.Push("");
    input.Push("abc");
    input.Push("");

    TString a12345 = LoadString(input, 8);
    UNIT_ASSERT_EQUAL(a12345, "abc12345");

    TString a67890 = LoadString(input, 5);
    UNIT_ASSERT_EQUAL(a67890, "67890");

    UNIT_ASSERT_EQUAL(input.ReadAll(), "");

    input.Push("def");
    input.Push("abc");

    UNIT_ASSERT_EQUAL(input.ReadAll(), "abcdef");
}
