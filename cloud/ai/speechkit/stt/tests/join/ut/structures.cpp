//
// Created by Mikhail Yutman on 18.05.2020.
//

#include <cloud/ai/speechkit/stt/lib/join/naive_dp.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TTText) {
    TText textEmptyOnsets({"a", "b", "c"}, 3000);
    TText text({"a", "b", "c"}, 3000, {}, {0, 1000, 3000}, {0, 50, 1000, 1050, 3000, 3050});

    Y_UNIT_TEST(TestOnsetEmpty0) {
        UNIT_ASSERT_EQUAL(textEmptyOnsets.GetOnset(0), 3000);
    }

    Y_UNIT_TEST(TestOnsetEmpty1) {
        UNIT_ASSERT_EQUAL(textEmptyOnsets.GetOnset(1), 3000);
    }

    Y_UNIT_TEST(TestOnsetEmpty2) {
        UNIT_ASSERT_EQUAL(textEmptyOnsets.GetOnset(2), 3000);
    }

    Y_UNIT_TEST(TestOnset0) {
        UNIT_ASSERT_EQUAL(text.GetOnset(0), 3000);
    }

    Y_UNIT_TEST(TestOnset1) {
        UNIT_ASSERT_EQUAL(text.GetOnset(1), 4000);
    }

    Y_UNIT_TEST(TestOnset2) {
        UNIT_ASSERT_EQUAL(text.GetOnset(2), 6000);
    }

    Y_UNIT_TEST(TestLetterOnsetEmpty0) {
        UNIT_ASSERT_EQUAL(textEmptyOnsets.GetLetterOnset(0), 3000);
    }

    Y_UNIT_TEST(TestLetterOnsetEmpty1) {
        UNIT_ASSERT_EQUAL(textEmptyOnsets.GetLetterOnset(1), 3000);
    }

    Y_UNIT_TEST(TestLetterOnsetEmpty2) {
        UNIT_ASSERT_EQUAL(textEmptyOnsets.GetLetterOnset(2), 3000);
    }

    Y_UNIT_TEST(TestLetterOnsetEmpty3) {
        UNIT_ASSERT_EQUAL(textEmptyOnsets.GetLetterOnset(2), 3000);
    }

    Y_UNIT_TEST(TestLetterOnsetEmpty4) {
        UNIT_ASSERT_EQUAL(textEmptyOnsets.GetLetterOnset(2), 3000);
    }

    Y_UNIT_TEST(TestLetterOnsetEmpty5) {
        UNIT_ASSERT_EQUAL(textEmptyOnsets.GetLetterOnset(2), 3000);
    }

    Y_UNIT_TEST(TestLetterOnset0) {
        UNIT_ASSERT_EQUAL(text.GetLetterOnset(0), 3000);
    }

    Y_UNIT_TEST(TestLetterOnset1) {
        UNIT_ASSERT_EQUAL(text.GetLetterOnset(1), 3050);
    }

    Y_UNIT_TEST(TestLetterOnset2) {
        UNIT_ASSERT_EQUAL(text.GetLetterOnset(2), 4000);
    }

    Y_UNIT_TEST(TestLetterOnset3) {
        UNIT_ASSERT_EQUAL(text.GetLetterOnset(3), 4050);
    }

    Y_UNIT_TEST(TestLetterOnset4) {
        UNIT_ASSERT_EQUAL(text.GetLetterOnset(4), 6000);
    }

    Y_UNIT_TEST(TestLetterOnset5) {
        UNIT_ASSERT_EQUAL(text.GetLetterOnset(5), 6050);
    }
}
