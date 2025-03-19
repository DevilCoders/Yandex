#include <library/cpp/testing/unittest/registar.h>

#include <util/system/defaults.h>

#include "keyconv.h"

Y_UNIT_TEST_SUITE(TQEFlexIndexKeyConvTest) {
    void TestSingleQuery(TFormToKeyConvertorWithCheck& keyConv, const char* query, bool isGood, bool checkEquality = true)
    {
        if (isGood) {
            if (checkEquality)
                UNIT_ASSERT_STRINGS_EQUAL(keyConv.ConvertUTF8(query, strlen(query)), query);
        } else {
            UNIT_ASSERT_EXCEPTION(keyConv.ConvertUTF8(query, strlen(query)), yexception);
        }
    }

    Y_UNIT_TEST(QueryToKeyConversion) {
        TFormToKeyConvertorWithCheck keyConv;

        TestSingleQuery(keyConv, "ask", true);
        TestSingleQuery(keyConv, "?ask", false);
        TestSingleQuery(keyConv, "(ask    )", false);
        TestSingleQuery(keyConv, "ask\tme", false);
        TestSingleQuery(keyConv, "got you ?", true);
        TestSingleQuery(keyConv, "     some extra space  ", true);
        TestSingleQuery(keyConv, "it is a very loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
                    "oooooooooooooooo... ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
                    "oooooooooooooo... ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo...ooong string.", false);
        TestSingleQuery(keyConv, "\"it is a very loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
                    "oooooooooooooooo... ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
                    "oooooooooooooo... ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo...ooong string.", false);

        TestSingleQuery(keyConv, "some ""\x7f""text", false);
        TestSingleQuery(keyConv, "русский текст", true, false);
        TestSingleQuery(keyConv, "türkçe olanlar", true, false);
    }
}


