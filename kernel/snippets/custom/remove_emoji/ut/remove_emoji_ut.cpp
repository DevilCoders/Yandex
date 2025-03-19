#include <kernel/snippets/custom/remove_emoji/remove_emoji.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>

namespace NSnippets {

Y_UNIT_TEST_SUITE(TRemoveEmojiTest) {
    Y_UNIT_TEST(TestRemoveEmoji) {
        TUtf16String emptyString;
        TUtf16String noEmoji = u"this 丠𓅂 🀪 is nöt €möوi!";
        TUtf16String emoji1 = u"ойё😂😂😂😂";
        TUtf16String emoji2 = u" 🚑 z 🚑 . z 🦁  ";
        TUtf16String emojiOnly = u"🦁😂";
        TUtf16String joinedEmoji = u"👨‍👩‍👧क्‍ष";

        UNIT_ASSERT_EQUAL(CountEmojis(emptyString), 0);
        UNIT_ASSERT_EQUAL(CountEmojis(noEmoji), 0);
        UNIT_ASSERT_EQUAL(CountEmojis(emoji1), 4);
        UNIT_ASSERT_EQUAL(CountEmojis(emoji2), 3);
        UNIT_ASSERT_EQUAL(CountEmojis(emojiOnly), 2);
        UNIT_ASSERT_EQUAL(CountEmojis(joinedEmoji), 3);

        TUtf16String noEmojiCopy = noEmoji;
        UNIT_ASSERT_EQUAL(RemoveEmojis(emptyString), false);
        UNIT_ASSERT_EQUAL(RemoveEmojis(noEmoji), false);
        UNIT_ASSERT_EQUAL(RemoveEmojis(emoji1), true);
        UNIT_ASSERT_EQUAL(RemoveEmojis(emoji2), true);
        UNIT_ASSERT_EQUAL(RemoveEmojis(emojiOnly), true);
        UNIT_ASSERT_EQUAL(RemoveEmojis(joinedEmoji), true);

        UNIT_ASSERT_EQUAL(emptyString, TUtf16String());
        UNIT_ASSERT_EQUAL(noEmoji, noEmojiCopy);
        UNIT_ASSERT_EQUAL(emoji1, u"ойё");
        UNIT_ASSERT_EQUAL(emoji2, u"z. z");
        UNIT_ASSERT_EQUAL(emojiOnly, TUtf16String());
        UNIT_ASSERT_EQUAL(joinedEmoji, u"क्‍ष");
    }
}

}
