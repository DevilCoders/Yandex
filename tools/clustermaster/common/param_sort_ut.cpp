#include "param_sort.h"

#include <library/cpp/testing/unittest/registar.h>

namespace {
    void ExpectLess(const char* a, const char* b) {
        TString message = TString() + a + " must be lt " + b;
        UNIT_ASSERT_C(TNiceLess<const char*>()(a, b), message);
    }

    void ExpectGreaterOrEqual(const char* a, const char* b) {
        TString message = TString() + a + " must be ge " + b;
        UNIT_ASSERT_C(!TNiceLess<const char*>()(a, b), message);
    }

}


Y_UNIT_TEST_SUITE(TNiceLess) {
    Y_UNIT_TEST(TestSimple) {
        ExpectLess("", "b");
        ExpectLess("a", "b");
        ExpectLess("a", "aa");
        ExpectLess("a", "a12");
        ExpectLess("a11", "a12");
        ExpectLess("a2", "a11");
        ExpectLess("a2", "a2a");
        ExpectLess("a2a2", "a2ab");
        ExpectLess("2", "11");
        ExpectLess("2aa", "11aa");
        ExpectLess("02", "11");

        ExpectGreaterOrEqual("b", "");
        ExpectGreaterOrEqual("b", "a");
        ExpectGreaterOrEqual("b", "b");
        ExpectGreaterOrEqual("b011", "b11");
        ExpectGreaterOrEqual("b012a", "b12a");
    }
}
