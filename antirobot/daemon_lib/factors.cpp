#include "factors.h"

#include "threat_level.h"

#include <antirobot/lib/addr.h>

#include <util/generic/singleton.h>
#include <util/generic/xrange.h>
#include <util/string/cast.h>
#include <util/string/join.h>

namespace {
    using namespace NAntiRobot;

    struct TWeight {
        inline TWeight(float s) noexcept
            : S(s)
            , W(CalcWeight(S))
        {
        }

        static float CalcWeight(float halfLifePeriod) noexcept {
            return 1.0f - pow(0.5f, 1.0f / halfLifePeriod);
        }

        float S;
        float W;
    };

    const TWeight AggregationWeights[TFactorsWithAggregation::NUM_AGGREGATIONS] = {5, 15, 50};

    const char* FACTOR_NAMES[] = {
        "suid_spravka",
        "dzen",
        "valid_spravka",
        "valid_fuid",
        "spravka_livetime",
        "xff",
        "nocookies",
        "noref",
        "ref_y",
        "badua", // bad user agent
        "baduan", // bad user agent new
        "ip_subnet_match",
        "tmsort",
        "num_docs",
        "numdoc",
        "bad_proto",
        "keepalive",
        "advanced_search",
        "lcookie",
        "pagenum",
        "karma",
        "bl",
        "syntax_error",
        "have_syntax",
        "misc_ops",
        "user_op",
        "has_necessity",
        "has_form_type",
        "quotes",
        "have_restr",
        "url_restr",
        "site_restr",
        "host_restr",
        "domain_restr",
        "in_url_restr",
        "other_restr",
        "all_restr",
        "comm",
        "nwords",
        "nchars",
        "isutf8",
        "popularity",
        "is_nav",
        "geo_city",
        "geo_locality",
        "is_geo",
        "porno_level",
        "wizard_cache_hit",
        "delta",
        "bad_deltas",
        "delta_exp",
        "have_unknown_headers",
        "headers_count",
        "gsk_score",
        "user_time",
        "mouse_move_entropy",
        "is_proxy",
        "is_tor",
        "is_vpn",
        "is_hosting",
        "requests_per_second",

        "Sec-Fetch-Dest",
        "Accept-Language",
        "Cookie",
        "Upgrade-Insecure-Requests",
        "Accept-Encoding",
        "DNT",
        "Origin",
        "User-Agent",
        "Host",
        "Referer",
        "Authority",
        "Cache-Control",
        "X-Forwarded-Proto",
        "Keep-Alive",
        "Pragma",
        "Proxy-Connection",
        "RTT",
        "ChromeHodor",

        "FraudJa3",
        "FraudSubnetNew",

        "UslugiGetWorkerPhoneHoneypots",
        "UslugiOtherRuchkasHoneypots",

        "KinopoiskFilmsHoneypots",
        "KinopoiskNamesHoneypots",
        "MarketHuman1Honeypots",
        "MarketHuman2Honeypots",
        "MarketHuman3Honeypots",

        "VerticalBlackSearch",
        "VerticalBlackOffer",
        "VerticalBlackPhone",
        "VerticalEventsLogs",

        "AutoruOfferHoneypot",

        "IsRobot",
        "IsMobile",
        "IsBrowser",
        "HistorySupport",
        "IsEmulator",
        "IsBrowserEngine",
        "IsBrowserEngineVersion",
        "IsBrowserVersion",
        "IsOSName",
        "IsOSVersion",
        "IsOSFamily",
        "IsOSFamilyAndroid",
        "IsOSFamilyWindows",
        "IsOSFamilyiOS",
        "IsOSFamilyMacOS",
        "IsOSFamilyLinux",
        "ITP",
        "ITPFakeCookie",
        "localStorageSupport",

        "captcha_input_delta",
        "captcha_incorrect_input_delta",

        "has_valid_market_jws",

        "P0fOlen",
        "P0fVersion",
        "P0fObservedTTL",

        "P0fEOL",
        "P0fITTLDistance",
        "P0fUnknownOptionID",

        "P0fMSS",
        "P0fWSize",
        "P0fScale",

        "P0fLayoutNOP",
        "P0fLayoutMSS",
        "P0fLayoutWS",
        "P0fLayoutSOK",
        "P0fLayoutSACK",
        "P0fLayoutTS",

        "P0fQuirksDF",
        "P0fQuirksIDp",
        "P0fQuirksIDn",
        "P0fQuirksECN",
        "P0fQuirks0p",
        "P0fQuirksFlow",
        "P0fQuirksSEQn",
        "P0fQuirksACKp",
        "P0fQuirksACKn",
        "P0fQuirksUPTRp",
        "P0fQuirksURGFp",
        "P0fQuirksPUSHFp",
        "P0fQuirksTS1n",
        "P0fQuirksTS2p",
        "P0fQuirksOPTp",
        "P0fQuirksEXWS",
        "P0fQuirksBad",

        "P0fPClass",

        "Ja3TlsVersion",
        "Ja3CiphersLen",
        "Ja3ExtensionsLen",

        "Ja3C159",
        "Ja3C57_61",
        "Ja3C53",
        "Ja3C60_49187",
        "Ja3C53_49187",
        "Ja3C52393_103",
        "Ja3C49162",
        "Ja3C50",
        "Ja3C51",
        "Ja3C255",
        "Ja3C52392",
        "Ja3C10",
        "Ja3C157_49200",
        "Ja3C49200",
        "Ja3C49171_103",
        "Ja3C49191_52394",
        "Ja3C49192_52394",
        "Ja3C65_52394",
        "Ja3C157",
        "Ja3C52393_49200",
        "Ja3C49159",
        "Ja3C4865",
        "Ja3C158_61",
        "Ja3C49196_47",
        "Ja3C103",
        "Ja3C103_49196",
        "Ja3C52393_49188",
        "Ja3C60_65",
        "Ja3C49195_69",
        "Ja3C154",
        "Ja3C49187_49188",
        "Ja3C49199_60",
        "Ja3C49195_51",
        "Ja3C49235",
        "Ja3C47",
        "Ja3C49169",
        "Ja3C49249",
        "Ja3C49171_60",
        "Ja3C49188_49196",
        "Ja3C61",
        "Ja3C156_255",
        "Ja3C47_57",
        "Ja3C186",
        "Ja3C49245",
        "Ja3C156_52394",
        "Ja3C20",
        "Ja3C49188_49195",
        "Ja3C52394_52392",
        "Ja3C53_49162",
        "Ja3C49191",
        "Ja3C49245_49249",
        "Ja3C49171",
        "Ja3C53_52393",
        "Ja3C51_49199",
        "Ja3C49234",
        "Ja3C49315",
        "Ja3C158",
        "Ja3C49187_49161",
        "Ja3C107",
        "Ja3C52394",
        "Ja3C49162_61",
        "Ja3C153",
        "Ja3C49170",
        "Ja3C156",
        "Ja3C52393_60",
        "Ja3C49195_49192",
        "Ja3C7",
        "Ja3C49187_103",
        "Ja3C61_49172",
        "Ja3C159_49188",
        "Ja3C49171_49187",
        "Ja3C49196_49188",
        "Ja3C158_49161",
        "Ja3C49193",
        "AcceptUniqueKeysNumber",
        "AcceptEncodingUniqueKeysNumber",
        "AcceptCharsetUniqueKeysNumber",
        "AcceptLanguageUniqueKeysNumber",
        "AcceptAnySpace",
        "AcceptEncodingAnySpace",
        "AcceptCharsetAnySpace",
        "AcceptLanguageAnySpace",
        "AcceptLanguageHasRussian",
        "HasValidYandexTrust",
        "HasValidAutoRuTamper",

        "AutoruJa3",
        "AutoruSubnet",
        "AutoruUA",

        "has_cookie_amcuid",
        "has_cookie_currentRegionId",
        "has_cookie_cycada",
        "has_cookie_local-offers-first",
        "has_cookie_lr",
        "has_cookie_market_ys",
        "has_cookie_onstock",
        "has_cookie_yandex_help",
        "has_cookie_cmp-merge",
        "has_cookie_computer",
        "has_cookie_head_baner",
        "has_cookie_utm_source",

        "cookie_younger_than_minute",
        "cookie_younger_than_hour",
        "cookie_younger_than_day",
        "cookie_younger_than_week",
        "cookie_younger_than_month",
        "cookie_younger_than_three_monthes",
        "cookie_older_than_month",
        "cookie_older_than_three_monthes",
    };

