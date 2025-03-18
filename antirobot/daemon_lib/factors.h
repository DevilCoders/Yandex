#pragma once

#include "cgi_params.h"
#include "classificator.h"
#include "cookie_names.h"
#include "market_stats.h"
#include "request_features.h"

#include <util/ysaveload.h>
#include <util/generic/singleton.h>
#include <util/string/cast.h>

namespace NAntiRobot {

    const ui32 FACTORS_VERSION = 117;

    enum EAntirobotFactors {
        F_REQ_TYPE_FIRST = 0,
        F_REQ_TYPE_LAST = F_REQ_TYPE_FIRST + REQ_NUMTYPES - 1,
        F_HOST_TYPE_FIRST,
        F_HOST_TYPE_LAST = F_HOST_TYPE_FIRST + HOST_NUMTYPES - 1,
        F_REPORT_TYPE_FIRST,
        F_REPORT_TYPE_LAST = F_REPORT_TYPE_FIRST + REP_NUMTYPES - 1,
        F_HAVE_TRUSTED_UID,
        F_YANDSEARCH_DZEN,
        F_HAVE_VALID_SPRAVKA,
        F_HAVE_VALID_FUID,
        F_SPRAVKA_LIVETIME,
        F_XFF_EXISTS,
        F_NO_COOKIES,
        F_NO_REFERER,
        F_REFERER_FROM_YANDEX,
        F_BAD_USER_AGENT,
        F_BAD_USER_AGENT_NEW,
        F_IP_SUBNET_MATCH,
        F_TIME_SORT,
        F_NUM_DOCS,
        F_BAD_NUMDOC,
        F_BAD_PROTOCOL,
        F_CONNECTION_KEEPALIVE,
        F_ADVANCED_SEARCH,
        F_LCOOKIE_EXISTS,
        F_PAGE_NUM,
        F_KARMA_GOOD,
        F_IN_BLACKLIST,
        F_SYNTAX_ERROR,
        F_HAVE_SYNTAX,
        F_MISC_OPS,
        F_USER_OP,
        F_HAS_NECESSITY,
        F_HAS_FORM_TYPE,
        F_HAS_QUOTES,
        F_HAVE_RESTR,
        F_URL_RESTR,
        F_SITE_RESTR,
        F_HOST_RESTR,
        F_DOMAIN_RESTR,
        F_IN_URL_RESTR,
        F_OTHER_RESTR,
        F_ALL_RESTR,
        F_COMMERCIAL,
        F_NUM_WORDS,
        F_REQUEST_LENGTH,
        F_REQUEST_IS_UTF8,
        F_POPULARITY,
        F_IS_NAV,
        F_GEO_CITY, // TODO remove
        F_GEO_LOCALITY,
        F_IS_GEO,
        F_PORNO_LEVEL,
        F_WIZARD_CACHE_HIT,
        F_TIME_DELTA,
        F_BAD_TIME_DELTAS,
        F_DELTA_DEVIATION_FROM_EXP_DISTRIB,
        F_HAVE_UNKNOWN_HEADERS,
        F_HEADERS_COUNT,
        F_GSK_SCORE,
        F_USERTIME,
        F_MOUSE_MOVE_ENTROPY,
        F_IS_PROXY,
        F_IS_TOR,
        F_IS_VPN,
        F_IS_HOSTING,

        F_REQUESTS_PER_SECOND,

        F_SEC_FETCH_DEST,
        F_ACCEPT_LANGUAGE,
        F_COOKIE,
        F_UPGRADE_INSECURE_REQUESTS,
        F_ACCEPT_ENCODING,
        F_DNT,
        F_ORIGIN,
        F_USER_AGENT,
        F_HOST,
        F_REFERER,
        F_AUTHORITY,
        F_CACHE_CONTROL,
        F_X_FORWARDED_PROTO,
        F_KEEP_ALIVE,
        F_PRAGMA,
        F_PROXY_CONNECTION,
        F_RTT,
        F_CHROME_HODOR,

        F_FRAUD_JA3,
        F_FRAUD_SUBNET_NEW,

        F_USLUGI_GET_WORKER_PHONE_HONEYPOTS,
        F_USLUGI_OTHER_API_HONEYPOTS,

