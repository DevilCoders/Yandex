#include "request_features.h"

#include "config_global.h"
#include "environment.h"
#include "factors.h"

#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/bad_user_agents.h>
#include <antirobot/lib/ip_list.h>
#include <antirobot/lib/kmp_skip_search.h>
#include <antirobot/lib/reqtext_utils.h>
#include <antirobot/lib/yx_searchprefs.h>
#include <antirobot/lib/p0f_parser/parser.h>

#include <kernel/geo/utils.h>
#include <kernel/xmlreq/xmlsearch_params.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/regex/pire/regexp.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <util/charset/utf8.h>
#include <util/generic/singleton.h>
#include <util/generic/xrange.h>
#include <util/string/cast.h>

#include <cctype>
#include <memory>

namespace NAntiRobot {
    static const TKmpSkipSearch searchGroupsOnPage("groups-on-page=");
    static const TStrBufEqualToCaseLess strbufEqCaseless = TStrBufEqualToCaseLess();

    class TFsmRefererYandex : public NRegExp::TFsm {
        public:
            TFsmRefererYandex()
                : NRegExp::TFsm("\\W(yandex\\.|ya\\.ru)", NRegExp::TFsm::TOptions().SetSurround(true).SetCaseInsensitive(true))
            {
            }
    };

    static const NRegExp::TFsm& FsmRefererYandex() {
        return *Singleton<TFsmRefererYandex>();
    }

    inline bool CalcRefererFromYandex(const TStringBuf& r) {
        return NRegExp::TMatcher(FsmRefererYandex()).Match(r).Final();
    }

    static inline size_t CalcNumDocs(const TCgiParameters& cgi, const TYxSearchPrefs& prefs) {
        const TStringBuf numDoc = cgi.Get(TStringBuf("numdoc"));

        size_t result;
        if (!numDoc.empty() && TryFromString<size_t>(numDoc, result)) {
            return result;
        }

        return prefs.NumDoc;
    }

    static inline size_t CalcNumDocs(const TStringBuf& numDoc) {
        size_t result;
        if (!numDoc.empty() && TryFromString<size_t>(numDoc, result)) {
            return result;
        }

        return 10;
    }

    inline bool CalcAdvancedSearch(const TCgiParameters& cgi) {
        // simple check if this is advanced search
        return !cgi.Get(TStringBuf("wordforms")).empty()
            && !cgi.Get(TStringBuf("zone")).empty()
            && !cgi.Get(TStringBuf("within")).empty();
    }

    static inline bool CalcIsTimeSort(const TStringBuf& how) {
        return how == "tm"sv;
    }

    static inline bool CalcIsTimeSort(const TCgiParameters& cgiParams) {
        return cgiParams.Has(TStringBuf("how"), TStringBuf("tm"))
            || cgiParams.Has(TStringBuf("sortby"), TStringBuf("tm"));
    }

    static inline bool CalcIsBadProtocol(const TRequest& request) {
        return request.RequestProtocol != "http/1.1"sv;
    }

    static inline bool CalcIsConnectionKeepAlive(const TRequest& requestParams, bool isBadProtocol) {
        TStringBuf connectionHeader = requestParams.Headers.Get(TStringBuf("Connection"));
        if (strbufEqCaseless(connectionHeader, TStringBuf("close")))
            return false;
        if (strbufEqCaseless(connectionHeader, TStringBuf("keep-alive")))
            return true;

        TStringBuf keepAliveHeader = requestParams.Headers.Get(TStringBuf("Keep-Alive"));
        if (!!keepAliveHeader)
            return true;

        return !isBadProtocol;
    }

    static inline size_t CalcPageNum(const TCgiParameters& cgiParams) {
         return FromStringWithDefault<size_t>(cgiParams.Get(TStringBuf("p")));
    }

    static inline size_t CalcPageNum(const TString& value) {
         return FromStringWithDefault<size_t>(value);
    }