    static_assert(Y_ARRAY_SIZE(FACTOR_NAMES) == F_MARKET_JWS_STATE_FIRST - F_HAVE_TRUSTED_UID,
                  "expect Y_ARRAY_SIZE(FACTOR_NAMES) == F_MARKET_JWS_STATE_FIRST - F_HAVE_TRUSTED_UID");
    static_assert(Y_ARRAY_SIZE(FACTOR_NAMES) == TRawFactors::Count() - (REQ_NUMTYPES + HOST_NUMTYPES + REP_NUMTYPES +
                                                                        CgiParamName.size() + NUM_WIZARD_QUERY_CLASSES +
                                                                        NUM_QUERY_LANGS + NUM_PERSON_LANGS +
                                                                        CookieName.size() + HttpHeaderName.size() +
                                                                        static_cast<size_t>(EJwsStats::Count) + 3 * static_cast<size_t>(EMarketStats::Count)
                                                                        ));

    inline TString Aggr(const TString& s, size_t aggr) {
        if (aggr) {
            return Join('^', s, AggregationWeights[aggr - 1].S);
        }

        return s;
    }

    //TODO - SSE
    inline void Aggregate(const float* from, float* to, size_t len, float w) {
        for (size_t i = 0; i < len; ++i) {
            to[i] = (1.f - w) * to[i] + w * from[i];
        }
    }

