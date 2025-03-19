#include "date_recognizer.h"

#include <library/cpp/charset/recyr.hh>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(DateRecognizerTest) {
    Y_UNIT_TEST(TestSimple) {
        TStreamDateRecognizer recognizer;
        recognizer.Push(RecodeToYandex(CODES_UTF8, "Сегодня 18 АВГУСТА 2019 г."));
        TRecognizedDate date;
        UNIT_ASSERT(recognizer.GetDate(&date));
        UNIT_ASSERT_VALUES_EQUAL(date.Year, 2019);
        UNIT_ASSERT_VALUES_EQUAL(date.Month, 8);
        UNIT_ASSERT_VALUES_EQUAL(date.Day, 18);
    }
}
