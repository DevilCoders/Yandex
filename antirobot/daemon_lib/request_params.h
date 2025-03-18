#pragma once

#include "antirobot_cookie.h"
#include "captcha_request.h"
#include "cbb_id.h"
#include "client_type.h"
#include "config_global.h"
#include "exp_bin.h"
#include "feat_addr.h"
#include "panic_flags.h"
#include "req_types.h"
#include "service_param_holder.h"
#include "uid.h"

#include <antirobot/idl/antirobot.ev.pb.h>
#include <antirobot/lib/addr.h>
#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/preview_uid.h>
#include <antirobot/lib/yx_searchprefs.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/http/cookies/cookies.h>
#include <library/cpp/http/cookies/lctable.h>
#include <library/cpp/langs/langs.h>

#include <util/datetime/base.h>
#include <util/generic/guid.h>
#include <util/generic/maybe.h>
#include <util/network/address.h>
#include <util/string/cast.h>
#include <util/string/type.h>

class IEventLog;

namespace NLCookie {
    struct IKeychain;
}

namespace NAntiRobot {
    struct TCaptchaRequest;
    class TRequest;

    namespace NCacheSyncProto {
        class TRequest;
    }
    class TUnifiedAgentLogBackend;
    using THeadersMap = TLowerCaseTable<TStringBuf>;
    using TCSHeadersMap = THashMultiMap<TStringBuf, TStringBuf, TStrBufHash>;

    enum class EScheme {
        Http        /* "http://" */,
        HttpSecure  /* "https://" */,
    };

    enum EJwsPlatform {
        Unknown,
        Android,
        Ios
    };

    enum class EJwsState {
        Invalid         /* "INVALID" */,
        Valid           /* "VALID" */,
        ValidExpired    /* "VALID_EXPIRED" */,
        Default         /* "DEFAULT" */,
        DefaultExpired  /* "DEFAULT_EXPIRED" */,
        Susp            /* "SUSP" */,
        SuspExpired     /* "SUSP_EXPIRED" */,
        Count
    };

    enum class EYandexTrustState {
        Invalid         /* "INVALID" */,
        Valid           /* "VALID" */,
        ValidExpired    /* "VALID_EXPIRED" */,
        Count
    };

    enum class EHeaderOrderNames {
        SecFetchDest                /* "Sec-Fetch-Dest" */,
        AcceptLanguage              /* "Accept-Language" */,
        Cookie                      /* "Cookie" */,
        UpgradeInsecureRequests     /* "Upgrade-Insecure-Requests" */,
        AcceptEncoding              /* "Accept-Encoding" */,
        Dnt                         /* "DNT" */,
        Origin                      /* "Origin" */,
        UserAgent                   /* "User-Agent" */,
        Host                        /* "Host" */,
        Referer                     /* "Referer" */,
        Authority                   /* "Authority" */,
        CacheControl                /* "Cache-Control" */,
        XForwardedProto             /* "X-Forwarded-Proto" */,
        KeepAlive                   /* "Keep-Alive" */,
        Pragma                      /* "Pragma" */,
        ProxyConnection             /* "Proxy-Connection" */,
        Rtt                         /* "RTT" */,
        Accept                      /* "Accept" */,
        Count,
    };

    class TRequest {
    protected:
        TPanicFlags PanicFlags;

    protected:
        TRequest();

    public:
        virtual ~TRequest() {}

        bool MayBanFor() const;
        bool CanShowCaptcha() const;
        bool CbbMayBan() const;
        bool WillBlock() const {
            return false;
        }

    public: // TRequestParams
        TAddr RawAddr;
        TString RequesterAddr;
        TAddr PartnerAddr;
        TFeaturedAddr UserAddr;
        TInstant ArrivalTime;
        EScheme Scheme;
        TStringBuf RequestMethod;
        TStringBuf RequestString;
        TStringBuf RequestProtocol;
        TStringBuf ReqId;
        TStringBuf ServiceReqId;
        TString ContentData;
        THeadersMap Headers;
        TCSHeadersMap CSHeaders;
        TStringBuf Doc;
        TStringBuf Cgi;
        TCgiParameters CgiParams;
        TString Request;
        TString Hodor;
        TString HodorHash;
        TStringBuf Uuid;

    public: // TParsedRequest
        THttpCookies Cookies;
        TStringBuf Host;
        TStringBuf HostWithPort;
        TStringBuf Tld;
        ELanguage Lang;
        EClientType ClientType;

        TUid Uid;
        TAddr SpravkaAddr;
        TInstant SpravkaTime;
        TString UniqueKey;
        ui64 RandomParam = 0;