    inline void Aggregate(const TRawFactors& f, TRawFactors& t, float w) {
        Aggregate(f.Factors, t.Factors, Y_ARRAY_SIZE(f.Factors), w);
    }

    inline TString IpAggr(const TString& s, size_t ipAggr) {
        static constexpr const char* aggr[TAllFactors::NUM_FACTOR_VALUES] = {
            "",
            "|ip",
            "|C",
            "|B",
            "|ja3",
        };

        if (ipAggr < Y_ARRAY_SIZE(aggr)) {
            return s + aggr[ipAggr];
        }

        ythrow yexception() << "incorrect index " << ipAggr;
    }
} // anonymous namespace

namespace NAntiRobot {
    TString TFactorsWithAggregation::GetFactorNameWithAggregationSuffix(size_t factorIndex) {
        if (factorIndex >= RawFactorsWithAggregationCount()) {
            ythrow yexception() << "incorrect index " << factorIndex;
        }

        const size_t index = factorIndex % TRawFactors::Count();
        const size_t aggr = factorIndex / TRawFactors::Count();

        if (index <= F_REQ_TYPE_LAST)
            return Aggr(TString("req_") + ToString(EReqType(index - F_REQ_TYPE_FIRST)), aggr);

        if (index <= F_HOST_TYPE_LAST)
            return Aggr(TString("host_") + ToString(EHostType(index - F_HOST_TYPE_FIRST)), aggr);

        if (index <= F_REPORT_TYPE_LAST)
            return Aggr(TString("rep_") + ToString(EReportType(index - F_REPORT_TYPE_FIRST)), aggr);

        if (index >= F_HTTP_HEADER_PRESENCE_FIRST)
            return Aggr(TString("has_http_header_") + ToString(HttpHeaderName[index - F_HTTP_HEADER_PRESENCE_FIRST]), aggr);

        if (index >= F_COOKIE_PRESENCE_FIRST)
            return Aggr(TString("has_cookie_") + ToString(CookieName[index - F_COOKIE_PRESENCE_FIRST]), aggr);

        if (index >= F_CGI_PARAM_FIRST)
            return Aggr(TString("cgi_") + CgiParamName[index - F_CGI_PARAM_FIRST], aggr);

        if (index >= F_PERSON_LANG_FIRST)
            return Aggr(TString("plang_") + NameByLanguage(PERSON_LANGS[index - F_PERSON_LANG_FIRST]), aggr);

        if (index >= F_QUERY_LANG_FIRST)
            return Aggr(TString("qlang_") + QUERY_LANGS[index - F_QUERY_LANG_FIRST], aggr);

        if (index >= F_QUERY_CLASS_FIRST)
            return Aggr(TString("class_") + WIZARD_QUERY_CLASSES[index - F_QUERY_CLASS_FIRST], aggr);

        if (index >= F_MARKET_UA_STATS_FIRST)
            return Aggr(TString("MarketUA") + ToString(static_cast<EMarketStats>(index - static_cast<size_t>(F_MARKET_UA_STATS_FIRST))), aggr);

        if (index >= F_MARKET_SUBNET_STATS_FIRST)
            return Aggr(TString("MarketSubnet") + ToString(static_cast<EMarketStats>(index - static_cast<size_t>(F_MARKET_SUBNET_STATS_FIRST))), aggr);

        if (index >= F_MARKET_JA3_STATS_FIRST)
            return Aggr(TString("MarketJa3") + ToString(static_cast<EMarketStats>(index - static_cast<size_t>(F_MARKET_JA3_STATS_FIRST))), aggr);

        if (index >= F_MARKET_JWS_STATE_FIRST)
            return Aggr(TString("MarketJwsStateIs") + ToString(static_cast<EJwsStats>(index - static_cast<size_t>(F_MARKET_JWS_STATE_FIRST))), aggr);

        return Aggr(FACTOR_NAMES[index - F_HAVE_TRUSTED_UID], aggr);
    }

