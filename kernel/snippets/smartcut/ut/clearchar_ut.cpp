#include <kernel/snippets/smartcut/clearchar.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NSnippets {

Y_UNIT_TEST_SUITE(TClearCharTests) {
    Y_UNIT_TEST(TestIsPermittedEdgeChar) {
        TUtf16String permitted = u"#$£§€№";
        for (wchar16 c : TWtringBuf(permitted)) {
            UNIT_ASSERT_EQUAL_C(IsPermittedEdgeChar(c), true, c);
        }
        TUtf16String notPermitted = u"ZЩ5 *)%»'^";
        for (wchar16 c : TWtringBuf(notPermitted)) {
            UNIT_ASSERT_EQUAL_C(IsPermittedEdgeChar(c), false, c);
        }
    }

    TString DoTestClearChars(const TString& utf8Input) {
        TUtf16String str = UTF8ToWide(utf8Input);
        ClearChars(str);
        return WideToUTF8(str);
    }

    Y_UNIT_TEST(TestClearChars) {
        UNIT_ASSERT_STRINGS_EQUAL(DoTestClearChars(""), "");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestClearChars("abc"), "abc");
        TString longText = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
        UNIT_ASSERT_STRINGS_EQUAL(DoTestClearChars(longText), longText);
        UNIT_ASSERT_STRINGS_EQUAL(DoTestClearChars("*** Abc. ***"), "Abc.");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestClearChars("??? Abc! Abc? ???"), "Abc! Abc? ???");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestClearChars("C++"), "C++");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestClearChars("★ Abc! Abc...... ✪"), "Abc! Abc...");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestClearChars("Abc! Abc...... Xyz"), "Abc! Abc...... Xyz");
    }

    Y_UNIT_TEST(TestAllowBreve) {
        const wchar16 chars[] = {u'И', 0x306, 0};
        const TUtf16String input(chars);
        TUtf16String cleared = input;
        ClearChars(cleared, false, true);
        UNIT_ASSERT_EQUAL(cleared, input);
    }
}

}
