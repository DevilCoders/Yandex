#include <library/cpp/html/html5/tag.h>
#include <library/cpp/html/spec/tags.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TTagsTest) {
    // https://html.spec.whatwg.org/multipage/syntax.html#void-elements
    Y_UNIT_TEST(VoidElements) {
        using namespace NHtml;

        UNIT_ASSERT(FindTag(HT_AREA).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_BASE).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_BR).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_COL).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_EMBED).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_HR).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_IMG).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_INPUT).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_KEYGEN).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_LINK).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_MENUITEM).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_META).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_PARAM).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_SOURCE).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_TRACK).is(HT_empty));
        UNIT_ASSERT(FindTag(HT_WBR).is(HT_empty));
    }

    Y_UNIT_TEST(TagEnums) {
        using namespace NHtml5;

#define GET_ENUM(name) GetTagEnum(name, sizeof(name) - 1)
        UNIT_ASSERT_EQUAL(GET_ENUM("a"), TAG_A);
        UNIT_ASSERT_EQUAL(GET_ENUM("body"), TAG_BODY);
        UNIT_ASSERT_EQUAL(GET_ENUM("head"), TAG_HEAD);
        UNIT_ASSERT_EQUAL(GET_ENUM("html"), TAG_HTML);
        UNIT_ASSERT_EQUAL(GET_ENUM("table"), TAG_TABLE);
        UNIT_ASSERT_EQUAL(GET_ENUM("title"), TAG_TITLE);
        UNIT_ASSERT_EQUAL(GET_ENUM("amp-vk"), TAG_AMP_VK);

        UNIT_ASSERT_EQUAL(GET_ENUM("abra"), TAG_UNKNOWN);
        UNIT_ASSERT_EQUAL(GET_ENUM("teble"), TAG_UNKNOWN);
#undef GET_TAG
    }
}