    void TRawFactors::FillFromFeatures(const TRequestFeatures& rf) {
        SetBool(F_REQ_TYPE_FIRST + rf.ReqType, true);
        SetBool(F_HOST_TYPE_FIRST + rf.HostType, true);
        SetBool(F_REPORT_TYPE_FIRST + rf.ReportType, true);

        SetBool(F_HAVE_TRUSTED_UID, rf.Request->Uid.Trusted());
        SetBool(F_YANDSEARCH_DZEN, rf.IsYandsearchDzen());
        SetBool(F_XFF_EXISTS, rf.XffExists);
        SetBool(F_NO_COOKIES, rf.NoCookies);
        SetBool(F_NO_REFERER, rf.NoReferer);
        SetBool(F_REFERER_FROM_YANDEX, rf.RefererFromYandex);
        SetBool(F_BAD_USER_AGENT, rf.IsBadUserAgent);
        SetBool(F_BAD_USER_AGENT_NEW, rf.IsBadUserAgentNew);
        Factors[F_IP_SUBNET_MATCH] = CalcIpDistance(rf.Request->UserAddr, rf.Request->SpravkaAddr);
        SetBool(F_TIME_SORT, rf.IsTimeSort);
        Factors[F_NUM_DOCS] = rf.NumDocs;
        SetBool(F_BAD_NUMDOC, (rf.NumDocs != 10));
        SetBool(F_BAD_PROTOCOL, rf.IsBadProtocol);
        SetBool(F_CONNECTION_KEEPALIVE, rf.IsConnectionKeepAlive);
        Factors[F_ADVANCED_SEARCH] = rf.AdvancedSearch;
        SetBool(F_LCOOKIE_EXISTS, rf.LCookieExists);
        Factors[F_PAGE_NUM] = (float)rf.PageNum;
        SetBool(F_KARMA_GOOD, false);   //TODO: remove the factor
        SetBool(F_IN_BLACKLIST, false); //TODO: remove the factor
        SetBool(F_HAVE_VALID_SPRAVKA, rf.Request->HasValidSpravka);
        SetBool(F_HAVE_VALID_FUID, rf.Request->HasValidFuid);
        Factors[F_SPRAVKA_LIVETIME] = SpravkaAge(rf.Request->ArrivalTime, rf.Request->SpravkaTime).SecondsFloat();
        SetBool(F_HAVE_UNKNOWN_HEADERS, rf.HaveUnknownHeaders);
        Factors[F_HEADERS_COUNT] = rf.HeadersCount;
        Factors[F_GSK_SCORE] = rf.GskScore;
        Factors[F_USERTIME] = rf.UserTime;
        Factors[F_MOUSE_MOVE_ENTROPY] = rf.MouseMoveEntropy;

        SetBool(F_IS_PROXY, rf.IsProxy);
        SetBool(F_IS_TOR, rf.IsTor);
        SetBool(F_IS_VPN, rf.IsVpn);
        SetBool(F_IS_HOSTING, rf.IsHosting);

        Factors[F_REQUESTS_PER_SECOND] = rf.RequestsPerSecond;

        Factors[F_SEC_FETCH_DEST] = rf.SecFetchDest;
        Factors[F_ACCEPT_LANGUAGE] = rf.AcceptLanguage;
        Factors[F_COOKIE] = rf.Cookie;
        Factors[F_UPGRADE_INSECURE_REQUESTS] = rf.UpgradeInsecureRequests;
        Factors[F_ACCEPT_ENCODING] = rf.AcceptEncoding;
        Factors[F_DNT] = rf.Dnt;
        Factors[F_ORIGIN] = rf.Origin;
        Factors[F_USER_AGENT] = rf.UserAgent;
        Factors[F_HOST] = rf.Host;
        Factors[F_REFERER] = rf.Referer;
        Factors[F_AUTHORITY] = rf.Authority;
        Factors[F_CACHE_CONTROL] = rf.CacheControl;
        Factors[F_X_FORWARDED_PROTO] = rf.XForwardedProto;
        Factors[F_KEEP_ALIVE] = rf.KeepAlive;
        Factors[F_PRAGMA] = rf.Pragma;
        Factors[F_PROXY_CONNECTION] = rf.ProxyConnection;
        Factors[F_RTT] = rf.Rtt;
        SetBool(F_CHROME_HODOR, rf.ChromeHodor);

        Factors[F_FRAUD_JA3] = rf.FraudJa3;
        Factors[F_FRAUD_SUBNET_NEW] = rf.FraudSubnetNew;

        Factors[F_AUTORU_JA3] = rf.AutoruJa3;
        Factors[F_AUTORU_SUBNET] = rf.AutoruSubnet;
        Factors[F_AUTORU_UA] = rf.AutoruUA;

        SetBool(F_USLUGI_GET_WORKER_PHONE_HONEYPOTS, rf.UslugiGetWorkerPhoneHoneypots);
        SetBool(F_USLUGI_OTHER_API_HONEYPOTS, rf.UslugiOtherApiHoneypots);

        Factors[F_KINOPOISK_FILMS_HONEYPOTS] = rf.KinopoiskFilmsHoneypots;
        Factors[F_KINOPOISK_NAMES_HONEYPOTS] = rf.KinopoiskNamesHoneypots;

        Factors[F_MARKET_HUMAN_1_HONEYPOTS] = rf.MarketHuman1Honeypots;
        Factors[F_MARKET_HUMAN_2_HONEYPOTS] = rf.MarketHuman2Honeypots;
        Factors[F_MARKET_HUMAN_3_HONEYPOTS] = rf.MarketHuman3Honeypots;

        SetBool(F_VERTICAL_BLACK_SEARCH, rf.Request->VerticalReqGroup == EVerticalReqGroup::BlackSearch);
        SetBool(F_VERTICAL_BLACK_OFFER,  rf.Request->VerticalReqGroup == EVerticalReqGroup::BlackOffer);
        SetBool(F_VERTICAL_BLACK_PHONE,  rf.Request->VerticalReqGroup == EVerticalReqGroup::BlackPhone);
        SetBool(F_VERTICAL_EVENTS_LOGS,  rf.Request->VerticalReqGroup == EVerticalReqGroup::EventsLog);

        SetBool(F_HAS_COOKIE_AMCUID, rf.HasCookieAmcuid);
        SetBool(F_HAS_COOKIE_CURRENT_REGION_ID, rf.HasCookieCurrentRegionId);
        SetBool(F_HAS_COOKIE_CYCADA, rf.HasCookieCycada);
        SetBool(F_HAS_COOKIE_LOCAL_OFFERS_FIRST, rf.HasCookieLocalOffersFirst);
        SetBool(F_HAS_COOKIE_LR, rf.HasCookieLr);
        SetBool(F_HAS_COOKIE_MARKET_YS, rf.HasCookieMarketYs);
        SetBool(F_HAS_COOKIE_ONSTOCK, rf.HasCookieOnstock);
        SetBool(F_HAS_COOKIE_YANDEX_HELP, rf.HasCookieYandexHelp);
        SetBool(F_HAS_COOKIE_CMP_MERGE, rf.HasCookieCmpMerge);
        SetBool(F_HAS_COOKIE_COMPUTER, rf.HasCookieComputer);
        SetBool(F_HAS_COOKIE_HEAD_BANER, rf.HasCookieHeadBaner);
        SetBool(F_HAS_COOKIE_UTM_SOURCE, rf.HasCookieUtmSource);

        Factors[F_COOKIE_YOUNGER_THAN_MINUTE] = rf.CookieYoungerMinute;
        Factors[F_COOKIE_YOUNGER_THAN_HOUR] = rf.CookieYoungerHour;
        Factors[F_COOKIE_YOUNGER_THAN_DAY] = rf.CookieYoungerDay;
        Factors[F_COOKIE_YOUNGER_THAN_WEEK] = rf.CookieYoungerWeek;
        Factors[F_COOKIE_YOUNGER_THAN_MONTH] = rf.CookieYoungerMonth;
        Factors[F_COOKIE_YOUNGER_THAN_THREE_MONTHES] = rf.CookieYoungerThreeMonthes;
        Factors[F_COOKIE_OLDER_THAN_MONTH] = rf.CookieOlderMonth;
        Factors[F_COOKIE_OLDER_THAN_THREE_MONTHES] = rf.CookieOlderThreeMonthes;

        SetBool(F_IS_ROBOT, rf.IsRobot);
        SetBool(F_IS_MOBILE, rf.IsMobile);
        SetBool(F_IS_BROWSER, rf.IsBrowser);
        SetBool(F_HISTORY_SUPPORT, rf.HistorySupport);
        SetBool(F_IS_EMULATOR, rf.IsEmulator);
        SetBool(F_IS_BROWSER_ENGINE, rf.IsBrowserEngine);
        SetBool(F_IS_BROWSER_ENGINE_VERSION, rf.IsBrowserEngineVersion);
        SetBool(F_IS_BROWSER_VERSION, rf.IsBrowserVersion);
        SetBool(F_IS_OS_NAME, rf.IsOSName);
        SetBool(F_IS_OS_VERSION, rf.IsOSVersion);
        SetBool(F_IS_OS_FAMILY, rf.IsOSFamily);
        SetBool(F_IS_OS_FAMILY_ANDROID, rf.IsOSFamilyAndroid);
        SetBool(F_IS_OS_FAMILY_WINDOWS, rf.IsOSFamilyWindows);
        SetBool(F_IS_OS_FAMILYI_OS, rf.IsOSFamilyiOS);
        SetBool(F_IS_OS_FAMILY_MAC_OS, rf.IsOSFamilyMacOS);
        SetBool(F_IS_OS_FAMILY_LINUX, rf.IsOSFamilyLinux);
        SetBool(F_ITP, rf.ITP);
        SetBool(F_ITPFAKE_COOKIE, rf.ITPFakeCookie);
        SetBool(F_LOCAL_STORAGE_SUPPORT, rf.localStorageSupport);

        SetBool(F_CAPTCHA_HAS_VALID_MARKET_JWS, rf.Request->JwsState == EJwsState::Valid);
        SetBool(F_HAS_VALID_YANDEX_TRUST, rf.Request->YandexTrustState == EYandexTrustState::Valid);
        SetBool(F_HAS_VALID_AUTORU_TAMPER, rf.Request->HasValidAutoRuTamper);

        Factors[F_P0F_OLEN] = rf.P0fOlen;
        Factors[F_P0F_VERSION] = rf.P0fVersion;
        Factors[F_P0F_OBSERVED_TTL] = rf.P0fObservedTTL;

        Factors[F_P0F_EOL] = rf.P0fEOL;
        Factors[F_P0F_ITTL_DISTANCE] = rf.P0fITTLDistance;
        Factors[F_P0F_UNKNOWN_OPTION_ID] = rf.P0fUnknownOptionID;

        SetBool(F_P0F_MSS, rf.P0fMSS);
        SetBool(F_P0F_WSIZE, rf.P0fWSize);
        SetBool(F_P0F_SCALE, rf.P0fScale);

        SetBool(F_P0F_LAYOUT_NOP, rf.P0fLayoutNOP);
        SetBool(F_P0F_LAYOUT_MSS, rf.P0fLayoutMSS);
        SetBool(F_P0F_LAYOUT_WS, rf.P0fLayoutWS);
        SetBool(F_P0F_LAYOUT_SOK, rf.P0fLayoutSOK);
        SetBool(F_P0F_LAYOUT_SACK, rf.P0fLayoutSACK);
        SetBool(F_P0F_LAYOUT_TS, rf.P0fLayoutTS);

        SetBool(F_P0F_QUIRKS_DF, rf.P0fQuirksDF);
        SetBool(F_P0F_QUIRKS_ID_P, rf.P0fQuirksIDp);
        SetBool(F_P0F_QUIRKS_ID_N, rf.P0fQuirksIDn);
        SetBool(F_P0F_QUIRKS_ECN, rf.P0fQuirksECN);
        SetBool(F_P0F_QUIRKS_0_P, rf.P0fQuirks0p);
        SetBool(F_P0F_QUIRKS_FLOW, rf.P0fQuirksFlow);
        SetBool(F_P0F_QUIRKS_SEQ_N, rf.P0fQuirksSEQn);
        SetBool(F_P0F_QUIRKS_ACK_P, rf.P0fQuirksACKp);
        SetBool(F_P0F_QUIRKS_ACK_N, rf.P0fQuirksACKn);
        SetBool(F_P0F_QUIRKS_UPTR_P, rf.P0fQuirksUPTRp);
        SetBool(F_P0F_QUIRKS_URGF_P, rf.P0fQuirksURGFp);
        SetBool(F_P0F_QUIRKS_PUSHF_P, rf.P0fQuirksPUSHFp);
        SetBool(F_P0F_QUIRKS_TS1_N, rf.P0fQuirksTS1n);
        SetBool(F_P0F_QUIRKS_TS2_P, rf.P0fQuirksTS2p);
        SetBool(F_P0F_QUIRKS_OPT_P, rf.P0fQuirksOPTp);
        SetBool(F_P0F_QUIRKS_EXWS, rf.P0fQuirksEXWS);
        SetBool(F_P0F_QUIRKS_BAD, rf.P0fQuirksBad);

        SetBool(F_P0F_P_CLASS, rf.P0fPClass);

        SetJwsStat(rf.MarketJwsStatesStats);
        SetMarketStats(F_MARKET_JA3_STATS_FIRST, rf.MarketJa3Stats);
        SetMarketStats(F_MARKET_SUBNET_STATS_FIRST, rf.MarketSubnetStats);
        SetMarketStats(F_MARKET_UA_STATS_FIRST, rf.MarketUAStats);

        SetSyntaxFactors(rf.WizardFeatures);
        SetWizardFactors(rf.WizardFeatures);

        for (auto i : xrange(CgiParamName.size())) {
            SetBool(F_CGI_PARAM_FIRST + i, rf.CgiParamPresent[i]);
        }

        for (auto i : xrange(CookieName.size())) {
            SetBool(F_COOKIE_PRESENCE_FIRST + i, rf.CookiePresent[i]);
        }

        for (auto i : xrange(HttpHeaderName.size())) {
            SetBool(F_HTTP_HEADER_PRESENCE_FIRST + i, rf.HttpHeaderPresent[i]);
        }
    }