        bool HasAnyICookie;
        bool HasValidICookie;
        bool HasValidOldICookie;
        bool HasAnyFuid;
        bool HasValidFuid;
        bool HasAnyLCookie;
        bool HasValidLCookie;
        bool HasAnySpravka;
        bool HasValidSpravka;
        bool HasValidSpravkaHash;
        bool SpravkaIgnored;
        bool Degradation;
        bool CbbPanicMode;
        bool InRobotSet = false;

        TStringBuf ICookie;
        ui64 LCookieUid;

        EReqType ReqType;
        EReqGroup ReqGroup;
        EVerticalReqGroup VerticalReqGroup;
        EHostType HostType;
        ECaptchaReqType CaptchaReqType;
        EBlockCategory BlockCategory;
        TYxSearchPrefs YxSearchPrefs;
        TStringBuf YandexUid;
        bool IsSearch;
        bool InitiallyWasXmlsearch;
        bool ForceShowCaptcha;
        bool HasUnknownServiceHeader;
        bool TrustedUser;

        TCaptchaRequest CaptchaRequest;

        TAntirobotCookie AntirobotCookie;
        bool AntirobotCookieDirty = false;

        struct TExpInfo {
            ui32 TestId;
            i8 Bucket;
        };

        TVector<TExpInfo> ExperimentsHeader;
        bool MarketExpSkipCaptcha = false;

        bool HasJws = false;
        EJwsPlatform JwsPlatform = EJwsPlatform::Unknown;
        EJwsState JwsState = EJwsState::Invalid;
        std::array<char, 16> JwsHash{};

        std::array<ui8, static_cast<size_t>(EHeaderOrderNames::Count)> HeaderPos = {};
        bool HasYandexTrust = false;
        EYandexTrustState YandexTrustState = EYandexTrustState::Invalid;

        bool HasValidAutoRuTamper = false;
        float CookieAge = 0.0;
        ui32 CurrentTimestamp;

        bool ForceCanShowCaptcha = false;

        bool SupportsBrotli = false;
        bool SupportsGzip = false;

    public: // TRequestParams
        bool IsPartnerRequest() const {
            return PartnerAddr.Valid();
        }

        inline TStringBuf CookiesString() const noexcept {
            return Headers.Get(TStringBuf("Cookie"));
        }

        inline TStringBuf Xff() const noexcept {
            return Headers.Get(TStringBuf("X-Forwarded-For"));
        }

        inline TStringBuf UserAgent() const noexcept {
            return Headers.Get(TStringBuf("User-Agent"));
        }

        EPreviewAgentType PreviewAgentType() const noexcept {
            const auto& userAgent = UserAgent();
            return GetPreviewAgentType(userAgent);
        }

        inline TStringBuf Referer() const noexcept {
            return Headers.Get(TStringBuf("Referer"));
        }

        inline TStringBuf Ja3() const noexcept {
            return Headers.Get(TStringBuf("X-Yandex-Ja3"));
        }

        inline TStringBuf P0f() const noexcept {
            return Headers.Get(TStringBuf("X-Yandex-P0f"));
        }

        inline TStringBuf Ja4() const noexcept {
            return Headers.Get(TStringBuf("X-Yandex-Ja4"));
        }

        inline bool MayBan() const noexcept {
            TMaybe<TStringBuf> may_ban;
            if (Headers.Has(TStringBuf("X-Antirobot-MayBanFor-Y"))) {
                may_ban = Headers.Get(TStringBuf("X-Antirobot-MayBanFor-Y"));
            }
            return may_ban.Defined() && IsTrue(*may_ban);
        }

        EExpBin ExperimentBin() const {
            const auto i = RandomParam % 20;
            return static_cast<EExpBin>(i <= 2 ? i : 3);
        }

        virtual void PrintData(IOutputStream& os, bool forceMaskCookies) const = 0;
        virtual void PrintHeaders(IOutputStream& os, bool forceMaskCookies) const = 0;
        virtual void SerializeTo(NCacheSyncProto::TRequest& serializedRequest) const = 0;

    public: // TParsedRequest
        NAntirobotEvClass::THeader MakeLogHeader() const;
        void LogRequestData(TUnifiedAgentLogBackend& log) const;

        bool IsAccountableRequest() const;
        bool IsImportantRequest() const;
        bool IsNewsClickReq() const;
        bool IsMetricalReq() const;
        bool IsChrome() const;

        bool HasSpravka() const;
        bool HasMagicRequestForCaptcha() const;
    };

    bool IsReqTypeSearch(EReqType reqType, const TStringBuf& reqDoc);

    auto GetVersionSupportsCaptcha(const TRequest& req, bool IsAndroid);
    bool CheckApiAutoVersion(const TRequest& req);

    float GetEnemyThreshold(const TRequest* req);

    TVector<TString> GetAllYqlRules();
    bool YqlBansEnabled();

} // namespace NAntiRobot
