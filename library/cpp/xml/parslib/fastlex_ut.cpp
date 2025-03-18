#include <library/cpp/testing/unittest/registar.h>

#include "fastlex.h"

/*
 * just copy-paste it for good start point
 */

class TFastlexTest: public TTestBase {
    UNIT_TEST_SUITE(TFastlexTest);
    UNIT_TEST(TestUnsigned)
    UNIT_TEST(TestFloat)
    UNIT_TEST(TestBool);
    UNIT_TEST(TestString);
    UNIT_TEST(TestDateTime);
    UNIT_TEST(TestRFCDateTime);
    UNIT_TEST_SUITE_END();

private:
    void TestUnsigned() {
        TUnsignedParser p;
        UNIT_ASSERT_EQUAL(p.Parse("1234", 4), true);
        UNIT_ASSERT_EQUAL(p.Result(), 1234);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("88763", 5), true);
        UNIT_ASSERT_EQUAL(p.Result(), 88763);
        UNIT_ASSERT_EQUAL(p.Parse("44", 2), true);
        UNIT_ASSERT_EQUAL(p.Result(), 8876344);
        UNIT_ASSERT_EQUAL(p.Parse("a", 1), false);
    }
    void TestFloat() {
        TFloatParser p;
        UNIT_ASSERT_EQUAL(p.Parse("-1", 2), false); // this is "unsigned float"
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("1234.76", 7), true);
        UNIT_ASSERT_DOUBLES_EQUAL(p.Result(), 1234.76, 0.0000001);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("88763", 5), true);
        UNIT_ASSERT_EQUAL(p.Result(), 88763);
        UNIT_ASSERT_EQUAL(p.Parse(".44", 3), true);
        UNIT_ASSERT_DOUBLES_EQUAL(p.Result(), 88763.44, 0.0000001);
        UNIT_ASSERT_EQUAL(p.Parse("-", 1), false);
    }
    void TestBool() {
        TBoolParser p;
        UNIT_ASSERT_EQUAL(p.Parse("yes", 3), true);
        UNIT_ASSERT_EQUAL(p.Result(), 1);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("Fal", 3), true);
        UNIT_ASSERT_EQUAL(p.Parse("se", 2), true);
        UNIT_ASSERT_EQUAL(p.Result(), -1);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("ON", 2), true);
        UNIT_ASSERT_EQUAL(p.Result(), 1);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("0", 1), true);
        UNIT_ASSERT_EQUAL(p.Result(), -1);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("Shit", 4), false);
    }
    void TestString() {
        TStringComparer p("lorem ipsum");
        UNIT_ASSERT_EQUAL(p.Result(), false);
        p.Check("lorem ", 6);
        UNIT_ASSERT_EQUAL(p.Result(), false);
        p.Check("ipsum", 5);
        UNIT_ASSERT_EQUAL(p.Result(), true);
        p.Check("junk", 4);
        UNIT_ASSERT_EQUAL(p.Result(), false);
        p.Reset();
        p.Check("lorem ipsum", 11);
        UNIT_ASSERT_EQUAL(p.Result(), true);
    }
    void TestDateTime() {
        TDateTimeParser p;
        UNIT_ASSERT_EQUAL(p.Parse("1970-01-01T00:00:00Z", 20), true);
        UNIT_ASSERT_EQUAL(p.Result(), 0);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("1970-01-01T00:01:02Z", 20), true);
        UNIT_ASSERT_EQUAL(p.Result(), 62);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("1970-01-01", 10), true);
        UNIT_ASSERT_EQUAL(p.Result(), -3600 * 3);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("1970-01-02", 10), true);
        UNIT_ASSERT_EQUAL(p.Result(), 86400 - 3600 * 3);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("2009-02-14T02:31:30", 19), true);
        UNIT_ASSERT_EQUAL(p.Result(), 1234567890);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("2009-02-14T02", 13), true);
        UNIT_ASSERT_EQUAL(p.Parse(":31:30", 6), true);
        UNIT_ASSERT_EQUAL(p.Result(), 1234567890);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("2009-02-14t02:31:30", 19), true);
        UNIT_ASSERT_EQUAL(p.Result(), 1234567890);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("2009-02-14T02:31:30+0300", 24), true);
        UNIT_ASSERT_EQUAL(p.Result(), 1234567890);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("2009-02-14T02:31:30+03:00", 25), true);
        UNIT_ASSERT_EQUAL(p.Result(), 1234567890);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("2009-02-14Q02:31:30+03:00", 25), false);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("2009-02-14 02:31:30+03:00", 25), true);
    }
    void TestRFCDateTime() {
        TDateTimeParserRFC822 p;
        UNIT_ASSERT_EQUAL(p.Parse("Thu, 01 Jan 1970 03:00:00 +0300", 29), true);
        UNIT_ASSERT_EQUAL(p.Result(), 0);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("Sat, 14 Feb 2009 02:31:30 +0300", 29), true);
        UNIT_ASSERT_EQUAL(p.Result(), 1234567890);
        p.Init();
        UNIT_ASSERT_EQUAL(p.Parse("ababab", 6), false);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TFastlexTest);
