#pragma once

#include "cgi_params.h"
#include "cookie_names.h"
#include "entity_dictionary.h"
#include "geo_checker.h"
#include "http_headers.h"
#include "req_types.h"
#include "request_params.h"
#include "robot_set.h"
#include "rps_filter.h"
#include "time_stats.h"
#include "wizards.h"

#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/ja3_parser/parser.h>

#include <metrika/uatraits/include/uatraits/detector.hpp>

#include <kernel/xmlreq/xmlsearch_params.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/generic/noncopyable.h>

namespace NAntiRobot {
    class TIpList;
    struct TYxSearchPrefs;
    class TBadUserAgents;
    class TWizardFactorsCalculator;
    class TRobotContainer;
    class TAutoruOfferDetector;
    struct TEnv;

    struct TRequestFeaturesBase : public TNonCopyable {
        TDuration BaseElapsed;
        size_t NumDocs = 10;
        size_t PageNum = 0;
        size_t HeadersCount = 0;

        bool IsProxy = false;
        bool IsTor = false;
        bool IsVpn = false;
        bool IsHosting = false;

        bool RefererFromYandex = false;
        bool IsBadProtocol = false;
        bool IsBadUserAgent = false;
        bool IsBadUserAgentNew = false;
        bool IsConnectionKeepAlive = false;
        bool HaveUnknownHeaders = false;

        // factors from dictionaries
        float FraudJa3 = std::numeric_limits<float>::quiet_NaN();
        float FraudSubnetNew = std::numeric_limits<float>::quiet_NaN();
        float AutoruJa3 = std::numeric_limits<float>::quiet_NaN();
        float AutoruSubnet = std::numeric_limits<float>::quiet_NaN();
        float AutoruUA = std::numeric_limits<float>::quiet_NaN();
        TMarketJwsStatesStats MarketJwsStatesStats;
        TMarketStats MarketJa3Stats;
        TMarketStats MarketSubnetStats;
        TMarketStats MarketUAStats;

        // factors from cookies
        bool HasCookieAmcuid = false;
        bool HasCookieCurrentRegionId = false;
        bool HasCookieCycada = false;
        bool HasCookieLocalOffersFirst = false;
        bool HasCookieLr = false;
        bool HasCookieMarketYs = false;
        bool HasCookieOnstock = false;
        bool HasCookieYandexHelp = false;
        bool HasCookieCmpMerge = false;
        bool HasCookieComputer = false;
        bool HasCookieHeadBaner = false;
        bool HasCookieUtmSource = false;

        float CookieYoungerMinute = std::numeric_limits<float>::quiet_NaN();
        float CookieYoungerHour = std::numeric_limits<float>::quiet_NaN();
        float CookieYoungerDay = std::numeric_limits<float>::quiet_NaN();
        float CookieYoungerWeek = std::numeric_limits<float>::quiet_NaN();
        float CookieYoungerMonth = std::numeric_limits<float>::quiet_NaN();
        float CookieYoungerThreeMonthes = std::numeric_limits<float>::quiet_NaN();
        float CookieOlderMonth = std::numeric_limits<float>::quiet_NaN();
        float CookieOlderThreeMonthes = std::numeric_limits<float>::quiet_NaN();

        // factors from UA traits
        bool IsRobot = false;
        bool IsMobile = false;
        bool IsBrowser = true;
        bool HistorySupport = false;
        bool IsEmulator = false;
        bool IsBrowserEngine = false;
        bool IsBrowserEngineVersion = false;
        bool IsBrowserVersion = false;
        bool IsOSName = false;
        bool IsOSVersion = false;
        bool IsOSFamily = false;
        bool IsOSFamilyAndroid = false;
        bool IsOSFamilyWindows = false;
        bool IsOSFamilyiOS = false;
        bool IsOSFamilyMacOS = false;
        bool IsOSFamilyLinux = false;
        bool ITP = false;
        bool ITPFakeCookie = false;
        bool localStorageSupport = false;
        bool ChromeHodor = false;
        bool IsIE = false;

        float SecFetchDest = 0;
        float AcceptLanguage = 0;
        float Cookie = 0;
        float UpgradeInsecureRequests = 0;
        float AcceptEncoding = 0;
        float Dnt = 0;
        float Origin = 0;
        float UserAgent = 0;
        float Host = 0;
        float Referer = 0;
        float Authority = 0;
        float CacheControl = 0;
        float XForwardedProto = 0;
        float KeepAlive = 0;
        float Pragma = 0;
        float ProxyConnection = 0;
        float Rtt = 0;
        float Accept = 0;

        // factors from p0f
        float P0fITTLDistance = std::numeric_limits<float>::quiet_NaN();
        float P0fMSS = std::numeric_limits<float>::quiet_NaN();
        float P0fWSize = std::numeric_limits<float>::quiet_NaN();
        float P0fScale = std::numeric_limits<float>::quiet_NaN();
        float P0fEOL = std::numeric_limits<float>::quiet_NaN();
        float P0fUnknownOptionID = std::numeric_limits<float>::quiet_NaN();
        float P0fVersion = 0;
        float P0fObservedTTL = 0;
        float P0fOlen = 0;

