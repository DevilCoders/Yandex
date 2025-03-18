//
// Created by Maxim Khardin on 19/04/2018.
//

#include <antiadblock/encrypter/encryption.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(SmokeTest) {
    Y_UNIT_TEST(EncryptionDecryption) {
        using namespace NAntiAdBlock;
        auto cookie = TString("328109941410122142");
        UNIT_ASSERT_VALUES_EQUAL(cookie, decryptCookie(encryptCookie(cookie, NIcookie::GetDefaultYandexKeys()).GetRef(), NIcookie::GetDefaultYandexKeys()));
    }

    Y_UNIT_TEST(RealCrookieDecryption) {
        using namespace NAntiAdBlock;
        const TString cookie = TString("6731871211515776902");
        const TString crookie = TString("kPAqJd7N7Q28bqvvnA8DTGP2tjTMLWOILs8LTJt1fFDjS+U9q0AyvN2uclNYsEt4IgszZF4zolu3D6EKXuPhUBG0CBE=");
        UNIT_ASSERT_VALUES_EQUAL(cookie, decryptCrookieWithDefaultKey(crookie));
    }

    Y_UNIT_TEST(InvalidCrookieDecryption) {
        using namespace NAntiAdBlock;
        UNIT_ASSERT_VALUES_EQUAL(decryptCrookieWithDefaultKey("").Defined(), false);
        UNIT_ASSERT_VALUES_EQUAL(decryptCrookieWithDefaultKey("not valid crookie").Defined(), false);
    }
};
