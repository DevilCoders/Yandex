#include <library/cpp/string_utils/base64/base64.h>

#include <library/cpp/testing/unittest/registar.h>

#include "uilangdetect.h"
#include "bycookie.h"
#include "byacceptlang.h"
#include "bytld.h"

Y_UNIT_TEST_SUITE(UILangTests) {
    Y_UNIT_TEST(LangByTLDTest) {
        UNIT_ASSERT_EQUAL(LANG_ENG, LanguageByTLD("com"));
        UNIT_ASSERT_EQUAL(LANG_RUS, LanguageByTLD("xn--p1ai"));
        UNIT_ASSERT_EQUAL(LANG_RUS, LanguageByTLD("ru"));
        UNIT_ASSERT_EQUAL(LANG_POR, LanguageByTLD("gw"));
    }
    Y_UNIT_TEST(LangByMyCookieTest) {
        TString cookie;

        UNIT_ASSERT_EQUAL(LANG_UNK, LanguageByMyCookie(""));
        UNIT_ASSERT_EQUAL(LANG_UNK, LanguageByMyCookie("1234567"));
        UNIT_ASSERT_EQUAL(LANG_RUS, LanguageByMyCookie("YxoBASID4ABnFeAAZxUAIwIDBScCAAEtAQI2AQEA"));

        cookie = Base64Encode(TStringBuf("\x63\x27\x02\x02\x02"sv));
        UNIT_ASSERT_STRINGS_EQUAL("YycCAgI=", cookie.data());
        UNIT_ASSERT_EQUAL(LANG_UKR, LanguageByMyCookie(cookie));

        cookie = Base64Encode(TStringBuf("\x63\x27\x02\x01\x01"sv));
        UNIT_ASSERT_STRINGS_EQUAL("YycCAQE=", cookie.data());
        UNIT_ASSERT_EQUAL(LANG_RUS, LanguageByMyCookie(cookie));
        //                                hd  bl0 bl1 bl2     bl3         bl4         BL5 N=3     L=7     bl6
        cookie = Base64Encode(TStringBuf("\x63\x00\x00\x03\x00\x03\x01\x7F\x01\x01\x03\x27\x03\x00\x07\x04\x23\x01\x34", 19));
        UNIT_ASSERT_EQUAL(LANG_AZE, LanguageByMyCookie(cookie));

        cookie = Base64Encode(TStringBuf("\x63\x00\x00\x03\x00\x03\x01\x7F\x01\x01\x03\x27\x03", 13));
        UNIT_ASSERT_EQUAL(LANG_UNK, LanguageByMyCookie(cookie));
    }
    Y_UNIT_TEST(LangByAcceptLangTest) {
        const ELanguage good[] = {LANG_RUS, LANG_UKR, LANG_ENG, LANG_KAZ, LANG_BEL, LANG_TAT, LANG_TUR};
        const TVector<ELanguage> goodLanguages(good, good + Y_ARRAY_SIZE(good));

        UNIT_ASSERT_EQUAL(LANG_UKR, LanguageByAcceptLanguage("uk,ru;q=0.8,tt;q=0.3", LANG_RUS, goodLanguages));
        UNIT_ASSERT_EQUAL(LANG_TAT, LanguageByAcceptLanguage("tt,ru;q=0.8,en-us;q=0.5,en;q=0.3", LANG_RUS, goodLanguages));
        UNIT_ASSERT_EQUAL(LANG_RUS, LanguageByAcceptLanguage("ru,tt;q=0.8,en;q=0.3", LANG_RUS, goodLanguages));
        UNIT_ASSERT_EQUAL(LANG_RUS, LanguageByAcceptLanguage("tt,ru,en-us;q=0.3", LANG_RUS, goodLanguages));
        UNIT_ASSERT_EQUAL(LANG_RUS, LanguageByAcceptLanguage("tt,ru-ru,ru;q=0.8,en-us;q=0.3", LANG_RUS, goodLanguages));
        UNIT_ASSERT_EQUAL(LANG_RUS, LanguageByAcceptLanguage("tt,ru-ru,ru;q=0.8,en-us;q=0.3", LANG_RUS, goodLanguages));
        UNIT_ASSERT_EQUAL(LANG_RUS, LanguageByAcceptLanguage("uk,ru,*;q=0.3", LANG_RUS, goodLanguages));
        UNIT_ASSERT_EQUAL(LANG_ENG, LanguageByAcceptLanguage("en,tr;q=0.3", LANG_TUR, goodLanguages));
        UNIT_ASSERT_EQUAL(LANG_ENG, LanguageByAcceptLanguage("en,tr;q=0.3", LANG_BEL, goodLanguages));
        UNIT_ASSERT_EQUAL(LANG_ENG, LanguageByAcceptLanguage("en,he;q=0.3", LANG_UKR, goodLanguages));
        UNIT_ASSERT_EQUAL(LANG_ENG, LanguageByAcceptLanguage("en", LANG_UKR, goodLanguages));
    }
    Y_UNIT_TEST(TLDByHostTest) {
        UNIT_ASSERT_EQUAL("ua", TLDByHost("yandex.ua"));
    }
    Y_UNIT_TEST(DetectUILangTest) {
        TString l10n0("");
        TString l10nIt("it");
        TString l10nTr("tr");

        TString my0 = "";
        //                              language code is here ->[  ][  ]
        TString myRU = Base64Encode(TStringBuf("\x63\x27\x02\x01\x01"sv)).data();
        TString myUK = Base64Encode(TStringBuf("\x63\x27\x02\x02\x02"sv)).data();
        TString myTT = Base64Encode(TStringBuf("\x63\x27\x02\x06\x06"sv)).data();

        TString accLang0 = "";
        TString accLangTTRU = "tt,ru;q=0.8,en-us;q=0.5,en;q=0.3";
        TString accLangRUTT = "ru,tt;q=0.8,en;q=0.3";
        TString accLangTRUT = "tt,ru,en-us;q=0.3";
        TString accLangUKRU = "uk,ru;q=0.8,*;q=0.3";
        TString accLangURUK = "uk,ru,*;q=0.3";
        TString accLangUKTTRU = "uk,tt;q=0.9,ru;q=0.8,en-us;q=0.5,en;q=0.3";

        ELanguage good1[] = {LANG_TAT, LANG_TUR, LANG_RUS, LANG_ARA};
        const TVector<ELanguage> goodLanguages1(good1, good1 + Y_ARRAY_SIZE(good1));
        ELanguage good2[] = {LANG_POR, LANG_ARA};
        const TVector<ELanguage> goodLanguages2(good2, good2 + Y_ARRAY_SIZE(good2));

        UNIT_ASSERT_EQUAL(LANG_ENG, DetectUserLanguage("com", l10n0, my0, accLang0));
        UNIT_ASSERT_EQUAL(LANG_ENG, DetectUserLanguage("com", l10n0, myRU, accLangTTRU));
        UNIT_ASSERT_EQUAL(LANG_TUR, DetectUserLanguage("com", l10nTr, myRU, accLangTTRU));
        UNIT_ASSERT_EQUAL(LANG_ENG, DetectUserLanguage("com", l10nIt, myUK, accLangRUTT));

        UNIT_ASSERT_EQUAL(LANG_RUS, DetectUserLanguage("ru", l10n0, myRU, accLangTTRU));
        UNIT_ASSERT_EQUAL(LANG_UKR, DetectUserLanguage("ru", l10n0, myUK, accLangRUTT));
        UNIT_ASSERT_EQUAL(LANG_TAT, DetectUserLanguage("ru", l10n0, myTT, accLangUKRU));
        UNIT_ASSERT_EQUAL(LANG_RUS, DetectUserLanguage("ru", l10n0, my0, accLang0));
        UNIT_ASSERT_EQUAL(LANG_TAT, DetectUserLanguage("ru", l10n0, my0, accLangTTRU));
        UNIT_ASSERT_EQUAL(LANG_RUS, DetectUserLanguage("ru", l10n0, my0, accLangRUTT));
        UNIT_ASSERT_EQUAL(LANG_RUS, DetectUserLanguage("ru", l10n0, my0, accLangTRUT));
        UNIT_ASSERT_EQUAL(LANG_UKR, DetectUserLanguage("ru", l10n0, my0, accLangUKRU));
        UNIT_ASSERT_EQUAL(LANG_RUS, DetectUserLanguage("ru", l10n0, my0, accLangURUK));

        UNIT_ASSERT_EQUAL(LANG_UKR, DetectUserLanguage("ua", l10n0, my0, accLangUKRU));
        UNIT_ASSERT_EQUAL(LANG_UKR, DetectUserLanguage("ua", l10n0, my0, accLangURUK));
        UNIT_ASSERT_EQUAL(LANG_TAT, DetectUserLanguage("ua", l10n0, myTT, accLangUKRU));

        UNIT_ASSERT_EQUAL(LANG_TUR, DetectUserLanguage("tr", l10n0, my0, accLang0));
        UNIT_ASSERT_EQUAL(LANG_TUR, DetectUserLanguage("tr", l10n0, my0, accLangTTRU));
        UNIT_ASSERT_EQUAL(LANG_TUR, DetectUserLanguage("tr", l10n0, my0, accLangUKRU));

        UNIT_ASSERT_EQUAL(LANG_TAT, DetectUserLanguage("ru", l10n0, my0, accLangUKTTRU, goodLanguages1));
        UNIT_ASSERT_EQUAL(LANG_BEL, DetectUserLanguage("by", l10n0, my0, accLangUKTTRU, goodLanguages2));
        UNIT_ASSERT_EQUAL(LANG_TUR, DetectUserLanguage("yandex.com.tr", l10n0, my0, accLangRUTT));
    }
    Y_UNIT_TEST(DetectUILangRDTest) {
        TServerRequestData rd;

        //kazakh in l10n
        {
            rd.Clear();
            rd.AddHeader("Host", "m.yandex.ru:80");
            rd.AddHeader("Cookie", "yandexuid=9541801300375625; my=YycCAgI=; fuid01=4d806ff42d74e89a.tFTz1C32k3bOD11xbQS9TexfDvyUzjJJy155Wxbr44JuXNd-mY4_FfeQ7kkR9jsmscTpcMmEemP3tjEi3yBfcYxL_ZZZ72VSQ9YlBZf_9NOzHoalATXx1zWd6pRx0R5W");
            rd.AddHeader("Accept-Language", "tt,ru;q=0.8,en-us;q=0.5,en;q=0.3");
            rd.Parse("/yandsearch?text=bamboo&l10n=kk&lr=213");
            rd.Scan();
            UNIT_ASSERT_EQUAL(LANG_KAZ, DetectUserLanguage(rd));
        }

        //ukrainian in cookie
        {
            rd.Clear();
            rd.AddHeader("Host", "m.yandex.ru:80");
            rd.AddHeader("Cookie", "yandexuid=9541801300375625; my=YycCAgI=; fuid01=4d806ff42d74e89a.tFTz1C32k3bOD11xbQS9TexfDvyUzjJJy155Wxbr44JuXNd-mY4_FfeQ7kkR9jsmscTpcMmEemP3tjEi3yBfcYxL_ZZZ72VSQ9YlBZf_9NOzHoalATXx1zWd6pRx0R5W");
            rd.AddHeader("Accept-Language", "tt,ru;q=0.8,en-us;q=0.5,en;q=0.3");
            rd.Parse("/yandsearch?text=bamboo&lr=213");
            rd.Scan();
            UNIT_ASSERT_EQUAL(LANG_UKR, DetectUserLanguage(rd));
        }

        //tatar in accept-language
        {
            rd.Clear();
            rd.AddHeader("Host", "m.yandex.ru:80");
            rd.AddHeader("Cookie", "yandexuid=9541801300375625; fuid01=4d806ff42d74e89a.tFTz1C32k3bOD11xbQS9TexfDvyUzjJJy155Wxbr44JuXNd-mY4_FfeQ7kkR9jsmscTpcMmEemP3tjEi3yBfcYxL_ZZZ72VSQ9YlBZf_9NOzHoalATXx1zWd6pRx0R5W");
            rd.AddHeader("Accept-Language", "tt,ru;q=0.8,en-us;q=0.5,en;q=0.3");
            rd.Parse("/yandsearch?text=bamboo&lr=213");
            rd.Scan();
            UNIT_ASSERT_EQUAL(LANG_TAT, DetectUserLanguage(rd));
        }
    }
}
