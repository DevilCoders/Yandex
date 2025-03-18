#include <library/cpp/testing/unittest/registar.h>

#include "search_crawlers.h"

namespace NAntiRobot {

Y_UNIT_TEST_SUITE(SearchCrawlers) {

Y_UNIT_TEST(UserAgent) {
    const std::pair<ECrawler, TStringBuf> TEST_DATA[] = {
        {ECrawler::GoogleBot,     R"(Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)"},
        {ECrawler::GoogleBot,     R"(Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Safari/537.36 Google (+https://developers.google.com/+/web/snippet/))"},
        {ECrawler::GoogleBot,     R"(Googlebot/2.1 (+http://www.google.com/bot.html))"},
        {ECrawler::GoogleBot,     R"(Mozilla/5.0 (iPhone; CPU iPhone OS 6_0 like Mac OS X) AppleWebKit/536.26 (KHTML, like Gecko) Version/6.0 Mobile/10A5376e Safari/8536.25 (compatible; Googlebot/2.1; +http://www.google.com/bot.html))"},
        {ECrawler::GoogleBot,     R"(Googlebot-Image/1.0)"},
        {ECrawler::GoogleBot,     R"(Googlebot-Video/1.0)"},
        {ECrawler::GoogleBot,     R"(Android(compatible; Googlebot-Mobile/2.1; +http://www.google.com/bot.html))"},
        {ECrawler::BingBot,       R"(Mozilla/5.0 (compatible; bingbot/2.0; +http://www.bing.com/bingbot.htm))"},
        {ECrawler::BingBot,       R"(msnbot/2.0b (+http://search.msn.com/msnbot.htm))"},
        {ECrawler::MailRuBot,     R"(Mozilla/5.0 (compatible; Linux x86_64; Mail.RU_Bot/2.0; +//go.mail.ru/help/robots))"},
        {ECrawler::MailRuBot,     R"(Mozilla/5.0 (compatible; Linux x86_64; Mail.RU_Bot/Fast/2.0; +//go.mail.ru/help/robots))"},
        {ECrawler::None,          R"(Mozilla/5.0 (compatible; Linux x86_64))"},
        {ECrawler::None,          R"(wget)"},
        {ECrawler::Pingdom,       R"(Pingdom.com_bot_version_1.4_(http://www.pingdom.com/))"},
        {ECrawler::Pingdom,       R"(Pingdom.com_bot_version_2.9_(http://www.pingdom.com))"},
        {ECrawler::Pingdom,       R"(bla-bla-bla Pingdom.com_bot_version_2.9)"},
        {ECrawler::None,          R"(Pingdom.com_bot)"},
        {ECrawler::None,          R"(Pingdom)"},
        {ECrawler::AppleBot,      R"(Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_1) AppleWebKit/600.2.5 (KHTML, like Gecko) Version/8.0.2 Safari/600.2.5 (Applebot/0.1; +http://www.apple.com/go/applebot))"},
        {ECrawler::AppleBot,      R"(Applebot)"},
        {ECrawler::None,          R"(Aplebot)"},
        {ECrawler::None,          R"(AppleBot)"},
        {ECrawler::YahooBot,      R"(Mozilla/5.0 (compatible; Yahoo! Slurp; http://help.yahoo.com/help/us/ysearch/slurp))"},
        {ECrawler::YahooBot,      R"(compatible; Yahoo! Slurp)"},
        {ECrawler::None,          R"(Mozilla/5.0 (compatible; Yahoo!)"},
        {ECrawler::SputnikBot,    R"(Mozilla/5.0 (compatible; SputnikBot/2.3; +http://corp.sputnik.ru/webmaster))"},
        {ECrawler::None,          R"(Mozilla/5.0 (compatible; SputnikBot; +http://corp.sputnik.ru/webmaster))"},
        {ECrawler::SputnikBot,    R"(Mozilla/5.0 (compatible; SputnikImageBot/2.3; +http://corp.sputnik.ru/webmaster))"},
        {ECrawler::SeznamBot,     R"(Mozilla/5.0 (compatible; SeznamBot/3.2; +http://napoveda.seznam.cz/en/seznambot-intro/))"},
        {ECrawler::None,          R"(Mozilla/5.0 (compatible; SeznamBot; +http://napoveda.seznam.cz/en/seznambot-intro/))"},
        {ECrawler::None,          R"(Mozilla/5.0 (compatible; SeznomBot/3.2; +http://napoveda.seznam.cz/en/seznambot-intro/))"},
        {ECrawler::PinterestBot,  R"(Pinterest/0.2 (+https://www.pinterest.com/bot.html))"},
        {ECrawler::PinterestBot,  R"(Mozilla/5.0 (compatible; Pinterestbot/1.0; +https://www.pinterest.com/bot.html))"},
        {ECrawler::PinterestBot,  R"(Mozilla/5.0 (Linux; Android 6.0.1; Nexus 5X Build/MMB29P) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.96 Mobile Safari/537.36 (compatible; Pinterestbot/1.0; +https://www.pinterest.com/bot.html))"},
        {ECrawler::None,          R"(Mozilla/5.0 (Linux; Android 6.0.1; Nexus 5X Build/MMB29P) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.96 Mobile Safari/537.36 (compatible; Pinterestbot/1.0; +https://www.pint3rest.com/bot.html))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexAccessibilityBot/3.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexAdNet/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexBlogs/0.99; robot; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexBot/3.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexBot/3.0; MirrorDetector; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexCalendar/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexDirect/3.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexDirectDyn/1.0; +http://yandex.com/bots)"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexFavicons/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YaDirectFetcher/1.0; Dyatel; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexForDomain/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexImages/3.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexImageResizer/2.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (iPhone; CPU iPhone OS 8_1 like Mac OS X) AppleWebKit/600.1.4 (KHTML, like Gecko) Version/8.0 Mobile/12B411 Safari/600.1.4 (compatible; YandexBot/3.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (iPhone; CPU iPhone OS 8_1 like Mac OS X) AppleWebKit/600.1.4 (KHTML, like Gecko) Version/8.0 Mobile/12B411 Safari/600.1.4 (compatible; YandexMobileBot/3.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexMarket/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexMarket/2.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexMarket/3.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexMedia/3.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexMetrika/2.0; +http://yandex.com/bots yabs01))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexMetrika/2.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexMetrika/3.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexMetrika/4.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexMobileScreenShotBot/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexNews/4.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexOntoDB/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexOntoDBAPI/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexPagechecker/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexPartner/3.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexRCA/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexSearchShop/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexSitelinks; Dyatel; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexSpravBot/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexTracker/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexTurbo/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexVertis/3.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexVerticals/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexVideo/3.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexVideoParser/1.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (compatible; YandexWebmaster/2.0; +http://yandex.com/bots))"},
        {ECrawler::YandexBot,     R"(Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2228.0 Safari/537.36 (compatible; YandexScreenshotBot/3.0; +http://yandex.com/bots))"},
        {ECrawler::None,          R"(Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/87.0.4280.88 Safari/537.36 )"},
        {ECrawler::AasaBot,       R"(AASA-Bot/1.0.0)"},
    };

    for (const auto& testCase : TEST_DATA) {
        UNIT_ASSERT_VALUES_EQUAL_C(UserAgentToCrawler(testCase.second), testCase.first, testCase.second);
    }
}

Y_UNIT_TEST(Host) {
    const std::pair<ECrawler, TStringBuf> TEST_DATA[] = {
        {ECrawler::GoogleBot,           "crawl-66-249-64-1.googlebot.com"},
        {ECrawler::GoogleBot,           "google-proxy-66-102-9-139.google.com"},
        {ECrawler::MailRuBot,           "fetcher7-7.p.mail.ru"},
        {ECrawler::BingBot,             "msnbot-131-253-46-102.search.msn.com"},
        {ECrawler::None,                "somehost.com"},
        {ECrawler::Pingdom,             "s453.pingdom.com"},
        {ECrawler::Pingdom,             "000.pingdom.com"},
        {ECrawler::Pingdom,             "foo.bar.baz.pingdom.com"},
        {ECrawler::None,                "pingdom.com"},
        {ECrawler::AppleBot,            "17-142-159-138.applebot.apple.com"},
        {ECrawler::AppleBot,            "a.b.c.d.e.f.applebot.apple.com"},
        {ECrawler::None,                "apple.com"},
        {ECrawler::YahooBot,            "a.b.c.crawl.yahoo.net"},
        {ECrawler::None,                "yahoo.net"},
        {ECrawler::None,                "crawl.yahoo.net"},
        {ECrawler::SputnikBot,          "spider-5-143-231-45.sputnik.ru"},
        {ECrawler::None,                "spider.sputnik.ru"},
        {ECrawler::None,                "imagespider-9.sputnik.ru"},
        {ECrawler::SeznamBot,           "fulltextrobot-77-75-79-32.seznam.cz"},
        {ECrawler::None,                "fulltextrobot.seznam.cz"},
        {ECrawler::PinterestBot,        "crawl-54-236-1-11.pinterest.com"},
        {ECrawler::OdnoklassnikiBot,    "ip217.153.odnoklassniki.ru"},
        {ECrawler::None,                "crawl-54-236-1-11.pinterest.com.tr"},
        {ECrawler::YandexBot,           "crawl-54-236-1-11.yandex.ru"},
        {ECrawler::YandexBot,           "crawl-54-236-1-11.yandex.com"},
        {ECrawler::YandexBot,           "crawl-54-236-1-11.yandex.net"},
        {ECrawler::None,                "crawl-54-236-1-11.yandex.com.tr"},
        {ECrawler::AasaBot,             "ap-origin-euc-1-1.aasa.edge.apple"},
        {ECrawler::AasaBot,             "ap-origin-use-1-1.aasa.edge.apple"},
    };

    for (const auto& testCase : TEST_DATA) {
        UNIT_ASSERT_VALUES_EQUAL_C(HostToCrawler(testCase.second), testCase.first, testCase.second);
    }
}

}

}
