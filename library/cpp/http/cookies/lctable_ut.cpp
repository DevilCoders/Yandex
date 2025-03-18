#include "lctable.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TLcTable) {
    typedef TLowerCaseTable<TStringBuf> TTable;

    Y_UNIT_TEST(TestEmpty) {
        UNIT_ASSERT(TTable().Empty());
    }

    Y_UNIT_TEST(TestSize) {
        TTable tbl;
        UNIT_ASSERT_VALUES_EQUAL(tbl.Size(), 0);

        char key[] = "a";
        char value[] = "b";
        for (int i=0; i < 20; ++i) {
            tbl.Add(key, value);
            UNIT_ASSERT_VALUES_EQUAL(tbl.Size(), i + 1);
            key[0]++;
            value[0]++;
        }
    }

    Y_UNIT_TEST(TestCaseIgnore) {
        TTable tbl;

        tbl.Add("abc", "1");

        UNIT_ASSERT_VALUES_EQUAL(tbl.Get("aBc"), "1");
    }

    Y_UNIT_TEST(TestMultiMap) {
        TTable tbl;

        tbl.Add("abc", "2");
        tbl.Add("abC", "1");

        UNIT_ASSERT_VALUES_EQUAL(tbl.NumOfValues("abC"), 2u);

        UNIT_ASSERT_VALUES_EQUAL(tbl.Get("Abc"), "2");
        UNIT_ASSERT_VALUES_EQUAL(tbl.Get("AbC", 0), "2");
        UNIT_ASSERT_VALUES_EQUAL(tbl.Get("Abc", 1), "1");
        UNIT_ASSERT_VALUES_EQUAL(tbl.Get("Abc", 2), "");
    }

    Y_UNIT_TEST(TestGetNew) {
        TTable tbl;

        tbl.Add("abc", "2");

        UNIT_ASSERT_VALUES_EQUAL(tbl.Get("def"), "");
    }

    Y_UNIT_TEST(TestErase) {
        TTable tbl;

        tbl.Add("ABC", "bca");
        UNIT_ASSERT_VALUES_EQUAL(tbl.Size(), 1);

        UNIT_ASSERT_VALUES_EQUAL(tbl.Erase("abc"), 1);
        UNIT_ASSERT_VALUES_EQUAL(tbl.Size(), 0);

        tbl.Add("aBc", "1");
        tbl.Add("Abc", "2");
        UNIT_ASSERT_VALUES_EQUAL(tbl.Erase("abC"), 2);
        UNIT_ASSERT(tbl.Empty());

        tbl.Add("qwe", "rty");
        tbl.Add("239", "143");
        UNIT_ASSERT_VALUES_EQUAL(tbl.Erase("qwe"), 1);
        UNIT_ASSERT_VALUES_EQUAL(tbl.Size(), 1);
    }
}
