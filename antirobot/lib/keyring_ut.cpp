#include <library/cpp/testing/unittest/registar.h>

#include "keyring.h"

using namespace NAntiRobot;

Y_UNIT_TEST_SUITE(TKeyRingTest) {
    static const TStringBuf data1 = "data11";

    Y_UNIT_TEST(TestMoreThanSevenKeys) {
        TString keys("qwerty1\nqwerty2\nqwerty3\nqwerty4\nqwerty5\nqwerty6\nqwerty7\nqwerty8");
        TStringInput keyStream(keys);
        TKeyRing rng(keyStream);

        const auto sign = rng.SignHex(data1);

        UNIT_ASSERT_VALUES_EQUAL(sign, TStringBuf("0d8250e4d894d61cc6924570ab6b7810"));
        UNIT_ASSERT(rng.IsSignedHex(data1, sign));
    }

    Y_UNIT_TEST(TestLessThanSevenKeys) {
        TString keys("qwerty1\nqwerty2\nqwerty3");
        TStringInput keyStream(keys);
        TKeyRing rng(keyStream);

        const auto sign = rng.SignHex(data1);

        UNIT_ASSERT_VALUES_EQUAL(sign, TStringBuf("06c8e32004753046e90d93601cfee5cf"));
        UNIT_ASSERT(rng.IsSignedHex(data1, sign));
    }

    Y_UNIT_TEST(TestNotExpiredYetKey) {
        TString keys1("qwerty1\nqwerty2\nqwerty3\nqwerty4\nqwerty5\nqwerty6\nqwerty7\nqwerty8");
        TString keys2("qwerty2\nqwerty3\nqwerty4\nqwerty5\nqwerty6\nqwerty7\nqwerty8\nqwerty9");
        TStringInput keyStream1(keys1);
        TStringInput keyStream2(keys2);
        TKeyRing rng1(keyStream1);
        TKeyRing rng2(keyStream2);

        const auto sign = rng1.SignHex(data1);

        UNIT_ASSERT(rng1.IsSignedHex(data1, sign));
        UNIT_ASSERT(rng2.IsSignedHex(data1, sign));
    }

    Y_UNIT_TEST(TestExpiredKey) {
        TString keys1("qwerty1\nqwerty2\nqwerty3\nqwerty4\nqwerty5\nqwerty6\nqwerty7\nqwerty8");
        TString keys2("qwerty3\nqwerty4\nqwerty5\nqwerty6\nqwerty7\nqwerty8\nqwerty9\nqwerty10");
        TStringInput keyStream1(keys1);
        TStringInput keyStream2(keys2);
        TKeyRing rng1(keyStream1);
        TKeyRing rng2(keyStream2);

        const auto sign = rng1.SignHex(data1);

        UNIT_ASSERT(rng1.IsSignedHex(data1, sign));
        UNIT_ASSERT(!rng2.IsSignedHex(data1, sign));
    }

    Y_UNIT_TEST(TestInvalidSignature) {
        TString keys("qwerty1\nqwerty2\nqwerty3\nqwerty4\nqwerty5\nqwerty6\nqwerty7\nqwerty8");
        TStringInput keyStream(keys);
        TKeyRing rng(keyStream);

        const auto sign = rng.SignHex(data1);

        UNIT_ASSERT(rng.IsSignedHex(data1, sign));
        UNIT_ASSERT(!rng.IsSignedHex(data1, "Z" + sign.substr(1)));
    }
}