    void TRawFactors::SetJwsStat(const TMarketJwsStatesStats& marketJwsStatesStats) {
        static_assert(sizeof(TMarketJwsStatesStats) == (F_MARKET_JWS_STATE_LAST - F_MARKET_JWS_STATE_FIRST + 1) * sizeof(float), "TMarketJwsStatesStats not equal with factors");
        memcpy(&Factors[F_MARKET_JWS_STATE_FIRST], &marketJwsStatesStats, sizeof(TMarketJwsStatesStats));
    }

    void TRawFactors::SetMarketStats(size_t offset, const TMarketStats& marketStats) {
        static_assert(sizeof(TMarketStats) == (F_MARKET_JA3_STATS_LAST - F_MARKET_JA3_STATS_FIRST + 1) * sizeof(float), "TMarketStats not equal with factors");
        memcpy(&Factors[offset], &marketStats, sizeof(TMarketStats));
    }

    void TRawFactors::SetWizardFactors(const TWizardFactorsCalculator::TValues& wizardFeatures) {
        Factors[F_COMMERCIAL] = wizardFeatures.Commercial;
        Factors[F_NUM_WORDS] = (float)wizardFeatures.NumWords;
        Factors[F_REQUEST_LENGTH] = (float)wizardFeatures.RequestLength;
        SetBool(F_REQUEST_IS_UTF8, wizardFeatures.IsUtf8);
        Factors[F_POPULARITY] = (float)wizardFeatures.PopularityLevel;
        SetBool(F_IS_NAV, wizardFeatures.IsNav);
        Factors[F_GEO_CITY] = (float)wizardFeatures.GeoCity;
        Factors[F_GEO_LOCALITY] = wizardFeatures.GeoLocality;
        SetBool(F_IS_GEO, wizardFeatures.GeoCity != 0);
        Factors[F_PORNO_LEVEL] = wizardFeatures.PornoLevel;
        SetBool(F_WIZARD_CACHE_HIT, wizardFeatures.WizardCacheHit);

        for (size_t i = 0; i < NUM_WIZARD_QUERY_CLASSES; ++i) {
            SetBool(F_QUERY_CLASS_FIRST + i, wizardFeatures.QueryClass[i]);
        }

        for (size_t i = 0; i < NUM_QUERY_LANGS; ++i) {
            SetBool(F_QUERY_LANG_FIRST + i, wizardFeatures.QueryLang[i]);
        }

        for (size_t i = 0; i < NUM_PERSON_LANGS; ++i) {
            SetBool(F_PERSON_LANG_FIRST + i, wizardFeatures.PersonLang[i]);
        }
    }

