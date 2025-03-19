#include <kernel/snippets/strhl/zonedstring.h>
#include <library/cpp/testing/unittest/registar.h>

class TZonedStringTest: public TTestBase
{
    UNIT_TEST_SUITE(TZonedStringTest);
        UNIT_TEST(testCopyCtor);
        UNIT_TEST(testSubstr);
    UNIT_TEST_SUITE_END();
public:
    void testCopyCtor();
    void testSubstr();
};

void TZonedStringTest::testCopyCtor()
{
    //init
    TZonedString zs;
    zs.String = u"One mcduck two scrooge three";
    const int ZONE_ID = 1;
    {
        TZonedString::TZone& zone = zs.Zones[ZONE_ID];
        zone.Spans.push_back(TZonedString::TSpan(TWtringBuf(zs.String.data() + 0, 3))); //One
        zone.Spans.push_back(TZonedString::TSpan(TWtringBuf(zs.String.data() + 4, 6))); //mcduck
        zone.Spans.push_back(TZonedString::TSpan(TWtringBuf(zs.String.data() + 11, 3))); //two
    }
    //test
    TZonedString zsCopy(zs);
    zs.Zones.clear();
    zs.String.clear();
    //assert
    TZonedString::TZone& zone = zsCopy.Zones[ZONE_ID];
    UNIT_ASSERT_EQUAL_C(zone.Spans.size(), 3, "Spans count");
    UNIT_ASSERT_NO_DIFF(WideToUTF8(zone.Spans[0].Span), "One");
    UNIT_ASSERT_NO_DIFF(WideToUTF8(zone.Spans[1].Span), "mcduck");
    UNIT_ASSERT_NO_DIFF(WideToUTF8(zone.Spans[2].Span), "two");
}

void TZonedStringTest::testSubstr() {
    //init
    TZonedString zs;
    TUtf16String w = u"One mcduck two scrooge three";
    zs.String = w;
    const int ZONE_ID = 1;
    {
        TZonedString::TZone& zone = zs.Zones[ZONE_ID];
        zone.Spans.push_back(TZonedString::TSpan(TWtringBuf(zs.String.data() + 0, 3))); //One
        zone.Spans.push_back(TZonedString::TSpan(TWtringBuf(zs.String.data() + 4, 6))); //mcduck
        zone.Spans.push_back(TZonedString::TSpan(TWtringBuf(zs.String.data() + 11, 3))); //two
        zone.Spans.push_back(TZonedString::TSpan(TWtringBuf(zs.String.data() + 15, 7))); //scrooge
    }
    //test
    TZonedString actual(zs.Substr(5, 15, u"...", u"!!!"));
    //assert
    UNIT_ASSERT_NO_DIFF(WideToUTF8(actual.String), "...cduck two scroo!!!");
    TZonedString::TZone& zone = actual.Zones[ZONE_ID];
    UNIT_ASSERT_EQUAL_C(zone.Spans.size(), 3, "Spans count");
    UNIT_ASSERT_NO_DIFF(WideToUTF8(zone.Spans[0].Span), "cduck");
    UNIT_ASSERT_NO_DIFF(WideToUTF8(zone.Spans[1].Span), "two");
    UNIT_ASSERT_NO_DIFF(WideToUTF8(zone.Spans[2].Span), "scroo");
}

UNIT_TEST_SUITE_REGISTRATION(TZonedStringTest);
