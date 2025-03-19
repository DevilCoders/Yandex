#include "complex_id.h"

#include <kernel/urlnorm/urlnorm.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/singleton.h>
#include <util/string/vector.h>
#include <util/stream/format.h>
#include <util/digest/fnv.h>

class TTestBuilder: public TComplexIdBuilder {
public:
    TTestBuilder()
        : TComplexIdBuilder(SplitString("narod.ru|li.ru/users/ [a-zA-Z0-9\\.\\-_]+", "|"))
    {
    }

    TTestBuilder(const TVector<TString>& areas)
        : TComplexIdBuilder(areas)
    {
    }
};

Y_UNIT_TEST_SUITE(ComplexIdTest)
{
    Y_UNIT_TEST(TestCompare) {
        TComplexId a(0xff00, 0x0001), b(0x0001, 0x0000);
        UNIT_ASSERT(a < b);
        TComplexId c(0xff00, 0x0002);
        UNIT_ASSERT(a < c);
    }

    Y_UNIT_TEST(TestOwner)
    {
        TComplexId id = Default<TTestBuilder>().Do("ru");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0x8325607b4eb1a82));
        UNIT_ASSERT_EQUAL(id.Path, 0);
        id = Default<TTestBuilder>().Do("narod.ru");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0x796a7a74dab19a9e));
        UNIT_ASSERT_EQUAL(id.Path, 0);
        id = Default<TTestBuilder>().Do("vasya.narod.ru");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, 0);
        id = Default<TTestBuilder>().Do("vasya.narod.ru:80");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0));
        id = Default<TTestBuilder>().Do("www.vasya.narod.ru");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x6f4860247fd338d));
        id = Default<TTestBuilder>().Do("Www.Vasya.Narod.Ru");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x6f4860247fd338d));
    }

    Y_UNIT_TEST(TestSkipDefaults)
    {
        TComplexId id = Default<TTestBuilder>().Do("HTTP://www.vasya.narod.ru:80");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x6f4860247fd338d));
        id = Default<TTestBuilder>().Do("http://www.vasya.narod.ru:80/");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x34b58608aadeebac));
    }

    Y_UNIT_TEST(TestPath)
    {
        TComplexId id = Default<TTestBuilder>().Do("vasya.narod.ru/");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, 0x1);
        id = Default<TTestBuilder>().Do("www.vasya.narod.ru/");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x34b58608aadeebac));
        id = Default<TTestBuilder>().Do("www.vasya.narod.ru/foo?bar=jar");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0xa55c4662c4b7cd0a));
        id = Default<TTestBuilder>().Do("www.vasya.narod.ru:8013/foo?bar=jar");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x57f8b51a7ec863da));
        id = Default<TTestBuilder>().Do("www.vasya.narod.ru/foo?bar=jar&tar=rar");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x8ea79a77083d7b4f));
        id = Default<TTestBuilder>().Do("www.vasya.narod.ru/foo?tar=rar&bar=jar");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x8ea79a77083d7b4f));
    }

    Y_UNIT_TEST(TestScheme)
    {
        TComplexId id = Default<TTestBuilder>().Do("https://www.vasya.narod.ru");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0xe091072e41bdd8b8));
        id = Default<TTestBuilder>().Do("ftp://www.vasya.narod.ru");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0xe0445ee35be604b));
    }

    Y_UNIT_TEST(TestPathPrefix)
    {
        TComplexId id = Default<TTestBuilder>().Do("li.ru/users/");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0x2D7CCD6AC54BA7BD));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0xEF1463A99FB3334F));

        id = Default<TTestBuilder>().Do("http://li.ru/users/foo");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xFA548B2F6599547D));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0));

        id = Default<TTestBuilder>().Do("http://li.ru/users/foo/");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xFA548B2F6599547D));
        UNIT_ASSERT_EQUAL(id.Path, ULL(1));

        id = Default<TTestBuilder>().Do("http://li.ru/users/foo/foo?bar=jar#DEADBEEF");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xFA548B2F6599547D));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x5dfbfe89772c6c62));

        // This is a workaround for UrlHashVal() non-acceptance of strings without '/'
        id = Default<TTestBuilder>().Do("/?bar=jar#DEADBEEF");
        UNIT_ASSERT_EQUAL(id.Owner, 0);
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x87fd02bfd35247ea));

        // Check UrlHashVal() capabilities for slash-prepended paths
        id = Default<TTestBuilder>().Do("/?bar=jar&bar=jar#DEADBEEF");
        UNIT_ASSERT_EQUAL(id.Owner, 0);
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x87fd02bfd35247ea));

        id = Default<TTestBuilder>().Do("http://li.ru/users/foo?bar=jar#DEADBEEF");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xFA548B2F6599547D));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x87fd02bfd35247ea));
    }

    Y_UNIT_TEST(TestParts)
    {
        TComplexId id = Default<TTestBuilder>().Do("/foo?bar=jar#DEADBEEF");
        UNIT_ASSERT_EQUAL(id.Owner, 0);
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x5dfbfe89772c6c62));
        id = Default<TTestBuilder>().FromParts("http://", "/foo?bar=jar#DEADBEEF");
        UNIT_ASSERT_EQUAL(id.Owner, 0);
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x5dfbfe89772c6c62));
        id = Default<TTestBuilder>().Do("www.vasya.narod.ru/foo?bar=jar#DEADBEEF");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x1366b10e8a49987f));
        id = Default<TTestBuilder>().Do("www.", "vasya.narod.ru", ":80", "", ULL(0x4f63967c0ad130a2));
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x368bf2e8d689fd3f));
        id = Default<TTestBuilder>().FromParts("www.vasya.narod.ru", "/foo?bar=jar#DEADBEEF");
        UNIT_ASSERT_EQUAL(id.Owner, ULL(0xad471b26072c0a5c));
        UNIT_ASSERT_EQUAL(id.Path, ULL(0x1366b10e8a49987f));
    }
}