        F_KINOPOISK_FILMS_HONEYPOTS,
        F_KINOPOISK_NAMES_HONEYPOTS,
        F_MARKET_HUMAN_1_HONEYPOTS,
        F_MARKET_HUMAN_2_HONEYPOTS,
        F_MARKET_HUMAN_3_HONEYPOTS,
        F_VERTICAL_BLACK_SEARCH,
        F_VERTICAL_BLACK_OFFER,
        F_VERTICAL_BLACK_PHONE,
        F_VERTICAL_EVENTS_LOGS,
        F_AUTORU_OFFER_HONEYPOT,
        F_IS_ROBOT,
        F_IS_MOBILE,
        F_IS_BROWSER,
        F_HISTORY_SUPPORT,
        F_IS_EMULATOR,
        F_IS_BROWSER_ENGINE,
        F_IS_BROWSER_ENGINE_VERSION,
        F_IS_BROWSER_VERSION,
        F_IS_OS_NAME,
        F_IS_OS_VERSION,
        F_IS_OS_FAMILY,
        F_IS_OS_FAMILY_ANDROID,
        F_IS_OS_FAMILY_WINDOWS,
        F_IS_OS_FAMILYI_OS,
        F_IS_OS_FAMILY_MAC_OS,
        F_IS_OS_FAMILY_LINUX,
        F_ITP,
        F_ITPFAKE_COOKIE,
        F_LOCAL_STORAGE_SUPPORT,

        F_CAPTCHA_INPUT_DELTA,
        F_CAPTCHA_INCORRECT_INPUT_DELTA,

        F_CAPTCHA_HAS_VALID_MARKET_JWS,

        F_P0F_OLEN,
        F_P0F_VERSION,
        F_P0F_OBSERVED_TTL,

        F_P0F_EOL,
        F_P0F_ITTL_DISTANCE,
        F_P0F_UNKNOWN_OPTION_ID,

        F_P0F_MSS,
        F_P0F_WSIZE,
        F_P0F_SCALE,

        F_P0F_LAYOUT_NOP,
        F_P0F_LAYOUT_MSS,
        F_P0F_LAYOUT_WS,
        F_P0F_LAYOUT_SOK,
        F_P0F_LAYOUT_SACK,
        F_P0F_LAYOUT_TS,

        F_P0F_QUIRKS_DF,
        F_P0F_QUIRKS_ID_P,
        F_P0F_QUIRKS_ID_N,
        F_P0F_QUIRKS_ECN,
        F_P0F_QUIRKS_0_P,
        F_P0F_QUIRKS_FLOW,
        F_P0F_QUIRKS_SEQ_N,
        F_P0F_QUIRKS_ACK_P,
        F_P0F_QUIRKS_ACK_N,
        F_P0F_QUIRKS_UPTR_P,
        F_P0F_QUIRKS_URGF_P,
        F_P0F_QUIRKS_PUSHF_P,
        F_P0F_QUIRKS_TS1_N,
        F_P0F_QUIRKS_TS2_P,
        F_P0F_QUIRKS_OPT_P,
        F_P0F_QUIRKS_EXWS,
        F_P0F_QUIRKS_BAD,

        F_P0F_P_CLASS,

        F_JA3_TLS_VERSION,
        F_JA3_CIPHERS_LEN,
        F_JA3_EXTENSIONS_LEN,

