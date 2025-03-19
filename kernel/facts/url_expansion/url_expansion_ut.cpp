#include "url_expansion.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NFacts;

Y_UNIT_TEST_SUITE(UrlExpansion) {
    // FACTS-3458 - More tests covering Turbo URLs are available here: quality/functionality/turbo/urls_lib/cpp/tests/url_ut.cpp
    Y_UNIT_TEST(YandexTurbo) {
        const TString turboUrl1 = "https://yandex.ru/turbo?text=https%3A%2F%2Fru.wikipedia.org%2Fwiki%2F%25D0%25A0%25D0%25B8%25D0%25B3%25D0%25B8%25D0%25B4%25D0%25BD%25D0%25BE%25D1%2581%25D1%2582%25D1%258C";
        const TString expandedUrl1 = "https://ru.wikipedia.org/wiki/%D0%A0%D0%B8%D0%B3%D0%B8%D0%B4%D0%BD%D0%BE%D1%81%D1%82%D1%8C";
        UNIT_ASSERT_STRINGS_EQUAL(ExpandReferringUrl(turboUrl1), expandedUrl1);

        const TString turboUrl2 = "https://yandex.ru/turbo/s/gazeta.ru/tech/news/2020/01/13/n_13911284.shtml";
        const TString expandedUrl2 = "https://gazeta.ru/tech/news/2020/01/13/n_13911284.shtml";
        UNIT_ASSERT_STRINGS_EQUAL(ExpandReferringUrl(turboUrl2), expandedUrl2);
    }

    Y_UNIT_TEST(YandexTranslate) {
        const TString partiallyEncodedUrl = "https://translate.yandex.ru/translate?lang=uk-ru&view=c&url=https%3A//uk.m.wikipedia.org/wiki/%25D0%2594%25D0%25B5%25D1%2584%25D0%25BE%25D0%25BB%25D1%2582";
        const TString partiallyDecodedUrl = "https://uk.m.wikipedia.org/wiki/%D0%94%D0%B5%D1%84%D0%BE%D0%BB%D1%82";
        UNIT_ASSERT_STRINGS_EQUAL(ExpandReferringUrl(partiallyEncodedUrl), partiallyDecodedUrl);

        const TString fullyEncodedUrl = "https://translate.yandex.ru/translate?lang=en-ru&view=c&url=https%3A%2F%2Fen.wikipedia.org%2Fwiki%2FBERT_(language_model)";
        const TString fullyDecodedUrl = "https://en.wikipedia.org/wiki/BERT_(language_model)";
        UNIT_ASSERT_STRINGS_EQUAL(ExpandReferringUrl(fullyEncodedUrl), fullyDecodedUrl);
    }

    Y_UNIT_TEST(Unexpanding) {
        const TString unexpandingUrl = "https://kermlin.ru/puten/about";
        UNIT_ASSERT_STRINGS_EQUAL(ExpandReferringUrl(unexpandingUrl), unexpandingUrl);
    }

    Y_UNIT_TEST(Empty) {
        const TString emptyUrl = "";
        UNIT_ASSERT_STRINGS_EQUAL(ExpandReferringUrl(emptyUrl), emptyUrl);
    }

    Y_UNIT_TEST(Invalid) {
        const TString invalidUrl1 = "https://";
        UNIT_ASSERT_STRINGS_EQUAL(ExpandReferringUrl(invalidUrl1), invalidUrl1);

        const TString invalidUrl2 = "--__--";
        UNIT_ASSERT_STRINGS_EQUAL(ExpandReferringUrl(invalidUrl2), invalidUrl2);

        const TString invalidUrl3 = "223322223322";
        UNIT_ASSERT_STRINGS_EQUAL(ExpandReferringUrl(invalidUrl3), invalidUrl3);
    }
}
