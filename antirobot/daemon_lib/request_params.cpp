#include "request_params.h"

#include "eventlog_err.h"
#include "host_ops.h"
#include "req_types.h"
#include "unified_agent_log.h"

#include <antirobot/lib/spravka.h>

#include <library/cpp/regex/pire/pire.h>

#include <util/string/join.h>
#include <util/string/type.h>

namespace NAntiRobot {

    namespace {
        const TStringBuf TEST_REQUEST_FOR_CAPTCHA_FROM_YANDEX = "Капчу!";

        template<size_t Size>
        class TBoolArray {
        public:
            TBoolArray(const size_t* trueIndices, size_t numIndices) {
                Zero(Values);
                for (size_t i = 0; i < numIndices; ++i)
                    Values[trueIndices[i]] = true;
            }

            bool operator[](size_t n) const {
                return Values[n];
            }

        private:
            bool Values[Size];
        };

#define BOOL_ARR(name, size, arr)    static const TBoolArray<size> name(arr, Y_ARRAY_SIZE(arr));

        const size_t REQ_TYPES_SEARCH[] = {
            REQ_YANDSEARCH,
            REQ_MSEARCH,
            REQ_FAMILYSEARCH,
            REQ_SCHOOLSEARCH,
            REQ_LARGESEARCH,
            REQ_XMLSEARCH,
            REQ_SITESEARCH,
            REQ_IMAGES,
            REQ_WEB,
        };

        const size_t REQ_TYPES_ACCOUNTABLE[] = {
            REQ_CATALOG_OFFERS,
            REQ_CATALOG_MODELS,
            REQ_CATALOG,
            REQ_COLLECTIONS,
            REQ_MODEL_PRICES,
            REQ_MODEL,
            REQ_GURU,
            REQ_OFFERS,
            REQ_MODEL_OPINIONS,
            REQ_BRANDS,
            REQ_COMPARE,
            REQ_SHOP,
            REQ_THEME,
            REQ_CHECKOUT,
            REQ_IMAGES,
            REQ_API,
            REQ_WEB,
        };

        const size_t REQ_TYPES_MAY_BAN_FOR[] = {
            REQ_CATALOG_OFFERS,
            REQ_CATALOG_MODELS,
            REQ_CATALOG,
            REQ_COLLECTIONS,
            REQ_MODEL_PRICES,
            REQ_MODEL,
            REQ_GURU,
            REQ_OFFERS,
            REQ_MODEL_OPINIONS,
            REQ_BRANDS,
            REQ_COMPARE,
            REQ_SHOP,
            REQ_THEME,
            REQ_CHECKOUT,
            REQ_NEWS_CLICK,
            REQ_ROUTE,
            REQ_ORG,
            REQ_TURBO,
            REQ_IMAGES,
            REQ_API,
            REQ_WEB,
            REQ_MAIN,
        };

        const size_t REQ_TYPES_CAPTCHA[] = {
            REQ_CATALOG_OFFERS,
            REQ_CATALOG_MODELS,
            REQ_CATALOG,
            REQ_COLLECTIONS,
            REQ_MODEL_PRICES,
            REQ_MODEL,
            REQ_GURU,
            REQ_OFFERS,
            REQ_MODEL_OPINIONS,
            REQ_BRANDS,
            REQ_COMPARE,
            REQ_SHOP,
            REQ_CHECKOUT,
            REQ_NEWS_CLICK,
            REQ_TURBO,
            REQ_IMAGES,
            REQ_API,
        };

        BOOL_ARR(IS_REQ_TYPE_WEB_SEARCH, REQ_NUMTYPES, REQ_TYPES_SEARCH);
        BOOL_ARR(IS_REQ_TYPE_ACCOUNTABLE, REQ_NUMTYPES, REQ_TYPES_ACCOUNTABLE);
        BOOL_ARR(MAY_BAN_FOR_REQ_TYPE, REQ_NUMTYPES, REQ_TYPES_MAY_BAN_FOR);
        BOOL_ARR(CAN_SHOW_CAPTCHA_FOR_REQ_TYPE, REQ_NUMTYPES, REQ_TYPES_CAPTCHA);
    }

