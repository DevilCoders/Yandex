#include "search_crawlers.h"

#include <library/cpp/regex/regexp_classifier/regexp_classifier.h>

namespace NAntiRobot {

ECrawler UserAgentToCrawler(const TStringBuf& userAgent) {
    static const TRegexpClassifier<ECrawler> USER_AGENT_TO_CRAWLER(
        {
            {".*Google.*", ECrawler::GoogleBot},
            {".*bingbot.*", ECrawler::BingBot},
            {"msnbot.*", ECrawler::BingBot},
            {".*Mail\\.RU_Bot.*", ECrawler::MailRuBot},
            {R"(.*Pingdom\.com_bot_version_\d+\.\d+.*)", ECrawler::Pingdom},
            {R"(.*Applebot.*)", ECrawler::AppleBot},
            {R"(.*Yahoo! Slurp.*)", ECrawler::YahooBot},
            {R"(.*Sputnik.*Bot/\d+.*)", ECrawler::SputnikBot},
            {R"(.*SeznamBot/\d+.*)", ECrawler::SeznamBot},
            {R"(.*www\.pinterest\.com/bot\.html.*)", ECrawler::PinterestBot},
            {R"(.*OdklBot/1\.0.*)", ECrawler::OdnoklassnikiBot},
            {R"(.*Ya(Direct|ndex).*)", ECrawler::YandexBot},
            {R"(AASA-Bot/1\.0.*)", ECrawler::AasaBot},
        },
        ECrawler::None);

    return USER_AGENT_TO_CRAWLER[userAgent];
}

ECrawler HostToCrawler(const TStringBuf& host) {
    static const TRegexpClassifier<ECrawler> HOST_TO_CRAWLER(
        {
            {".+\\.googlebot\\.com", ECrawler::GoogleBot},
            {".+\\.google\\.com", ECrawler::GoogleBot},
            {".+\\.mail\\.ru", ECrawler::MailRuBot},
            {".+\\.search\\.msn\\.com", ECrawler::BingBot},
            {R"(.+\.pingdom\.com)", ECrawler::Pingdom},
            {R"(.+\.applebot\.apple\.com)", ECrawler::AppleBot},
            {R"(.+\.crawl\.yahoo\.net)", ECrawler::YahooBot},
            {R"(spider.+\.sputnik\.ru)", ECrawler::SputnikBot},
            {R"(fulltextrobot.+\.seznam\.cz)", ECrawler::SeznamBot},
            {R"(.+\.pinterest\.com)", ECrawler::PinterestBot},
            {R"(.+\.odnoklassniki\.ru)", ECrawler::OdnoklassnikiBot},
            {R"(.+\.yandex\.(ru|net|com))", ECrawler::YandexBot},
            {R"(.*\.aasa\.edge\.apple)", ECrawler::AasaBot},
        },
        ECrawler::None);

    return HOST_TO_CRAWLER[host];
}

}
