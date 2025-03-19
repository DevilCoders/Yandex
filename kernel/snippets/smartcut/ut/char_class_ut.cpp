#include <kernel/snippets/smartcut/char_class.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NSnippets {

Y_UNIT_TEST_SUITE(TCharClassTests) {
    Y_UNIT_TEST(TestIsLeftQuoteOrBracket) {
        TUtf16String left = u"([{\"'«„“‟‵";
        for (wchar16 c : TWtringBuf(left)) {
            UNIT_ASSERT_EQUAL_C(IsLeftQuoteOrBracket(c), true, c);
        }
        TUtf16String notLeft = u"ZЩ *$)]}»”";
        for (wchar16 c : TWtringBuf(notLeft)) {
            UNIT_ASSERT_EQUAL_C(IsLeftQuoteOrBracket(c), false, c);
        }
    }

    Y_UNIT_TEST(TestIsRightQuoteOrBracket) {
        TUtf16String right = u")]}\"'»”›’";
        for (wchar16 c : TWtringBuf(right)) {
            UNIT_ASSERT_EQUAL_C(IsRightQuoteOrBracket(c), true, c);
        }
        TUtf16String notRight = u" ЙY&([{«‟‹‘‛";
        for (wchar16 c : TWtringBuf(notRight)) {
            UNIT_ASSERT_EQUAL_C(IsRightQuoteOrBracket(c), false, c);
        }
        // Special case: Left Double Quotation Mark “
        UNIT_ASSERT_EQUAL(IsRightQuoteOrBracket(wchar16(0x201C)), true);
    }
}


}