    TRequest::TRequest()
        : PanicFlags(TPanicFlags::CreateFake())
        , Scheme(EScheme::Http)
        , ClientType(CLIENT_GENERAL)
        , ReqType(REQ_OTHER)
        , HostType(HOST_OTHER)
        , CaptchaReqType(CAPTCHAREQ_NONE)
        , BlockCategory(BC_UNDEFINED)
        , IsSearch(false)
        , InitiallyWasXmlsearch(false)
        , ForceShowCaptcha(false)
        , HasUnknownServiceHeader(false)
    {
    }

    NAntirobotEvClass::THeader TRequest::MakeLogHeader() const {
        return NAntiRobot::MakeEvlogHeader(ReqId, UserAddr, Uid, YandexUid, PartnerAddr, UniqueKey);
    }

    void TRequest::LogRequestData(TUnifiedAgentLogBackend& log) const {
        TString requestData;
        TStringOutput so(requestData);
        PrintData(so, /* forceMaskCookies := */ true);
        
        NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(NAntirobotEvClass::TRequestData(MakeLogHeader(), requestData));
        log.WriteLogRecord(rec);
    }

    bool TRequest::IsAccountableRequest() const {
        return IsSearch || IS_REQ_TYPE_ACCOUNTABLE[ReqType];
    }

    bool TRequest::IsImportantRequest() const {
        return MayBanFor() || CanShowCaptcha();
    }

    bool TRequest::IsNewsClickReq() const {
        return HostType == HOST_NEWS
                && CgiParams.Find(TStringBuf("cl4url")) != CgiParams.end()
                && CgiParams.Find(TStringBuf("text")) == CgiParams.end();
    }

    bool TRequest::IsMetricalReq() const {
        return ReqType == REQ_YANDSEARCH ||
               ReqType == REQ_MSEARCH;
    }

    bool TRequest::IsChrome() const {
        float Accept = static_cast<float>(HeaderPos[static_cast<size_t>(EHeaderOrderNames::Accept)]);
        float Host = static_cast<float>(HeaderPos[static_cast<size_t>(EHeaderOrderNames::Host)]);
        float UserAgent = static_cast<float>(HeaderPos[static_cast<size_t>(EHeaderOrderNames::UserAgent)]);
        float AcceptLanguage = static_cast<float>(HeaderPos[static_cast<size_t>(EHeaderOrderNames::AcceptLanguage)]);
        float AcceptEncoding = static_cast<float>(HeaderPos[static_cast<size_t>(EHeaderOrderNames::AcceptEncoding)]);
        return ((Accept && Accept < AcceptEncoding && AcceptEncoding < Host && Host < UserAgent)
                || (Accept && Accept < AcceptEncoding && AcceptEncoding < AcceptEncoding < AcceptLanguage && AcceptLanguage < Host && Host < UserAgent)
                || (AcceptEncoding && AcceptEncoding < AcceptLanguage && AcceptLanguage < Host && Host < UserAgent)
                || (Accept && Accept < AcceptLanguage && AcceptLanguage < Host && Host < UserAgent && UserAgent < AcceptEncoding)
                || (Accept && Accept < AcceptLanguage && AcceptLanguage < Host && Host < UserAgent));
    }

    bool TRequest::HasSpravka() const {
        TSpravka tmp;
        return tmp.ParseCookies(Cookies, GetCookiesDomainFromHost(Host)) == TSpravka::ECookieParseResult::Valid;
    }

    bool TRequest::HasMagicRequestForCaptcha() const {
        return CgiParams.Has(TStringBuf("text"), TEST_REQUEST_FOR_CAPTCHA_FROM_YANDEX) ||
                CgiParams.Has(TStringBuf("query"), TEST_REQUEST_FOR_CAPTCHA_FROM_YANDEX);
    }

