#include <library/cpp/testing/unittest/registar.h>

#include "host_ops.h"

namespace NAntiRobot {

Y_UNIT_TEST_SUITE(TestHostOps) {

Y_UNIT_TEST(TestGetYandexDomainFromHost) {
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("213.180.193.14"), "");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("yandex"), "");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("yandex.by.com.tr"), "");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("abracadabra"), "");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("abracadabra.yandex"), "");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("an.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("api-maps.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("appsearch.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("awaps.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("balancer-sas01.search.yandex.net"), "yandex.net");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("balancer-sas03.search.yandex.net"), "yandex.net");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("balancer-sas07.search.yandex.net"), "yandex.net");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("balancer-sas12.search.yandex.net"), "yandex.net");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("beta.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("blogs.yandex.by"), "yandex.by");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("buildfarm-d-web-pull-138.zelo.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("clck.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("copy.yandex.net"), "yandex.net");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("download.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("gorsel.yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("haber.yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("hamster.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("hghltd.yandex.net"), "yandex.net");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("images.yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("images.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("img.yandex.net"), "yandex.net");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("jgo.maps.yandex.net"), "yandex.net");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("legal.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("lostsoul-d-dev2.serp3.leon22.yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("lostsoul-d-dev2.serp3.leon22.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("m.blogs.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("m.blogs.yandex.ua"), "yandex.ua");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("m.gorsel.yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("m.haber.yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("m.images.yandex.com"), "yandex.com");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("m.news.yandex.ua"), "yandex.ua");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("m.video.yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("m.yandex.com"), "yandex.com");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("maps.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("maybee-d-islands1.serp3.leon02.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("mc.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("msk.yandex.com"), "yandex.com");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("news.yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("pass.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("pda.blogs.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("pda.haber.yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("pda.news.yandex.by"), "yandex.by");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("pda.news.yandex.kz"), "yandex.kz");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("pda.news.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("pda.news.yandex.ua"), "yandex.ua");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("people.yandex.by"), "yandex.by");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("people.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("people.yandex.ua"), "yandex.ua");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("phone-search.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("r.maps.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("rca.yandex.com"), "yandex.com");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("search.yaca.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("serp3.zelo.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("seznam.xmlsearch.yandex.com"), "yandex.com");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("soft.export.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("soft.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("suggest.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("vec04.maps.yandex.net"), "yandex.net");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("video-xmlsearch.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("video.yandex.com"), "yandex.com");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("video.yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("www.boswp.com"), "");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("www.images.yandex.com"), "yandex.com");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("www.yandex.com"), "yandex.com");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("www.yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("www.yandex.net"), "yandex.net");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("www.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("xmlsearch.yandex.com"), "yandex.com");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("xmlsearch.yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("xmlsearch.yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("yandex.by"), "yandex.by");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("yandex.com"), "yandex.com");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("yandex.kz"), "yandex.kz");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("yandex.net"), "yandex.net");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("yandex.ru"), "yandex.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("yandex.st"), "yandex.st");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("yandex.ua"), "yandex.ua");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("zzhanka-d-islands2.serp3.leon02.yandex.com.tr"), "yandex.com.tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("test.autoru.yandex.net"), "yandex.net");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("all7.test.autoru.yandex.net"), "yandex.net");
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost("www-desktop.test.autoru.yandex.net"), "yandex.net");
}

Y_UNIT_TEST(TestGetCookiesDomainFromHostEqualsGetYandexDomainFromHost) {
    const TStringBuf yandexSubDomains[] = {
        "www.yandex.net",
        "www.yandex.ru",
        "xmlsearch.yandex.com",
        "xmlsearch.yandex.com.tr",
        "yandex.com",
        "yandex.com.tr",
    };
    for (const TStringBuf& yandexSubDomain : yandexSubDomains) {
        UNIT_ASSERT_STRINGS_EQUAL(GetYandexDomainFromHost(yandexSubDomain), GetCookiesDomainFromHost(yandexSubDomain));
    }
}

Y_UNIT_TEST(TestGetDomainFromNonYandexHost) {
    UNIT_ASSERT_STRINGS_EQUAL(GetCookiesDomainFromHost("auto.ru"), "auto.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetCookiesDomainFromHost("www.auto.ru"), "auto.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetCookiesDomainFromHost("moscow.auto.ru"), "auto.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetCookiesDomainFromHost("kinopoisk.ru"), "kinopoisk.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetCookiesDomainFromHost("www.kinopoisk.ru"), "kinopoisk.ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetCookiesDomainFromHost("plus.kinopoisk.ru"), "kinopoisk.ru");
}

Y_UNIT_TEST(TestGetTldFromHost) {
    UNIT_ASSERT_STRINGS_EQUAL(GetTldFromHost("yandex.ru"), "ru");
    UNIT_ASSERT_STRINGS_EQUAL(GetTldFromHost("yandex.com.tr"), "tr");
    UNIT_ASSERT_STRINGS_EQUAL(GetTldFromHost("by"), "by");
    UNIT_ASSERT_STRINGS_EQUAL(GetTldFromHost(""), "");
}

}

}