    void TRawFactors::SetSyntaxFactors(const TWizardFactorsCalculator::TValues& wizardFeatures) {
        SetBool(F_SYNTAX_ERROR, wizardFeatures.SyntaxError);
        SetBool(F_HAVE_SYNTAX, wizardFeatures.HaveSyntax());
        SetBool(F_MISC_OPS, wizardFeatures.HasMiscOps);
        SetBool(F_USER_OP, wizardFeatures.HasUserOp);
        SetBool(F_HAS_NECESSITY, wizardFeatures.HasNecessity);
        SetBool(F_HAS_FORM_TYPE, wizardFeatures.HasFormType);
        SetBool(F_HAS_QUOTES, wizardFeatures.HasQuotes);
        SetBool(F_HAVE_RESTR, wizardFeatures.HaveRestr());
        Factors[F_URL_RESTR] = wizardFeatures.UrlRestr;
        Factors[F_SITE_RESTR] = wizardFeatures.SiteRestr;
        Factors[F_HOST_RESTR] = wizardFeatures.HostRestr;
        Factors[F_DOMAIN_RESTR] = wizardFeatures.DomainRestr;
        Factors[F_IN_URL_RESTR] = wizardFeatures.InUrlRestr;
        Factors[F_OTHER_RESTR] = wizardFeatures.OtherRestr;
        Factors[F_ALL_RESTR] = wizardFeatures.Restrictions();
    }