    bool TRequest::MayBanFor() const {
        if (
            ForceCanShowCaptcha ||
            Y_UNLIKELY(PanicFlags.IsPanicMayBanForSet(HostType, ReqType))
        ) {
            return true;
        }

        if (ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(HostType, Tld).BansEnabled) {
            return IsSearch || MAY_BAN_FOR_REQ_TYPE[ReqType];
        }
        return false;
    }

    bool TRequest::CbbMayBan() const {
        if (ForceCanShowCaptcha) {
            return true;
        }

        if (!ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(HostType, Tld).BansEnabled) {
            return false;
        }

        return MayBan();
    }

    auto GetVersionSupportsCaptcha(const TRequest& req, bool IsAndroid) {
        if (IsAndroid) {
            auto AndroidVersion = ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(req.HostType, req.Tld).AndroidVersionSupportsCaptcha;
            return AndroidVersion;
        } else {
            auto IosVerssion = ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(req.HostType, req.Tld).IosVersionSupportsCaptcha;
            return IosVerssion;
        }
    }

    // version format: "{ui32}.{ui32}.{ui32}"
    // returns true, if version1 <= version2, false otherwise;
    bool CompareApiAutoVersions(TStringBuf version1, TStringBuf version2) {
        try {
            while(!version1.empty() && !version2.empty()) {
                ui32 token1 = FromString<ui32>(version1.NextTok('.'));
                ui32 token2 = FromString<ui32>(version2.NextTok('.'));
                if (token1 != token2) {
                    return token1 < token2;
                }
            }
        } catch (...) {
            return false;
        }
        return true;
    }

    bool CheckApiAutoVersion(const TRequest& req) {
        if (req.HostType == HOST_APIAUTO && req.ClientType == CLIENT_AJAX) {
            TString version = req.Headers.Get("x-android-app-version").data();
            bool isAndroid = !version.empty();
            if (version.empty()) {
                version = req.Headers.Get("x-ios-app-version");
                isAndroid = version.empty();
            }
            auto versionSupportsCaptcha = GetVersionSupportsCaptcha(req, isAndroid);
            if (!version.empty() && CompareApiAutoVersions(versionSupportsCaptcha, version)) {
                return true;
            }
        }
        return false;
    }

    bool TRequest::CanShowCaptcha() const {
        if (
            ForceCanShowCaptcha ||
            Y_UNLIKELY(PanicFlags.IsPanicCanShowCaptchaSet(HostType, ReqType))
        ) {
            return true;
        }

        return IsSearch || CAN_SHOW_CAPTCHA_FOR_REQ_TYPE[ReqType];
    }

    bool IsReqTypeSearch(EReqType reqType, const TStringBuf& reqDoc) {
        bool hasGeo = reqDoc.Contains("adresa-geolocation");  // https://st.yandex-team.ru/CAPTCHA-855
        return IS_REQ_TYPE_WEB_SEARCH[reqType] && !hasGeo;
    }

    float GetEnemyThreshold(const TRequest* req) {
        return ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(req->HostType, req->Tld).ProcessorThreshold;
    }

    TVector<TString> GetAllYqlRules() {
        TVector<TString> yqlRules;

        for (const auto& rules : {
            ANTIROBOT_DAEMON_CONFIG.GlobalJsonConfig.Rules,
            ANTIROBOT_DAEMON_CONFIG.GlobalJsonConfig.MarkRules
        }) {
            for (const auto& rule : rules) {
                yqlRules.push_back(JoinSeq(
                    " AND ",
                    MakeMappedRange(rule.Yql, [] (const auto& yqlRule) {
                        return "(" + yqlRule + ")";
                    })
                ));
            }
        }

        return yqlRules;
    }

    bool YqlBansEnabled() {
        return
            !ANTIROBOT_DAEMON_CONFIG.DisableBansByFactors &&
            !ANTIROBOT_DAEMON_CONFIG.DisableBansByYql;
    }
} // namespace NAntiRobot
