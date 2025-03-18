#include "util.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(Util) {
    Y_UNIT_TEST_DECLARE(Conditions);
}

Y_UNIT_TEST_SUITE_IMPLEMENTATION(Util) {
    Y_UNIT_TEST(SpliByOneOf) {
        {
            TString str("one two");
            TStringBuf first, rest;
            SplitByOneOf(str, " ", first, rest);
            UNIT_ASSERT(first == "one");
            UNIT_ASSERT(rest == "two");
        }

        {
            TString str("one two");
            TStringBuf first, rest;
            SplitByOneOf(str, " \t,;\n\r", first, rest);
            UNIT_ASSERT(first == "one");
            UNIT_ASSERT(rest == "two");
        }

        {
            TString str("one two \t , three");
            TStringBuf first, rest;
            SplitByOneOf(str, " \t,;\n\r", first, rest);
            UNIT_ASSERT(first == "one");
            UNIT_ASSERT(rest == "two \t , three");
        }

        {
            TString str("one\t two");
            TStringBuf first, rest;
            SplitByOneOf(str, " \t", first, rest);
            UNIT_ASSERT(first == "one");
            UNIT_ASSERT(rest == "two");
        }

        {
            TString str("one\t\rtwo");
            TStringBuf first, rest;
            SplitByOneOf(str, " \t", first, rest);
            UNIT_ASSERT(first == "one");
            UNIT_ASSERT(rest == "\rtwo");
        }
    }
};
