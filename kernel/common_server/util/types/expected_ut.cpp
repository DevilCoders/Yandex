#include "expected.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(ExpectedSuite) {
    Y_UNIT_TEST(Simple) {
        {
            TExpected<TString, int> result = "hello";
            UNIT_ASSERT(result);
            UNIT_ASSERT_VALUES_EQUAL(result->size(), 5);
            UNIT_ASSERT_VALUES_EQUAL(result.GetValue(), "hello");
            UNIT_ASSERT_VALUES_EQUAL(ToString(result), "hello");
        }
        {
            TExpected<TString, int> result = MakeUnexpected(42);
            UNIT_ASSERT(!result);
            UNIT_ASSERT_VALUES_EQUAL(result.GetError(), 42);
            UNIT_ASSERT_VALUES_EQUAL(ToString(result), "42");
            try {
                result.GetValue(result.Throw);
                UNIT_ASSERT(false && "exception has not been thrown");
            } catch (const TUnexpectedException&) {
            }
        }
    }

    int ThrowMe(TString error) {
        ythrow yexception() << error;
        return -1;
    }

    Y_UNIT_TEST(Wrapping) {
        auto wrapped = WrapUnexpected<yexception>(ThrowMe, "fake");
        UNIT_ASSERT(!wrapped);
        UNIT_ASSERT(TStringBuf(wrapped.GetError().what()).EndsWith("fake"));
    }
}