        bool P0fLayoutNOP = false;
        bool P0fLayoutMSS = false;
        bool P0fLayoutWS = false;
        bool P0fLayoutSOK = false;
        bool P0fLayoutSACK = false;
        bool P0fLayoutTS = false;

        bool P0fQuirksDF = false;
        bool P0fQuirksIDp = false;
        bool P0fQuirksIDn = false;
        bool P0fQuirksECN = false;
        bool P0fQuirks0p = false;
        bool P0fQuirksFlow = false;
        bool P0fQuirksSEQn = false;
        bool P0fQuirksACKp = false;
        bool P0fQuirksACKn = false;
        bool P0fQuirksUPTRp = false;
        bool P0fQuirksURGFp = false;
        bool P0fQuirksPUSHFp = false;
        bool P0fQuirksTS1n = false;
        bool P0fQuirksTS2p = false;
        bool P0fQuirksOPTp = false;
        bool P0fQuirksEXWS = false;
        bool P0fQuirksBad = false;

        bool P0fPClass = false;

        Ja3Parser::TJa3 Ja3;

        float AcceptUniqueKeysNumber = std::numeric_limits<float>::quiet_NaN();
        float AcceptEncodingUniqueKeysNumber = std::numeric_limits<float>::quiet_NaN();
        float AcceptCharsetUniqueKeysNumber = std::numeric_limits<float>::quiet_NaN();
        float AcceptLanguageUniqueKeysNumber = std::numeric_limits<float>::quiet_NaN();
        float AcceptAnySpace = std::numeric_limits<float>::quiet_NaN();
        float AcceptEncodingAnySpace = std::numeric_limits<float>::quiet_NaN();
        float AcceptCharsetAnySpace = std::numeric_limits<float>::quiet_NaN();
        float AcceptLanguageAnySpace = std::numeric_limits<float>::quiet_NaN();
        float AcceptLanguageHasRussian = std::numeric_limits<float>::quiet_NaN();

        struct TContext {
            const TRequest* Request;
            const TReloadableData* ReloadableData;
            const uatraits::detector* Detector;
        };

        explicit TRequestFeaturesBase(const TContext& ctx);
    };

    struct TRequestFeatures : public TRequestFeaturesBase {
        using TBase = TRequestFeaturesBase;

        const TRequest* Request;
        EReqType ReqType;       // TODO: remove these. Use those from Request
        EHostType HostType;
        EReportType ReportType;

        TString ReqText;

        bool XffExists;
        bool NoCookies;
        bool NoReferer;
        bool IsTimeSort;
        bool IsMetricsRequest;
        bool AdvancedSearch;

        bool LCookieExists;

        TWizardFactorsCalculator::TValues WizardFeatures;
        bool CgiParamPresent[CgiParamName.size()];
        bool CookiePresent[CookieName.size()];
        bool HttpHeaderPresent[HttpHeaderName.size()];
        float GskScore;
        float UserTime;
        float MouseMoveEntropy;

        float RequestsPerSecond;

        bool UslugiGetWorkerPhoneHoneypots = false;
        bool UslugiOtherApiHoneypots = false;

        float KinopoiskFilmsHoneypots = 0;
        float KinopoiskNamesHoneypots = 0;
        bool MarketHuman1Honeypots = false;
        bool MarketHuman2Honeypots = false;
        bool MarketHuman3Honeypots = false;

        bool AutoruOfferHoneypot = false;

        struct TContext {
            const TRequest* Request;
            const TWizardFactorsCalculator* ReqWizard;
            const TReloadableData* ReloadableData;
            const uatraits::detector* Detector;
            const TAutoruOfferDetector* AutoruOfferDetector;
            TRpsFilter& RpsFilter;
        };

        explicit TRequestFeatures(const TContext& ctx, TTimeStats& wizardCalcTimeStats, TTimeStats& factorsCalcTimeStats);

        bool IsYandsearchDzen() const;
    private:
        void ApplyHackForXmlPartners();
        void SetProxyFlags(const TContext& ctx);
    };

    // TODO: add hacks for xml like in TRequestFeatures
    struct TCacherRequestFeatures : public TRequestFeaturesBase {
        using TBase = TRequestFeaturesBase;

        size_t IpSubnetMatch = 0;
        float SpravkaLifetime = 0.0F;


        // factors from dictionaries
        // TODO(ashagarov) remove it after update catboost formula
        float FraudSubnet = std::numeric_limits<float>::quiet_NaN();

        std::array<bool, CacherCgiParamName.size()> CgiParamPresent;
        std::array<bool, CacherCookieName.size()> CookiePresent;
        std::array<bool, CacherHttpHeaderName.size()> HttpHeaderPresent;

        bool InRobotSet = false;
        bool HasValidSpravka = false;

        TVector<ui8> LastVisits;

        struct TContext {
            const TRequest* Request;
            const TReloadableData* ReloadableData;
            const TRobotSet* Robots;
            const uatraits::detector* Detector;
        };

        TCacherRequestFeatures() = default;

        explicit TCacherRequestFeatures(const TContext& ctx, TTimeStats& cacherFactorsCalcTimeStats);

    private:
        void SetProxyFlags(const TContext& ctx);
    };
}
