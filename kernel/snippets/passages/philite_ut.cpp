#include <library/cpp/testing/unittest/registar.h>

#include "passages.h"

#include <util/charset/wide.h>

class TPHilightTest: public TTestBase
{
    UNIT_TEST_SUITE(TPHilightTest);

    UNIT_TEST(test_cut_passage);
    UNIT_TEST(test_replace_hilites);

    UNIT_TEST_SUITE_END();

public:
    void test_cut_passage()
    {
        TString s = TString(HILIGHT_MARK) + TString(TITLE_MARK);
        CutPassage(s, 10, false);
        UNIT_ASSERT(s.empty());
        TUtf16String sample = UTF8ToWide(TString(
                "some""\x20""very""\x20""""\x07""""\x5B""clever""\x07""""\x5D""""\x20""text""\x20""created""\x20""by""\x20""very""\x20""""\x07""""\x5B""clever""\x07""""\x5D""""\x20""""\x07""""\x5B""man""\x07""""\x5D""""\x20""goes""\x20""from""\x20""here."
            ));

        TUtf16String t;

        t = sample;
        CutPassage(t, 10, 0, u"<<<", u">>>");
        UNIT_ASSERT(t == u"<<<\007[clever\007] \007[man\007]>>>");

        t = sample;
        CutPassage(t, 10, 1, u"<<<", u">>>");
        UNIT_ASSERT(t == u"<<<\007[clever\007]>>>");

        t = u"just text without highlights";
        CutPassage(t, 10, 0, u"<<<", u">>>");
        UNIT_ASSERT(t == u"just text>>>");

        t = u"just text without highlights";
        CutPassage(t, 10, 1, u"<<<", u">>>");
        UNIT_ASSERT(t == u"just>>>");
    }

    void test_replace_hilites() {
        {
            TUtf16String str = u"Hello \x07[world\x07]";
            ReplaceHilites(str, true);
            UNIT_ASSERT(str == u"Hello <b><font color = #a00000>world</font></b>");
        }
        {
            TUtf16String str = u"Hello \x07[world";
            ReplaceHilites(str, true);
            UNIT_ASSERT(str == u"Hello world");
        }
        {
            TUtf16String str = u"Hello \x07]world";
            ReplaceHilites(str, true);
            UNIT_ASSERT(str == u"Hello world");
        }
        {
            TUtf16String str = u"Hello \x07]\x07]world";
            ReplaceHilites(str, true);
            UNIT_ASSERT(str == u"Hello world");
        }
        {
            TUtf16String str = u"Hello \x07]\x07[world";
            ReplaceHilites(str, true);
            UNIT_ASSERT(str == u"Hello world");
        }
        {
            TUtf16String str = u"Hello \x07]world! \x07[Hello\x07]!";
            ReplaceHilites(str, true);
            UNIT_ASSERT(str == u"Hello world! <b><font color = #a00000>Hello</font></b>!");
        }
        {
            TString str = "Hello \x07[world\x07]";
            ReplaceHilites(str, true);
            UNIT_ASSERT(str == "Hello <b><font color = #a00000>world</font></b>");
        }
        {
            TString str = "Hello \x07[world";
            ReplaceHilites(str, true);
            UNIT_ASSERT(str == "Hello world");
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TPHilightTest);

