#include <kernel/snippets/smartcut/hilited_length.h>
#include <kernel/snippets/strhl/hilite_mark.h>
#include <kernel/snippets/strhl/glue_common.h>
#include <kernel/snippets/strhl/zonedstring.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <initializer_list>
#include <utility>

namespace NSnippets {

    static void AddHiliteZones(TZonedString& str, const std::initializer_list<std::pair<size_t, size_t>> boldSpans) {
        auto& boldZones = str.Zones[+TZonedString::ZONE_UNKNOWN];
        boldZones.Mark = &DEFAULT_MARKS;
        for (const auto& boldSpan : boldSpans) {
            TWtringBuf span(str.String.data() + boldSpan.first, boldSpan.second);
            boldZones.Spans.push_back(TZonedString::TSpan(span));
        }
    }

    static const THiliteMark LINK_MARK = THiliteMark(u"\07+", u"\07-");

    static void AddLinkZones(TZonedString& str, const std::initializer_list<std::pair<size_t, size_t>> boldSpans) {
        auto& boldZones = str.Zones[+TZonedString::ZONE_LINK];
        boldZones.Mark = &LINK_MARK;
        for (const auto& boldSpan : boldSpans) {
            TWtringBuf span(str.String.data() + boldSpan.first, boldSpan.second);
            boldZones.Spans.push_back(TZonedString::TSpan(span));
        }
    }

Y_UNIT_TEST_SUITE(THilitedLengthTests) {
    Y_UNIT_TEST(TestGetHilitedTextLengthInSymbols) {
        TZonedString empty;
        TZonedString regular = TUtf16String(u"abc");
        TZonedString bold = TUtf16String(u"abc def07");
        AddHiliteZones(bold, {{4, 3}, {7, 2}});
        TZonedString linkInText = TUtf16String(u"abc link07");
        AddLinkZones(linkInText, {{4, 4}});
        AddHiliteZones(linkInText, {{8, 2}});

        UNIT_ASSERT_EQUAL(GetHilitedTextLengthInSymbols({}), 0);
        UNIT_ASSERT_EQUAL(GetHilitedTextLengthInSymbols({empty}), 0);

        // "abc"
        UNIT_ASSERT_EQUAL(GetHilitedTextLengthInSymbols({regular}), 3);

        // "abc ... abc"
        UNIT_ASSERT_EQUAL(GetHilitedTextLengthInSymbols({regular, regular}), 11);

        // "abc ... "
        UNIT_ASSERT_EQUAL(GetHilitedTextLengthInSymbols({regular, empty}), 8);

        // "abc def07"
        UNIT_ASSERT_EQUAL(GetHilitedTextLengthInSymbols({bold}), 9);

        // "abc def07 ... abc"
        UNIT_ASSERT_EQUAL(GetHilitedTextLengthInSymbols({bold, regular}), 17);

        // "abc link07"
        UNIT_ASSERT_EQUAL(GetHilitedTextLengthInSymbols({linkInText}), 10);
        UNIT_ASSERT_STRINGS_EQUAL(WideToASCII(MergedGlue(linkInText)), "abc \07+link\07-\07[07\07]");
    }

    Y_UNIT_TEST(TestGetHilitedTextLengthInRows) {
        TZonedString empty;
        TZonedString regular = TUtf16String(u"abc def");
        TZonedString bold = TUtf16String(u"abc def");
        AddHiliteZones(bold, {{0, 3}, {4, 3}});
        TZonedString full = TUtf16String(u"abc def ... abc def");
        AddHiliteZones(full, {{8, 3}, {12, 3}, {16, 3}});
        TZonedString boldLastWord = TUtf16String(u"def");
        AddHiliteZones(boldLastWord, {{0, 3}});

        UNIT_ASSERT_EQUAL(GetHilitedTextLengthInRows({}, 500, 16), 0);
        UNIT_ASSERT_EQUAL(GetHilitedTextLengthInRows({empty}, 500, 16), 0);

        // abc def
        float regularLen = GetHilitedTextLengthInRows({regular}, 500, 16);
        UNIT_ASSERT(0 < regularLen && regularLen < 1);

        // [abc] [def]
        float boldLen = GetHilitedTextLengthInRows({bold}, 500, 16);
        UNIT_ASSERT(regularLen + 0.001 < boldLen && boldLen < 1);

        // abc def [...] [abc] [def]
        float len1 = GetHilitedTextLengthInRows({full}, 500, 16);
        UNIT_ASSERT(regularLen + boldLen < len1 && len1 < 1);
        float len2 = GetHilitedTextLengthInRows({regular, bold}, 500, 16);
        UNIT_ASSERT_DOUBLES_EQUAL(len1, len2, 1e-4);

        // abc def [...] [abc]
        // [def]
        float len3 = GetHilitedTextLengthInRows({full}, 110, 16);
        UNIT_ASSERT(1 < len3 && len3 < 2);
        float len4 = GetHilitedTextLengthInRows({regular, bold}, 110, 16);
        UNIT_ASSERT_DOUBLES_EQUAL(len3, len4, 1e-4);

        // Break before the ellipsis in a text is allowed:
        // abc def
        // [...] [abc]
        // [def]
        float len5 = GetHilitedTextLengthInRows({full}, 70, 16);
        float len6 = GetHilitedTextLengthInRows({boldLastWord}, 70, 16);
        UNIT_ASSERT_DOUBLES_EQUAL(len5, 2 + len6, 1e-4);

        // But break is not allowed before the ellipsis as a fragment separator:
        // abc
        // def [...]
        // [abc] [def]
        float len7 = GetHilitedTextLengthInRows({regular, bold}, 70, 16);
        float len8 = GetHilitedTextLengthInRows({bold}, 70, 16);
        UNIT_ASSERT(0 < len8 && len8 < 1);
        UNIT_ASSERT_DOUBLES_EQUAL(len7, 2 + len8, 1e-4);

        // "abc link 07"
        TZonedString linkInText = TUtf16String(u"ifj ifjk if");
        AddHiliteZones(linkInText, {{9, 2}});
        float len9 = GetHilitedTextLengthInRows({linkInText}, 70, 16);
        AddLinkZones(linkInText, {{4, 4}});
        float len10 = GetHilitedTextLengthInRows({linkInText}, 70, 16);
        UNIT_ASSERT_DOUBLES_EQUAL(len9, len10, 1e-4);
        UNIT_ASSERT_STRINGS_EQUAL(WideToASCII(MergedGlue(linkInText)), "ifj \07+ifjk\07- \07[if\07]");
        AddHiliteZones(linkInText, {{4, 4}});
        linkInText.Normalize();
        float len11 = GetHilitedTextLengthInRows({linkInText}, 70, 16);
        UNIT_ASSERT(len11 > len10 + 1e-4);
        UNIT_ASSERT_STRINGS_EQUAL(WideToASCII(MergedGlue(linkInText)), "ifj \07+\07[ifjk\07]\07- \07[if\07]");
    }
}

}