    static inline bool IsPostRequest(const TRequest& requestParams) {
        // request string is always in lower case (see server.cpp)
        return requestParams.RequestMethod == "post"sv;
    }

    static bool HandleXmlPost(const TRequest& requestParams, TXmlSearchRequest& xmlRequest) {
        TString content = requestParams.ContentData;

        UrlUnescape(content);
        CGIUnescape(content);

        size_t pos = content.find('<');
        if (pos == TString::npos)
            return false;

        content.erase(0,pos);

        TStringInput inp(content);
        return ParseXmlSearch(inp, xmlRequest);
    }

    static void HandleXmlGet(TRequestFeatures& rf) {
        const TCgiParameters&  cgiParams = rf.Request->CgiParams;
        rf.ReqText = ExtractReqTextFromCgi(cgiParams);
        rf.PageNum = CalcPageNum(cgiParams.Get(TStringBuf("page")));
        rf.IsTimeSort |= CalcIsTimeSort(cgiParams.Get(TStringBuf("sortby")));

        TCgiParameters::const_iterator toGroupby = cgiParams.Find(TStringBuf("groupby"));
        if (toGroupby != cgiParams.end()) {
            rf.NumDocs = 0;
            for (; toGroupby != cgiParams.end() && toGroupby->first == "groupby"sv; ++toGroupby) {
                TStringBuf groupsOnPage = searchGroupsOnPage.SearchInText(toGroupby->second).Skip(searchGroupsOnPage.Length()).Before('.').Before('&');
                rf.NumDocs = Max(rf.NumDocs, CalcNumDocs(groupsOnPage));
            }
        }
    }

    static float GetMouseMoveEntropy(const TRequest& req) {
        const float DEFAULT = 0.0f;

        if (req.ReqType != REQ_REDIR) {
            return DEFAULT;
        }

        const TStringBuf& url = req.Doc;
        const TStringBuf PARAM_NAME("-mc=");
        size_t valueBegin = url.find(PARAM_NAME);
        if (valueBegin == TStringBuf::npos) {
            return DEFAULT;
        }

        valueBegin += PARAM_NAME.size();
        if (valueBegin >= url.size() || !isdigit(url[valueBegin])) {
            return DEFAULT;
        }

        size_t valueEnd = valueBegin;
        while (valueEnd + 1 < url.size() && isdigit(url[valueEnd + 1])) {
            ++valueEnd;
        }
        if (valueEnd + 1 < url.size() && url[valueEnd + 1] == '.') {
            ++valueEnd;
            while (valueEnd + 1 < url.size() && isdigit(url[valueEnd + 1])) {
                ++valueEnd;
            }
        }
        return FromString<float>(url.SubStr(valueBegin, valueEnd - valueBegin + 1));
    }

    static float CalcAndUpdateRequestsPerSecond(const TRequestFeatures::TContext& ctx) {
        ctx.RpsFilter.RecalcUser(ctx.Request->Uid, ctx.Request->ArrivalTime);
        return ctx.RpsFilter.GetRpsById(ctx.Request->Uid);
    }

    static size_t GetUniqueKeysNumber(TStringBuf buf, char delimeter) {
        THashSet<TStringBuf> keys;
        while (!buf.empty()) {
            keys.insert(StripString(buf.NextTok(delimeter)));
        }
        return keys.size();
    }

    static void ParseHeaderFeatures(const THeadersMap& headers, TStringBuf header, float& unique_keys, float& any_space) {
        if (headers.Has(header)) {
            TStringBuf value = headers.Get(header);
            unique_keys = GetUniqueKeysNumber(value, ',');
            any_space = (value.find(' ') != std::string::npos);
        }
    }

