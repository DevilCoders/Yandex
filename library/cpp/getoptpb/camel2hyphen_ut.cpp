#include <library/cpp/testing/unittest/registar.h>

#include "camel2hyphen.h"

using namespace NGetoptPb;

class TCamelToHyphenTest: public TTestBase {
    UNIT_TEST_SUITE(TCamelToHyphenTest);
    UNIT_TEST(Test);
    UNIT_TEST_SUITE_END();

private:
    void Test();
};

UNIT_TEST_SUITE_REGISTRATION(TCamelToHyphenTest)

void TCamelToHyphenTest::Test() {
    TString str0 = "";
    TString str1 = "alllower";
    TString str2 = "PlainCamel3Case";
    TString str3 = "ALL_CAPITAL";
    TString str4 = "TActorID";
    TString str5 = "-With-Hyphens- and  ..Space6s -test--";

    UNIT_ASSERT_EQUAL(CamelToHyphen(str0), "");
    UNIT_ASSERT_EQUAL(CamelToHyphen(str1), "alllower");
    UNIT_ASSERT_EQUAL(CamelToHyphen(str2), "plain-camel3-case");
    UNIT_ASSERT_EQUAL(CamelToHyphen(str3), "all-capital");
    UNIT_ASSERT_EQUAL(CamelToHyphen(str4), "tactor-id"); // TODO what can i do?
    UNIT_ASSERT_EQUAL(CamelToHyphen(str5), "with-hyphens-and-space6s-test");
}