        F_JA3_C159,
        F_JA3_C57_61,
        F_JA3_C53,
        F_JA3_C60_49187,
        F_JA3_C53_49187,
        F_JA3_C52393_103,
        F_JA3_C49162,
        F_JA3_C50,
        F_JA3_C51,
        F_JA3_C255,
        F_JA3_C52392,
        F_JA3_C10,
        F_JA3_C157_49200,
        F_JA3_C49200,
        F_JA3_C49171_103,
        F_JA3_C49191_52394,
        F_JA3_C49192_52394,
        F_JA3_C65_52394,
        F_JA3_C157,
        F_JA3_C52393_49200,
        F_JA3_C49159,
        F_JA3_C4865,
        F_JA3_C158_61,
        F_JA3_C49196_47,
        F_JA3_C103,
        F_JA3_C103_49196,
        F_JA3_C52393_49188,
        F_JA3_C60_65,
        F_JA3_C49195_69,
        F_JA3_C154,
        F_JA3_C49187_49188,
        F_JA3_C49199_60,
        F_JA3_C49195_51,
        F_JA3_C49235,
        F_JA3_C47,
        F_JA3_C49169,
        F_JA3_C49249,
        F_JA3_C49171_60,
        F_JA3_C49188_49196,
        F_JA3_C61,
        F_JA3_C156_255,
        F_JA3_C47_57,
        F_JA3_C186,
        F_JA3_C49245,
        F_JA3_C156_52394,
        F_JA3_C20,
        F_JA3_C49188_49195,
        F_JA3_C52394_52392,
        F_JA3_C53_49162,
        F_JA3_C49191,
        F_JA3_C49245_49249,
        F_JA3_C49171,
        F_JA3_C53_52393,
        F_JA3_C51_49199,
        F_JA3_C49234,
        F_JA3_C49315,
        F_JA3_C158,
        F_JA3_C49187_49161,
        F_JA3_C107,
        F_JA3_C52394,
        F_JA3_C49162_61,
        F_JA3_C153,
        F_JA3_C49170,
        F_JA3_C156,
        F_JA3_C52393_60,
        F_JA3_C49195_49192,
        F_JA3_C7,
        F_JA3_C49187_103,
        F_JA3_C61_49172,
        F_JA3_C159_49188,
        F_JA3_C49171_49187,
        F_JA3_C49196_49188,
        F_JA3_C158_49161,
        F_JA3_C49193,

        F_ACCEPT_UNIQUE_KEYS_NUMBER,
        F_ACCEPT_ENCODING_UNIQUE_KEYS_NUMBER,
        F_ACCEPT_CHARSET_UNIQUE_KEYS_NUMBER,
        F_ACCEPT_LANGUAGE_UNIQUE_KEYS_NUMBER,
        F_ACCEPT_ANY_SPACE,
        F_ACCEPT_ENCODING_ANY_SPACE,
        F_ACCEPT_CHARSET_ANY_SPACE,
        F_ACCEPT_LANGUAGE_ANY_SPACE,
        F_ACCEPT_LANGUAGE_HAS_RUSSIAN,

        F_HAS_VALID_YANDEX_TRUST,
        F_HAS_VALID_AUTORU_TAMPER,

        F_AUTORU_JA3,
        F_AUTORU_SUBNET,
        F_AUTORU_UA,

        F_HAS_COOKIE_AMCUID,
        F_HAS_COOKIE_CURRENT_REGION_ID,
        F_HAS_COOKIE_CYCADA,
        F_HAS_COOKIE_LOCAL_OFFERS_FIRST,
        F_HAS_COOKIE_LR,
        F_HAS_COOKIE_MARKET_YS,
        F_HAS_COOKIE_ONSTOCK,
        F_HAS_COOKIE_YANDEX_HELP,
        F_HAS_COOKIE_CMP_MERGE,
        F_HAS_COOKIE_COMPUTER,
        F_HAS_COOKIE_HEAD_BANER,
        F_HAS_COOKIE_UTM_SOURCE,

        F_COOKIE_YOUNGER_THAN_MINUTE,
        F_COOKIE_YOUNGER_THAN_HOUR,
        F_COOKIE_YOUNGER_THAN_DAY,
        F_COOKIE_YOUNGER_THAN_WEEK,
        F_COOKIE_YOUNGER_THAN_MONTH,
        F_COOKIE_YOUNGER_THAN_THREE_MONTHES,
        F_COOKIE_OLDER_THAN_MONTH,
        F_COOKIE_OLDER_THAN_THREE_MONTHES,

        F_MARKET_JWS_STATE_FIRST,
        F_MARKET_JWS_STATE_LAST = F_MARKET_JWS_STATE_FIRST + static_cast<size_t>(EJwsStats::Count) - 1,

        F_MARKET_JA3_STATS_FIRST,
        F_MARKET_JA3_STATS_LAST = F_MARKET_JA3_STATS_FIRST + static_cast<size_t>(EMarketStats::Count) - 1,

        F_MARKET_SUBNET_STATS_FIRST,
        F_MARKET_SUBNET_STATS_LAST = F_MARKET_SUBNET_STATS_FIRST + static_cast<size_t>(EMarketStats::Count) - 1,

        F_MARKET_UA_STATS_FIRST,
        F_MARKET_UA_STATS_LAST = F_MARKET_UA_STATS_FIRST + static_cast<size_t>(EMarketStats::Count) - 1,