    TRequestFeatures::TRequestFeatures(const TContext& ctx, TTimeStats& wizardCalcTimeStats, TTimeStats& factorsCalcTimeStats)
        : TBase({ctx.Request, ctx.ReloadableData, ctx.Detector})
        , Request(ctx.Request)
        , ReqType(ctx.Request->ReqType)
        , HostType(ctx.Request->HostType)
        , ReportType(REP_OTHER)
        , XffExists(false)
        , NoCookies(false)
        , NoReferer(false)
        , IsTimeSort(false)
        , IsMetricsRequest(false)
        , AdvancedSearch(false)
        , LCookieExists(false)
        , GskScore(0.f)
        , UserTime(0.f)
        , MouseMoveEntropy(GetMouseMoveEntropy(*ctx.Request))
        , RequestsPerSecond(0.f)
    {
        TPausableMeasureDuration factorsCalcTimeDuration{factorsCalcTimeStats, BaseElapsed};
        Zero(CgiParamPresent);
        Zero(CookiePresent);
        Zero(HttpHeaderPresent);

        const THttpCookies& cookies = Request->Cookies;

        const TCgiParameters& cgiParams = Request->CgiParams;

        XffExists = !Request->Xff().empty();
        NoCookies = cookies.Empty();

        {
            const TStringBuf referer = Request->Referer();
            NoReferer = referer.empty();
        }

        SecFetchDest = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::SecFetchDest)]);
        AcceptLanguage = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::AcceptLanguage)]);
        Cookie = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::Cookie)]);
        UpgradeInsecureRequests = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::UpgradeInsecureRequests)]);
        AcceptEncoding = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::AcceptEncoding)]);
        Dnt = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::Dnt)]);
        Origin = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::Origin)]);
        UserAgent = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::UserAgent)]);
        Host = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::Host)]);
        Referer = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::Referer)]);
        Authority = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::Authority)]);
        CacheControl = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::CacheControl)]);
        XForwardedProto = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::XForwardedProto)]);
        KeepAlive = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::KeepAlive)]);
        Pragma = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::Pragma)]);
        ProxyConnection = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::ProxyConnection)]);
        Rtt = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::Rtt)]);
        Accept = static_cast<float>(Request->HeaderPos[static_cast<size_t>(NAntiRobot::EHeaderOrderNames::Accept)]);

        ChromeHodor = Request->IsChrome();

        IsTimeSort = CalcIsTimeSort(cgiParams);
        AdvancedSearch = CalcAdvancedSearch(cgiParams);
        RequestsPerSecond = CalcAndUpdateRequestsPerSecond(ctx);

        //TODO - erase
        LCookieExists = cookies.Has(TStringBuf("L"));
        UserTime = Request->UserAddr.GeoRegion() != -1 ? ctx.ReloadableData->GeoChecker.Get()->GetRemoteTime(Request->UserAddr.GeoRegion(), Request->ArrivalTime)
                                                        : (Request->ArrivalTime.TimeT() % (24 * 60 * 60));

        IsMetricsRequest = StartsWith(cgiParams.Get(TStringBuf("pron")), TStringBuf("metrics-"));

        if (Request->InitiallyWasXmlsearch) {
            if (IsPostRequest(*Request)) {
                TXmlSearchRequest xmlSearchRequest;
                HandleXmlPost(*Request, xmlSearchRequest);

                if (!xmlSearchRequest.Groupings.empty()) {
                    NumDocs = 0;
                    for (TXmlSearchRequest::TGroupByList::const_iterator toGrouping = xmlSearchRequest.Groupings.begin(); toGrouping != xmlSearchRequest.Groupings.end(); ++toGrouping) {
                        NumDocs = Max(NumDocs, CalcNumDocs(toGrouping->GroupsOnPage));
                    }
                }

                ReqText = xmlSearchRequest.Query;
                PageNum = CalcPageNum(xmlSearchRequest.Page);
                IsTimeSort |= CalcIsTimeSort(xmlSearchRequest.SortBy);

            } else {
                HandleXmlGet(*this);
            }
        } else if (Request->IsSearch) {
            ReqText = ExtractReqTextFromCgi(cgiParams);
        }
        if (ReqText.size() > ANTIROBOT_DAEMON_CONFIG.MaxRequestLength) {
            ReqText.erase(ANTIROBOT_DAEMON_CONFIG.MaxRequestLength);
        }

        if (!ReqText.empty() && ctx.ReqWizard) {
            factorsCalcTimeDuration.Pause();
            {
                TMeasureDuration measureDuration{wizardCalcTimeStats};
                ctx.ReqWizard->CalcFactors(ReqText, Request->Cookies, WizardFeatures, Request->ReqId);
            }
            factorsCalcTimeDuration.Resume();
            if (ANTIROBOT_DAEMON_CONFIG.OutputWizardFactors) {
                TTempBufOutput out;
                out << "reqid: "sv << Request->ReqId << '\n';
                WizardFeatures.Print(out);
                out << '\n';
                Cerr << TStringBuf(out.Data(), out.Filled());
            }
        }

        for (auto i : xrange(CgiParamName.size())) {
            CgiParamPresent[i] = !cgiParams.Get(CgiParamName[i]).empty();
        }

        for (auto i : xrange(CookieName.size())) {
            CookiePresent[i] = cookies.Has(CookieName[i]);
        }

        const THeadersMap& headers = Request->Headers;
        for (auto i : xrange(HttpHeaderName.size())) {
            if (headers.Has(HttpHeaderName[i])) {
                HttpHeaderPresent[i] = true;
            }
        }

        SetProxyFlags(ctx);

        TStringBuf doc = ctx.Request->Doc;
        doc.ChopSuffix("/");
        if (ctx.Request->HostType == HOST_USLUGI) {
            enum class EUslugiRuchkas {
                None,
                GetWorkerPhone,
                OtherApi,
            };

            static const TRegexpClassifier<EUslugiRuchkas> uslugiClassifier(
                    {
                        {"(/uslugi)?/api/get_worker_phone", EUslugiRuchkas::GetWorkerPhone},
                        {"(/uslugi)?/api/(map|get_views_count_for_orders|get_products_to_buy|get_worker_active_reaction_packs)", EUslugiRuchkas::OtherApi},
                        {"(/uslugi)?/(cab|api)/orders", EUslugiRuchkas::OtherApi},
                    }, EUslugiRuchkas::None);
            const EUslugiRuchkas type = uslugiClassifier[doc];

            UslugiGetWorkerPhoneHoneypots = type == EUslugiRuchkas::GetWorkerPhone;
            UslugiOtherApiHoneypots = type == EUslugiRuchkas::OtherApi;
        }
        if (ctx.Request->HostType == HOST_KINOPOISK) {
            KinopoiskFilmsHoneypots = ctx.ReloadableData->KinopoiskFilmsHoneypots.Get()->GetValue(doc, 0);
            KinopoiskNamesHoneypots = ctx.ReloadableData->KinopoiskNamesHoneypots.Get()->GetValue(doc, 0);
        }
        if (ctx.Request->HostType == HOST_MARKET) {
            enum class EMarketClassifier {
                None,
                Human1Honeypots,
                Human2Honeypots,
                Human3Honeypots,
            };
            static const TRegexpClassifier<EMarketClassifier> marketClassifier(
                    {
                        {"/api/(shop|turbo|compare|product|search-feedback|remote-proxy-resolve|clickproxy|files|tires|settings|resolve|grades|faq|report|timers).*", EMarketClassifier::Human1Honeypots},
                        {"/(search/filters|catalog/filters|compare|product.*/videos|product.*/special).*", EMarketClassifier::Human2Honeypots},
                        {"/(my|bonus|brands|lp|promotion).*", EMarketClassifier::Human3Honeypots},
                    }, EMarketClassifier::None);
            const EMarketClassifier type = marketClassifier[doc];

            MarketHuman1Honeypots = (type == EMarketClassifier::Human1Honeypots);
            MarketHuman2Honeypots = (type == EMarketClassifier::Human2Honeypots);
            MarketHuman3Honeypots = (type == EMarketClassifier::Human3Honeypots);
        }
        if (ctx.Request->HostType == HOST_AUTORU) {
            AutoruOfferHoneypot = ctx.AutoruOfferDetector->Process(doc);
        }

        if (ctx.Request->IsPartnerRequest() && ctx.Request->InitiallyWasXmlsearch) {
            ApplyHackForXmlPartners();
        }
    }

    bool TRequestFeatures::IsYandsearchDzen() const {
        const TCgiParameters&  cgiParams = Request->CgiParams;
        return HostType == HOST_WEB && Request->IsSearch && cgiParams.Find(TStringBuf("randomtext")) != cgiParams.end();
    }

    void TRequestFeatures::ApplyHackForXmlPartners() {
        NoCookies = false;
        NoReferer = false;
        RefererFromYandex = true;
        IsBadUserAgent = false;
        IsBadUserAgentNew = false;
        IsConnectionKeepAlive = true;
        if (!ReqText.empty()) {
            CgiParamPresent[CGI_TEXT_INDEX] = true;
            CgiParamPresent[CGI_LR_INDEX] = true;
        }
    }

    TRequestFeaturesBase::TRequestFeaturesBase(const TContext& ctx) {
        TInstant start{TInstant::Now()};
        const TCgiParameters& cgiParams = ctx.Request->CgiParams;
        NumDocs = CalcNumDocs(cgiParams, ctx.Request->YxSearchPrefs);
        PageNum = CalcPageNum(cgiParams);

        const THeadersMap& headers = ctx.Request->Headers;
        size_t numInternalHeaders = 0;
        for (auto i : xrange(HttpInternalHeaderName.size())) {
            if (headers.Has(HttpInternalHeaderName[i])) {
                ++numInternalHeaders;
            }
        }
        size_t numKnownHeaders = 0;
        for (auto i : xrange(HttpHeaderName.size())) {
            if (headers.Has(HttpHeaderName[i])) {
                ++numKnownHeaders;
            }
        }
        HeadersCount = headers.Size() - numInternalHeaders;
        HaveUnknownHeaders = HeadersCount != numKnownHeaders;

        {
            const TStringBuf referer = ctx.Request->Referer();
            if (!referer.empty()) {
                RefererFromYandex = CalcRefererFromYandex(referer);
            }
        }

        IsBadProtocol = CalcIsBadProtocol(*ctx.Request);
        IsBadUserAgent = ctx.ReloadableData->BadUserAgents.IsUserAgentBad(ctx.Request->UserAgent());
        IsBadUserAgentNew = ctx.ReloadableData->BadUserAgentsNew.IsUserAgentBad(ctx.Request->UserAgent());
        IsConnectionKeepAlive = CalcIsConnectionKeepAlive(*ctx.Request, IsBadProtocol);

        const auto ja3 = ctx.Request->Ja3();
        if (!ja3.empty()) {
            FraudJa3 = ctx.ReloadableData->FraudJa3.Get()->GetValue(ja3);
            MarketJwsStatesStats = ctx.ReloadableData->MarketJwsStatesStats.Get()->GetValue(ja3);
            MarketJa3Stats = ctx.ReloadableData->MarketJa3Stats.Get()->GetValue(ja3);
        }
        FraudSubnetNew = ctx.ReloadableData->FraudSubnetNew.Get()->GetValue(ctx.Request->RawAddr);

        if (ctx.Request->RawAddr.IsIp4() || ctx.Request->RawAddr.IsIp6()) {
            const TString addr = ctx.Request->RawAddr.GetSubnet(ctx.Request->RawAddr.IsIp4() ? 24 : 64).ToString();
            MarketSubnetStats = ctx.ReloadableData->MarketSubnetStats.Get()->GetValue(addr);
        }

        const TStringBuf user_agent = ctx.Request->UserAgent();
        if (ctx.Request->HostType == HOST_AUTORU) {
            if (!ja3.empty()) {
                AutoruJa3 = ctx.ReloadableData->AutoruJa3.Get()->GetValue(ja3);
            }
            AutoruSubnet = ctx.ReloadableData->AutoruSubnet.Get()->GetValue(ctx.Request->RawAddr);
            if (!user_agent.empty()) {
                AutoruUA = ctx.ReloadableData->AutoruUA.Get()->GetValue(user_agent);
            }
        }

        const THttpCookies& cookies = ctx.Request->Cookies;
        HasCookieAmcuid = cookies.Has("amcuid");
        HasCookieCurrentRegionId = cookies.Has("currentRegionId");
        HasCookieCycada = cookies.Has("cycada");
        HasCookieLocalOffersFirst = cookies.Has("local-offers-first");
        HasCookieLr = cookies.Has("lr");
        HasCookieMarketYs = cookies.Has("market_ys");
        HasCookieOnstock = cookies.Has("onstock");
        HasCookieYandexHelp = cookies.Has("yandex_help");
        HasCookieCmpMerge = cookies.Has("cmp-merge");
        HasCookieComputer = cookies.Has("computer");
        HasCookieHeadBaner = cookies.Has("head_baner");
        HasCookieUtmSource = cookies.Has("utm_source");
        
        if (ctx.Request->CookieAge) {
            CookieYoungerMinute = ctx.Request->CookieAge < 60;
            CookieYoungerHour = ctx.Request->CookieAge < 3600;
            CookieYoungerDay = ctx.Request->CookieAge < 86400;
            CookieYoungerWeek = ctx.Request->CookieAge < 604800;
            CookieYoungerMonth = ctx.Request->CookieAge < 2592000;
            CookieYoungerThreeMonthes = ctx.Request->CookieAge < 7776000;
            CookieOlderMonth = ctx.Request->CookieAge > 2592000;
            CookieOlderThreeMonthes = ctx.Request->CookieAge > 7776000;
        }

        const auto uaParams = ctx.Detector->detect({user_agent.data(), user_agent.size()});
        if (!user_agent.empty()) {
            MarketUAStats = ctx.ReloadableData->MarketUAStats.Get()->GetValue(user_agent);
        }

        if (auto res = uaParams.find("isRobot"); res != uaParams.end()) {
            TryFromString<bool>(res->second, IsRobot);
        }
        if (auto res = uaParams.find("isMobile"); res != uaParams.end()) {
            TryFromString<bool>(res->second, IsMobile);
        }
        if (auto res = uaParams.find("isBrowser"); res != uaParams.end()) {
            TryFromString<bool>(res->second, IsBrowser);
        }
        if (auto res = uaParams.find("historySupport"); res != uaParams.end()) {
            TryFromString<bool>(res->second, HistorySupport);
        }
        if (auto res = uaParams.find("isEmulator"); res != uaParams.end()) {
            TryFromString<bool>(res->second, IsEmulator);
        }
        if (auto res = uaParams.find("BrowserEngine"); res != uaParams.end() && res->second != "Unknown") {
            IsBrowserEngine = true;
        }
        if (auto res = uaParams.find("BrowserEngineVersion"); res != uaParams.end() && res->second != "Unknown") {
            IsBrowserEngineVersion = true;
        }
        if (auto res = uaParams.find("BrowserVersion"); res != uaParams.end()) {
            IsBrowserVersion = true;
        }
        if (auto res = uaParams.find("OSName"); res != uaParams.end()) {
            IsOSName = true;
        }
        if (auto res = uaParams.find("OSVersion"); res != uaParams.end()) {
            IsOSVersion = true;
        }
        if (auto res = uaParams.find("BrowserName"); res != uaParams.end()) {
            IsIE = EqualToOneOf(res->second, "MSIE", "IEMobile");
        }
        if (auto res = uaParams.find("OSFamily"); res != uaParams.end() && res->second != "Unknown") {
            IsOSFamily = true;
            if (res->second == "Android") {
                IsOSFamilyAndroid = true;
            } else if (res->second == "Windows") {
                IsOSFamilyWindows = true;
            } else if (res->second == "iOS") {
                IsOSFamilyiOS = true;
            } else if (res->second == "MacOS") {
                IsOSFamilyMacOS = true;
            } else if (res->second == "Linux") {
                IsOSFamilyLinux = true;
            }
        }
        if (auto res = uaParams.find("ITP"); res != uaParams.end()) {
            TryFromString<bool>(res->second, ITP);
        }
        if (auto res = uaParams.find("ITPFakeCookie"); res != uaParams.end()) {
            TryFromString<bool>(res->second, ITPFakeCookie);
        }
        if (auto res = uaParams.find("localStorageSupport"); res != uaParams.end()) {
            TryFromString<bool>(res->second, localStorageSupport);
        }

        const auto p0fString = ctx.Request->P0f();
        if (!p0fString.empty()) {
            try {
                P0fParser::TP0f p0f = P0fParser::StringToP0f(p0fString);

                P0fOlen = p0f.Olen;
                P0fVersion = p0f.Version;
                P0fObservedTTL = p0f.ObservedTTL;

                if (p0f.EOL) {
                    P0fEOL = *p0f.EOL;
                }
                if (p0f.ITTLDistance) {
                    P0fITTLDistance = *p0f.ITTLDistance;
                }
                if (p0f.UnknownOptionID) {
                    P0fUnknownOptionID = *p0f.UnknownOptionID;
                }
                if (p0f.MSS) {
                    P0fMSS = *p0f.MSS;
                }
                if (p0f.WSize) {
                    P0fWSize = *p0f.WSize;
                }
                if (p0f.Scale) {
                    P0fScale = *p0f.Scale;
                }

                P0fLayoutNOP = p0f.LayoutNOP;
                P0fLayoutMSS = p0f.LayoutMSS;
                P0fLayoutWS = p0f.LayoutWS;
                P0fLayoutSOK = p0f.LayoutSOK;
                P0fLayoutSACK = p0f.LayoutSACK;
                P0fLayoutTS = p0f.LayoutTS;

                P0fQuirksDF = p0f.QuirksDF;
                P0fQuirksIDp = p0f.QuirksIDp;
                P0fQuirksIDn = p0f.QuirksIDn;
                P0fQuirksECN = p0f.QuirksECN;
                P0fQuirks0p = p0f.Quirks0p;
                P0fQuirksFlow = p0f.QuirksFlow;
                P0fQuirksSEQn = p0f.QuirksSEQn;
                P0fQuirksACKp = p0f.QuirksACKp;
                P0fQuirksACKn = p0f.QuirksACKn;
                P0fQuirksUPTRp = p0f.QuirksUPTRp;
                P0fQuirksURGFp = p0f.QuirksURGFp;
                P0fQuirksPUSHFp = p0f.QuirksPUSHFp;
                P0fQuirksTS1n = p0f.QuirksTS1n;
                P0fQuirksTS2p = p0f.QuirksTS2p;
                P0fQuirksOPTp = p0f.QuirksOPTp;
                P0fQuirksEXWS = p0f.QuirksEXWS;
                P0fQuirksBad = p0f.QuirksBad;

                P0fPClass = p0f.PClass;
            } catch (...) {
                Cerr << CurrentExceptionMessage() << '\n';
            }
        }

        const auto ja3String = ctx.Request->Ja3();
        if (!ja3.empty()) {
            try {
                Ja3 = Ja3Parser::StringToJa3(ja3String);
            } catch (...) {
                Cerr << CurrentExceptionMessage() << '\n';
            }
        }

        ParseHeaderFeatures(headers, "Accept", AcceptUniqueKeysNumber, AcceptAnySpace);
        ParseHeaderFeatures(headers, "Accept-Encoding", AcceptEncodingUniqueKeysNumber, AcceptEncodingAnySpace);
        ParseHeaderFeatures(headers, "Accept-Charset", AcceptCharsetUniqueKeysNumber, AcceptCharsetAnySpace);
        if (headers.Has("Accept-Language")) {
            ParseHeaderFeatures(headers, "Accept-Language", AcceptLanguageUniqueKeysNumber, AcceptLanguageAnySpace);
            TStringBuf value = headers.Get("Accept-Language");
            AcceptLanguageHasRussian = (value.Contains("ru") || value.Contains("RU"));
        }

        BaseElapsed = TInstant::Now() - start;
    }

    void TRequestFeatures::SetProxyFlags(const TContext& ctx) {
        const NThreading::TRcuAccessor<TGeoChecker>& geoChecker = ctx.ReloadableData->GeoChecker;

        TString currentIp = ctx.Request->UserAddr.ToString();
        const NGeobase::TIpBasicTraits traits = geoChecker.Get()->GetGeobase().GetBasicTraitsByIp(currentIp);

        IsProxy   = traits.IsProxy();
        IsTor     = traits.IsTor();
        IsVpn     = traits.IsVpn();
        IsHosting = traits.IsHosting();
    }

    void TCacherRequestFeatures::SetProxyFlags(const TContext& ctx) {
        IsProxy   = ctx.Request->UserAddr.IsProxy();
        IsTor     = ctx.Request->UserAddr.IsTor();
        IsVpn     = ctx.Request->UserAddr.IsVpn();
        IsHosting = ctx.Request->UserAddr.IsHosting();
    }

    TCacherRequestFeatures::TCacherRequestFeatures(const TContext& ctx, TTimeStats& cacherFactorsCalcTimeStats)
        : TBase({ctx.Request, ctx.ReloadableData, ctx.Detector})
    {
        TMeasureDuration factorsCalcTimeDuration{cacherFactorsCalcTimeStats, BaseElapsed};

        const TCgiParameters& cgiParams = ctx.Request->CgiParams;
        for (auto i : xrange(CacherCgiParamName.size())) {
            CgiParamPresent[i] = !cgiParams.Get(CacherCgiParamName[i]).empty();
        }

        const THttpCookies& cookies = ctx.Request->Cookies;
        for (auto i : xrange(CacherCookieName.size())) {
            CookiePresent[i] = cookies.Has(CacherCookieName[i]);
        }

        const THeadersMap& headers = ctx.Request->Headers;
        for (auto i : xrange(CacherHttpHeaderName.size())) {
            HttpHeaderPresent[i] = headers.Has(CacherHttpHeaderName[i]);
        }

        FraudSubnet = ctx.ReloadableData->FraudSubnet.Get()->GetValue(ctx.Request->RawAddr);

        IpSubnetMatch = CalcIpDistance(ctx.Request->UserAddr, ctx.Request->SpravkaAddr);
        SpravkaLifetime = static_cast<float>(SpravkaAge(ctx.Request->ArrivalTime, ctx.Request->SpravkaTime).SecondsFloat());
        InRobotSet = ctx.Robots->Contains(ctx.Request->HostType, ctx.Request->Uid);

        HasValidSpravka = ctx.Request->HasValidSpravka;

        for (const auto& rule : ANTIROBOT_DAEMON_CONFIG.GlobalJsonConfig.LastVisitsRules) {
            LastVisits.push_back(ctx.Request->AntirobotCookie.LastVisitsCookie.Get(rule.Id));
        }

        SetProxyFlags(ctx);
    }

} // namespace NAntiRobot