    void TRawFactors::Save(IOutputStream* out) const {
        out->Write(Factors, sizeof(Factors));
    }

    void TRawFactors::Load(IInputStream* in) {
        in->Read(Factors, sizeof(Factors));
    }

    void TFactorsWithAggregation::Aggregate() {
        for (size_t i = 0; i < NUM_AGGREGATIONS; ++i) {
            ::Aggregate(NonAggregatedFactors, AggregatedFactors[i], AggregationWeights[i].W);
        }
    }

    void TFactorsWithAggregation::Save(IOutputStream* out) const {
        NonAggregatedFactors.Save(out);
        for (const auto& factor : AggregatedFactors) {
            factor.Save(out);
        }
    }

    void TFactorsWithAggregation::Load(IInputStream* in, size_t count) {
        if (count != RawFactorsWithAggregationCount())
            ythrow yexception() << "Bad factors count";

        NonAggregatedFactors.Load(in);

        for (auto& factor : AggregatedFactors) {
            factor.Load(in);
        }
    }

    void TFactorsWithAggregation::FillNaN() {
        FillN(NonAggregatedFactors.Factors, NonAggregatedFactors.Count(), std::numeric_limits<float>::quiet_NaN());
        for (size_t i = 0; i < NUM_AGGREGATIONS; i++) {
            FillN(AggregatedFactors[i].Factors, AggregatedFactors[i].Count(), std::numeric_limits<float>::quiet_NaN());
        }
    }

