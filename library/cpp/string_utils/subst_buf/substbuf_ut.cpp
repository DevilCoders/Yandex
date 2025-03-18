#include "substbuf.h"
#include <util/memory/segmented_string_pool.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TStringBufSubst) {
    Y_UNIT_TEST(TestSubstGlobal) {
        TStringBuf s;
        segmented_string_pool p;
        s = "aaa";
        SubstGlobal(s, "a", "bb", p);
        UNIT_ASSERT_EQUAL(s, TString("bbbbbb"));
        s = "aaa";
        SubstGlobal(s, "a", "b", p);
        UNIT_ASSERT_EQUAL(s, TString("bbb"));
        s = "aaa";
        SubstGlobal(s, "a", "", p);
        UNIT_ASSERT_EQUAL(s, TString(""));
        s = "aaa";
        SubstGlobal(s, "aaaa", "bbbb", p);
        UNIT_ASSERT_EQUAL(s, TString("aaa"));
        s = "abc";
        SubstGlobal(s, "ab", "ccc", p);
        UNIT_ASSERT_EQUAL(s, TString("cccc"));
        s = "abc";
        SubstGlobal(s, "bc", "aaa", p);
        UNIT_ASSERT_EQUAL(s, TString("aaaa"));
        s = "abc";
        SubstGlobal(s, 'b', 'c', p);
        UNIT_ASSERT_EQUAL(s, TString("acc"));
    }
}