        F_QUERY_CLASS_FIRST,
        F_QUERY_CLASS_LAST = F_QUERY_CLASS_FIRST + NUM_WIZARD_QUERY_CLASSES - 1,

        F_QUERY_LANG_FIRST,
        F_QUERY_LANG_LAST = F_QUERY_LANG_FIRST + NUM_QUERY_LANGS - 1,

        F_PERSON_LANG_FIRST,
        F_PERSON_LANG_LAST = F_PERSON_LANG_FIRST + NUM_PERSON_LANGS - 1,

        F_CGI_PARAM_FIRST,
        F_CGI_PARAM_LAST = F_CGI_PARAM_FIRST + CgiParamName.size() - 1,

        F_COOKIE_PRESENCE_FIRST,
        F_COOKIE_PRESENCE_LAST = F_COOKIE_PRESENCE_FIRST + CookieName.size() - 1,

        F_HTTP_HEADER_PRESENCE_FIRST,
        F_HTTP_HEADER_PRESENCE_LAST = F_HTTP_HEADER_PRESENCE_FIRST + HttpHeaderName.size() - 1,

        F_NUM_FACTORS
    };

    // Plain factors without any aggregation or grouping by IP
    struct TRawFactors {
        float Factors[F_NUM_FACTORS];

        static constexpr size_t Count() {
            return F_NUM_FACTORS;
        }

        TRawFactors()
            : Factors()
        {
        }

        inline void SetBool(size_t index, bool value) {
            Factors[index] = (float)value;
        }

        void FillFromFeatures(const TRequestFeatures& rf);

        void Save(IOutputStream* out) const;
        void Load(IInputStream* in);

        void SetSyntaxFactors(const TWizardFactorsCalculator::TValues& wizardFeatures);
        void SetWizardFactors(const TWizardFactorsCalculator::TValues& wizardFeatures);
        void SetJwsStat(const TMarketJwsStatesStats& marketJwsStatesStats);
        void SetMarketStats(size_t offset, const TMarketStats& marketStats);
    };

    // Plain factors aggregated with exponential moving average
    struct TFactorsWithAggregation {
        enum {
            NUM_AGGREGATIONS = 3
        };

        TRawFactors NonAggregatedFactors;
        TRawFactors AggregatedFactors[NUM_AGGREGATIONS];

        static constexpr size_t RawFactorsWithAggregationCount() {
            return TRawFactors::Count() * (NUM_AGGREGATIONS + 1);
        }

        void Aggregate();

        void Save(IOutputStream* out) const;
        void Load(IInputStream* in, size_t count);

        float& GetFactor(size_t index);

        static TString GetFactorNameWithAggregationSuffix(size_t factorIndex);

        void FillNaN();
    };

    // Aggregated values grouped by IP and subnet masks
    struct TAllFactors {
        enum {
            MAX_IP_AGGREGATION_LEVEL = TUid::NumOfIpAggregationLevels - 1,
            JA3_AGGREGATION_LEVEL = MAX_IP_AGGREGATION_LEVEL + 1,
            NUM_FACTOR_VALUES = JA3_AGGREGATION_LEVEL + 2
        };

        static constexpr size_t NumLocalFactors =
            TFactorsWithAggregation::RawFactorsWithAggregationCount() * NUM_FACTOR_VALUES;

        inline TAllFactors() {
            Zero(FactorsWithAggregation);
        }

        TProcessorLinearizedFactors GetLinearizedFactors(
            const TAntirobotCookie* arCookie = nullptr
        ) const;

        static size_t AllFactorsCount() noexcept {
            return NumLocalFactors + ANTIROBOT_DAEMON_CONFIG.GlobalJsonConfig.LastVisitsRules.size();
        }

        // With aggregation and subnet suffixes, like "req_other^15|ip"
        static TString GetFactorNameByIndex(size_t index);

        TFactorsWithAggregation FactorsWithAggregation[NUM_FACTOR_VALUES];
    };

    constexpr TDuration SpravkaAge(TInstant arrivalTime, TInstant spravkaTime) noexcept {
        if (arrivalTime < spravkaTime || !spravkaTime.GetValue()) {
            return TDuration::Zero();
        }

        return arrivalTime - spravkaTime;
    }

} // namespace NAntiRobot

Y_DECLARE_PODTYPE(NAntiRobot::TFactorsWithAggregation);
