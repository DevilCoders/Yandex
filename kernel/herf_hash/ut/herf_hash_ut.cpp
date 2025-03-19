#include <kernel/herf_hash/herf_hash.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NHerfHash;

Y_UNIT_TEST_SUITE(THerfHashTest) {

    Y_UNIT_TEST(TestHashPack) {
        const TString twitterCom = "twitter.com";
        const TString wwwFacebookCom = "www.facebook.com";
        const TString ruRuFacebookCom = "ru-ru.facebook.com";
        const TString trTrFacebookCom = "tr-tr.facebook.com";
        const TString facebookCom = "facebook.com";

        const ui32 hTwitterCom = 4045781102; // "twitter.com"
        const ui32 oTwitterCom = 4045781102; // "twitter.com"
        const ui32 hWwwFacebookCom = 3426277159; // "www.facebook.com"
        const ui32 hRuRuFacebookCom = 4114450021; // "ru-ru.facebook.com"
        const ui32 hTrTrFacebookCom = 2632202393; // "tr-tr.facebook.com"
        const ui32 oFacebookCom = 581868039; // "facebook.com"

        UNIT_ASSERT_VALUES_EQUAL(HostHash(twitterCom), hTwitterCom);
        UNIT_ASSERT_VALUES_EQUAL(OwnerHash(twitterCom), oTwitterCom);
        UNIT_ASSERT_VALUES_EQUAL(HostHash(wwwFacebookCom), hWwwFacebookCom);
        UNIT_ASSERT_VALUES_EQUAL(HostHash(ruRuFacebookCom), hRuRuFacebookCom);
        UNIT_ASSERT_VALUES_EQUAL(HostHash(trTrFacebookCom), hTrTrFacebookCom);
        UNIT_ASSERT_VALUES_EQUAL(OwnerHash(facebookCom), oFacebookCom);
    }
}
