#include "metainfo.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TAttrsMetainfoTest) {

    void TestScanC2NImpl(bool dynamic) {
        const char* c2n =
            "3\tg\n"
            "4\thijkl\n"
            "1\tabc\n"
            "2\tdef\n"
            "5\tx\ty\tz\n"; // Even this upyachka possible :(

        NGroupingAttrs::TMetainfo meta(dynamic);
        meta.ScanStroka(c2n, NGroupingAttrs::TMetainfo::C2N, true);

        UNIT_ASSERT_STRINGS_EQUAL("abc", meta.Categ2Name(1));
        UNIT_ASSERT_STRINGS_EQUAL("def", meta.Categ2Name(2));
        UNIT_ASSERT_STRINGS_EQUAL("g", meta.Categ2Name(3));
        UNIT_ASSERT_STRINGS_EQUAL("hijkl", meta.Categ2Name(4));
        UNIT_ASSERT_STRINGS_EQUAL("x\ty\tz", meta.Categ2Name(5));

        UNIT_ASSERT_EQUAL(1, meta.Name2Categ("abc"));
        UNIT_ASSERT_EQUAL(2, meta.Name2Categ("def"));
        UNIT_ASSERT_EQUAL(3, meta.Name2Categ("g"));
        UNIT_ASSERT_EQUAL(4, meta.Name2Categ("hijkl"));
        UNIT_ASSERT_EQUAL(5, meta.Name2Categ("x\ty\tz"));
    }

    Y_UNIT_TEST(ScanC2P) {
        NGroupingAttrs::TMetainfo meta(false);
        meta.ScanStroka("1\t3\n2\t4\n", NGroupingAttrs::TMetainfo::C2P);

        UNIT_ASSERT_EQUAL(3, meta.Categ2Parent(1));
        UNIT_ASSERT_EQUAL(4, meta.Categ2Parent(2));
    }

    Y_UNIT_TEST(ScanBadC2P) {
        // test after SEARCH-2961
        NGroupingAttrs::TMetainfo meta(false);
        UNIT_ASSERT_EXCEPTION(meta.ScanStroka("a", NGroupingAttrs::TMetainfo::C2P), yexception);
    }

    Y_UNIT_TEST(ScanC2L) {
        const char* c2l =
            "1\t3\n"
            "1\t4\n"
            "1\t5\n"
            "2\t1\n"
            "2\t2\n"
            "1\t6";

        NGroupingAttrs::TMetainfo meta(false);
        meta.ScanStroka(c2l, NGroupingAttrs::TMetainfo::C2L);

        TCategSeries s;
        meta.CategLinks(1, s);
        UNIT_ASSERT_EQUAL(4, s.size());
        UNIT_ASSERT_EQUAL(3, s.GetCateg(0));
        UNIT_ASSERT_EQUAL(4, s.GetCateg(1));
        UNIT_ASSERT_EQUAL(5, s.GetCateg(2));
        UNIT_ASSERT_EQUAL(6, s.GetCateg(3));
        s.Clear();
        meta.CategLinks(2, s);
        UNIT_ASSERT_EQUAL(2, s.size());
        UNIT_ASSERT_EQUAL(1, s.GetCateg(0));
        UNIT_ASSERT_EQUAL(2, s.GetCateg(1));
    }

    Y_UNIT_TEST(ScanC2N) {
        TestScanC2NImpl(false);
        TestScanC2NImpl(true);
    }

    Y_UNIT_TEST(TestScanC2Co) {
        const char* c2co =
            "3\t1.5\n"
            "1\t4\n"
            "2\t2.3333";

        NGroupingAttrs::TMetainfo meta(false);
        meta.ScanStroka(c2co, NGroupingAttrs::TMetainfo::C2Co);

        UNIT_ASSERT_EQUAL(4.f, meta.CategCoeff(1));
        UNIT_ASSERT_EQUAL(2.3333f, meta.CategCoeff(2));
        UNIT_ASSERT_EQUAL(1.5f, meta.CategCoeff(3));
    }
}