    float& TFactorsWithAggregation::GetFactor(size_t index) {
        constexpr size_t factorsCount = TRawFactors::Count();
        const size_t aggrIndex = index / factorsCount;
        const size_t factorIndex = index % factorsCount;
        if (aggrIndex == 0) {
            return NonAggregatedFactors.Factors[factorIndex];
        }
        return AggregatedFactors[aggrIndex - 1].Factors[factorIndex];
    }

    TProcessorLinearizedFactors TAllFactors::GetLinearizedFactors(
        const TAntirobotCookie* arCookie
    ) const {
        TProcessorLinearizedFactors linearizedFactors;
        linearizedFactors.reserve(AllFactorsCount());

        for (const auto& factorsWithAggregation : FactorsWithAggregation) {
            const auto& nonAggregatedFactors = factorsWithAggregation.NonAggregatedFactors;
            linearizedFactors.insert(
                linearizedFactors.end(),
                nonAggregatedFactors.Factors,
                nonAggregatedFactors.Factors + Y_ARRAY_SIZE(nonAggregatedFactors.Factors)
            );

            for (const auto& aggregatedFactors : factorsWithAggregation.AggregatedFactors) {
                linearizedFactors.insert(
                    linearizedFactors.end(),
                    aggregatedFactors.Factors,
                    aggregatedFactors.Factors + Y_ARRAY_SIZE(aggregatedFactors.Factors)
                );
            }
        }

        if (arCookie) {
            for (const auto& rule : ANTIROBOT_DAEMON_CONFIG.GlobalJsonConfig.LastVisitsRules) {
                linearizedFactors.push_back(arCookie->LastVisitsCookie.Get(rule.Id));
            }
        } else {
            linearizedFactors.resize(AllFactorsCount());
        }

        return linearizedFactors;
    }

    TString TAllFactors::GetFactorNameByIndex(size_t index) {
        if (index < NumLocalFactors) {
            constexpr size_t cnt = TFactorsWithAggregation::RawFactorsWithAggregationCount();
            return IpAggr(TFactorsWithAggregation::GetFactorNameWithAggregationSuffix(index % cnt), index / cnt);
        }

        if (index < AllFactorsCount()) {
            const size_t ruleIndex = index - NumLocalFactors;
            return ANTIROBOT_DAEMON_CONFIG.GlobalJsonConfig.LastVisitsRules[ruleIndex].Name;
        }

        ythrow yexception() << "incorrect index " << index;
    }
} // namespace NAntiRobot
