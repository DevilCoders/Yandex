#pragma once
#include <util/generic/strbuf.h>

namespace NAntiRobot {
// Pingdom is not a crawler but fits perfectly in our recognition system (https://st.yandex-team.ru/CAPTCHA-771)

enum class ECrawler {
    None      /* "None" */,
    GoogleBot /* "GoogleBot" */,
    BingBot   /* "BingBot" */,
    MailRuBot /* "MailRuBot" */,
    Pingdom   /* "Pingdom" */, 
    AppleBot  /* "AppleBot" */,
    YahooBot  /* "YahooBot" */,
    SputnikBot /* "SputnikBot" */,
    SeznamBot /* "SezamBot" */,
    PinterestBot /* "PinterestBot" */,
    OdnoklassnikiBot /* "OdnoklassnikiBot" */,
    YandexBot /* "YandexBot" */,
    AasaBot /* "AasaBot" */,
};

ECrawler UserAgentToCrawler(const TStringBuf& userAgent);
ECrawler HostToCrawler(const TStringBuf& host);

}
