#include "user_reply.h"

#include "antirobot_cookie.h"
#include "bad_request_handler.h"
#include "blocked_handler.h"
#include "cache_sync.h"
#include "cacher_factors.h"
#include "captcha_check.h"
#include "captcha_descriptor.h"
#include "captcha_format.h"
#include "captcha_gen.h"
#include "captcha_key.h"
#include "captcha_page_params.h"
#include "captcha_stat.h"
#include "client_type.h"
#include "config_global.h"
#include "environment.h"
#include "eventlog_err.h"
#include "fullreq_info.h"
#include "get_host_name_request.h"
#include "host_ops.h"
#include "many_requests_handler.h"
#include "neh_requesters.h"
#include "partners_captcha_stat.h"
#include "reloadable_data.h"
#include "request_classifier.h"
#include "request_context.h"
#include "request_features.h"
#include "return_path.h"
#include "robot_detector.h"
#include "static_handler.h"
#include "time_stats.h"
#include "training_set_generator.h"
#include "user_metric.h"

#include <antirobot/idl/antirobot.ev.pb.h>
#include <antirobot/idl/cache_sync.pb.h>
#include <antirobot/lib/antirobot_response.h>
#include <antirobot/lib/http_helpers.h>
#include <antirobot/lib/keyring.h>
#include <antirobot/lib/kmp_skip_search.h>
#include <antirobot/lib/spravka.h>
#include <antirobot/lib/uri.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/html/pcdata/pcdata.h>
#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/http/static/static.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/uri/http_url.h>

#include <util/datetime/base.h>
#include <util/network/hostip.h>
#include <util/random/random.h>
#include <util/stream/format.h>
#include <util/stream/str.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/printf.h>
#include <util/string/vector.h>
#include <util/system/hostname.h>

namespace NAntiRobot {

namespace {
    const TStringBuf BAN_COOKIE_NAME = "YX_SHOW_CAPTCHA";
    const TStringBuf MOBILE_PREFIX = "m.";
    const TStringBuf CAPTCHA_API_DOMAIN = "api";

    TString Quote(TString s) {
        ::Quote(s);
        return s;
    }

    TSpravka MakeSpravka(const TRequest& req, TSpravka::TDegradation degradation, TCaptchaSettingsPtr settings) {
        TStringBuf domain = CAPTCHA_API_DOMAIN;
        if (req.ClientType == CLIENT_CAPTCHA_API) {
            if (settings) {
                domain = TStringBuf(settings->Getclient_key());
            }
        } else {
            domain = GetCookiesDomainFromHost(req.Host);
        }
        return TSpravka::Generate(req.UserAddr, domain, degradation);
    }

    bool ParseSpravka(const TRequest& req, TSpravka& spravka, TStringBuf spravkaStr, TCaptchaSettingsPtr settings) {
        TStringBuf domain = settings ? TStringBuf(settings->Getclient_key()) : CAPTCHA_API_DOMAIN;
        return spravka.Parse(spravkaStr, domain)
            && req.ArrivalTime <= spravka.Time + ANTIROBOT_DAEMON_CONFIG.SpravkaApiExpireInterval;
    }

    void ClearCookie(TResponse& resp, const TStringBuf& cookieName, const TStringBuf& domain = TStringBuf()) {
        TStringStream valueStream;
        valueStream << cookieName << "=";
        AddCookieSuffix(valueStream, domain, TInstant::Now() - TDuration::Days(365));
        resp.AddHeader("Set-Cookie", valueStream.Str());
    }

    NThreading::TFuture<TResponse> CaptchaSuccessReply(const TRequestContext& rc, const TSpravka& spravka, bool withExternalRequests = false) {
        const TRequest& req = *rc.Req;
        try {
            TResponse resp = CaptchaFormat(req).CaptchaSuccessReply(req, spravka);

            if (req.ForceShowCaptcha && req.ClientType != CLIENT_XML_PARTNER) {
                ClearCookie(resp, BAN_COOKIE_NAME, GetCookiesDomainFromHost(req.Host));
            }

            resp.SetExternalRequestsFlag(withExternalRequests);
            return NThreading::MakeFuture(resp);
        } catch (const TReturnPath::TInvalidRetPathException& ex) {
            return TBadRequestHandler(HTTP_BAD_REQUEST)(rc, ex.what());
        }
    }

    ECaptchaRedirectType GetCaptchaRedirectType(const TRobotStatus& robotStatus) {
        if (robotStatus.IsCaptcha()) {
            return ECaptchaRedirectType::RegularCaptcha;
        }
        return ECaptchaRedirectType::NoRedirect;
    }

    void CheckCbbCaptchaRegexpFlagAndSetBanIfMatched(const TRequest& req, TRequestContext& rc,
                                                     TRobotStatus& robotStatus) {
        const bool canBeBanned = !req.UserAddr.IsWhitelisted() && req.MayBanFor() && req.Uid.Ns != TUid::SPRAVKA;
        if (!canBeBanned) {
            return;
        }

        const bool alreadyRobot = robotStatus.IsCaptcha();

        // баны по IP:
        // - CbbFlagIpBasedIdentificationsBan - для Uid = IP | IP6
        // - "cbb_farmable_ban_flag" - для всех Uid, кроме SPRAVKA и LCOOKIE
        if (const TString banReason = rc.Env.CbbBannedIpHolder.GetBanReason(req); !banReason.empty()) {
            if (!alreadyRobot) {
                rc.Env.CbbBannedIpHolder.UpdateStats(req);
                rc.BlockReason = banReason;
            }

            if (rc.Req->CanShowCaptcha()) {
                robotStatus.RegexpCaptcha = true;
            }
        }

        // баны по RegExp
        // - "cbb_captcha_re_flag"
        if (!rc.MatchedRules->Captcha.empty()) {
            if (!alreadyRobot) {
                rc.Env.CbbBannedIpHolder.Counters.Inc(req, TCbbBannedIpHolder::ECounter::ByRegexp);
                rc.BlockReason = CbbReason(rc.MatchedRules->Captcha);
            }

            if (req.CanShowCaptcha()) {
                robotStatus.RegexpCaptcha = true;
            }
        }
    }

    void CheckApiAutoVersionAndSetBan(const TRequest& req, TRequestContext& rc, TRobotStatus& robotStatus) {
        const bool alreadyRobot = robotStatus.IsCaptcha();
        if (alreadyRobot) {
            return;
        }

        const bool canBeBanned = !req.UserAddr.IsWhitelisted() && req.MayBanFor() && req.Uid.Ns != TUid::SPRAVKA;
        if (!canBeBanned) {
            return;
        }

        if (CheckApiAutoVersion(req)) {
            rc.BlockReason = "APIAUTO_VERSION";
            if (req.CanShowCaptcha()) {
                robotStatus.RegexpCaptcha = true;
                rc.ApiAutoVersionBan = true;
            }
        }
    }

    bool IsRobotAlready(TRequestContext& rc) {
        auto request = rc.Req.Get();
        return !rc.BlockReason.Empty() || request->InRobotSet;
    }

    float GetSuspiciousness(const TRequestContext& rc) {
        auto request = rc.Req;
        if (request->UserAddr.IsWhitelisted()) {
            return 0.0f;
        }
        return rc.MatchedRules->Suspiciousness ? 1.0f : 0.0f;
    }

    TEnv::EJwsCounter JwsStateToCounter(EJwsPlatform platform, EJwsState state) {
        switch (state) {
        case EJwsState::Invalid:
            return TEnv::EJwsCounter::Invalid;

        case EJwsState::Valid:
            switch (platform) {
            case EJwsPlatform::Unknown:
                return TEnv::EJwsCounter::UnknownValid;
            case EJwsPlatform::Android:
                return TEnv::EJwsCounter::AndroidValid;
            case EJwsPlatform::Ios:
                return TEnv::EJwsCounter::IosValid;
            }

        case EJwsState::ValidExpired:
            switch (platform) {
            case EJwsPlatform::Android:
                return TEnv::EJwsCounter::AndroidValidExpired;
            case EJwsPlatform::Ios:
            default: // EJwsPlatform::Unknown not applicable.
                return TEnv::EJwsCounter::IosValidExpired;
            }

        case EJwsState::Default:
            return TEnv::EJwsCounter::Default;

        case EJwsState::DefaultExpired:
            return TEnv::EJwsCounter::DefaultExpired;

        case EJwsState::Susp:
            switch (platform) {
            case EJwsPlatform::Android:
                return TEnv::EJwsCounter::AndroidSusp;
            case EJwsPlatform::Ios:
            default: // EJwsPlatform::Unknown not applicable.
                return TEnv::EJwsCounter::IosSusp;
            }

        case EJwsState::SuspExpired:
            switch (platform) {
            case EJwsPlatform::Android:
                return TEnv::EJwsCounter::AndroidSuspExpired;
            case EJwsPlatform::Ios:
            default: // EJwsPlatform::Unknown not applicable.
                return TEnv::EJwsCounter::IosSuspExpired;
            }

        case EJwsState::Count:
            Y_ENSURE(false, "EJwsState::Count passed to JwsStateToCounter");
        }
    }

    TEnv::EYandexTrustCounter YandexTrustStateToCounter(EYandexTrustState state) {
        switch (state) {
        case EYandexTrustState::Invalid:
            return TEnv::EYandexTrustCounter::Invalid;

        case EYandexTrustState::Valid:
            return TEnv::EYandexTrustCounter::Valid;

        case EYandexTrustState::ValidExpired:
            return TEnv::EYandexTrustCounter::ValidExpired;

        case EYandexTrustState::Count:
            Y_ENSURE(false, "EYandexTrustState::Count passed to YandexTrustStateToCounter");
        }
    }

    bool IsUnwantedChinaTraffic(const TRequest& req) {
        if (!EqualToOneOf(req.HostType, EHostType::HOST_WEB, EHostType::HOST_MORDA)) {
            return false;
        }

        const bool isLoginned = req.HasValidLCookie;
        if (isLoginned) {
            return false;
        }

        static THashSet<TStringBuf> kubrDomains = {"ru", "ua", "kz", "by", "uz"};
        const bool isKUBRTargetDomain = kubrDomains.contains(req.Tld);
        if (!isKUBRTargetDomain) {
            return false;
        }

        TString languageHeader = TString{req.Headers.Get("accept-language")};
        languageHeader.to_lower();
        const bool acceptsRussianLanguage = languageHeader.Contains("ru");
        if (acceptsRussianLanguage) {
            return false;
        }

        constexpr size_t chinaCountryCode = 134;
        const bool isRequestFromChina = req.UserAddr.CountryId() == chinaCountryCode;
        return isRequestFromChina;
    }

    EHostType GetHostTypeForProxiedUrl(TString& proxiedUrl, const TRequest& request) {
        auto url = TStringBuf(proxiedUrl);

        TStringBuf protocolHostAndLocation, queryParameters;
        if (!url.TrySplit('?', protocolHostAndLocation, queryParameters)) {
            return request.HostType;
        }

        TCgiParameters cgi;
        cgi.Scan(queryParameters);

        const auto serviceAsString = cgi.Get("service");
        if (serviceAsString.empty()) {
            return request.HostType;
        }

        cgi.EraseAll("service");
        if (!cgi.empty()) {
            proxiedUrl = TString::Join(protocolHostAndLocation, '?', cgi.Print());
        } else {
            proxiedUrl = protocolHostAndLocation;
        }

        EHostType service;
        if (TryFromString(serviceAsString, service)) {
            return service;
        }
        return request.HostType;
    }

    struct TProxiedUrlHandlerInfo {
        EHostType Service;
        TUid::ENameSpace Ns;
        EReqGroup ReqGroup;
    };

    template <typename SuccessfulParseHandler>
    NThreading::TFuture<TResponse> HandleProxiedUrl(TRequestContext& rc, SuccessfulParseHandler successHandler) {
        const TRequest& req = *rc.Req;

        enum EErrorStatus {
            CaptchaUrlUnparsed = ULL(1) << 32,
        };

        TTempBuf reqUnescapedBuf(req.RequestString.size() + 1);
        TStringBuf reqUnescaped = CgiUnescape(reqUnescapedBuf.Data(), req.RequestString);

        TString proxiedUrl;
        TStringBuf token;
        try {
            ParseProxiedCaptchaUrl(reqUnescaped, proxiedUrl, token, !req.UserAddr.IsYandex());

            TProxiedUrlHandlerInfo info;
            info.Service = GetHostTypeForProxiedUrl(proxiedUrl, *rc.Req);
            info.Ns = rc.Req->Uid.GetNameSpace();
            info.ReqGroup = rc.Env.ReqGroupClassifier.Group(info.Service, proxiedUrl, ""_sb);

            successHandler(token, info);
            return NThreading::MakeFuture(TResponse::Redirect(proxiedUrl));
        } catch (const TProxiedCaptchaUrlBad& ex) {
            NAntirobotEvClass::TCaptchaImageError event = {req.MakeLogHeader(),
                                                    ToString(token),
                                                    CaptchaUrlUnparsed,
                                                    Quote(ToString(req.RequestString.SubStr(0, 1000)))};
            NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(event);
            ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);

            req.LogRequestData(*ANTIROBOT_DAEMON_CONFIG.EventLog);
            return TBadRequestHandler(HTTP_BAD_REQUEST)(rc, ex.what());
        }
    }

    NAntirobotEvClass::TBanReasons GetBanReasons(const TRequestContext& rc) {
        NAntirobotEvClass::TBanReasons pbReasons;

        const auto reasons = rc.Env.Robots->GetBanReasons(rc.Req->HostType, rc.Req->Uid);
        pbReasons.SetMatrixnet(reasons.Matrixnet);
        pbReasons.SetYql(reasons.Yql);
        pbReasons.SetCbb(reasons.Cbb);

        return pbReasons;
    }
} // anonymous namespace

const TResponse NOT_A_ROBOT = TResponse::ToBalancer(HTTP_OK);

NThreading::TFuture<TResponse> HandleXmlSearch(TRequestContext& rc) {
    return rc.Req->IsPartnerRequest() ? HandleGeneralRequest(rc) : NThreading::MakeFuture(NOT_A_ROBOT);
}

/**
 * @param rc
 * @param again                Указывает, что капчу проходят повторно из-за неправильного ввода (нужно для отрисовки надписи об ошибке)
 *                             Например, ввели текст картики неправильно, нужно показать новую картинку с надписью "Неверно, попробуйте ещё раз"
 * @param fromCaptcha          Указывает, что редирект пришел с капчи. Нужно, чтобы узнать как формировать retPath
 *                             Например, прошли галочку или ввели текст с картинки - это fromCaptcha=true
 * @param token
 * @param withExternalRequests Флаг, указывающий, что при обработке были обращения к внешним сервисам (к капче или фурии, например)
 *                             Нужно, чтобы не учитывать их на графике времени ответа
 * @param settings             Настройки капчи
 */
NThreading::TFuture<TResponse> RedirectToCaptcha(const TRequestContext& rc, bool again, bool fromCaptcha, const TCaptchaToken& token, bool withExternalRequests, TCaptchaSettingsPtr settings) {
    const TRequest& req = *rc.Req;
    if (again || ANTIROBOT_DAEMON_CONFIG.WorkMode == WORK_ENABLED) {
        TCaptchaStat& captchaStat = *rc.Env.CaptchaStat;
        if (req.ClientType == CLIENT_GENERAL) {
            try {
                TReturnPath retPath = fromCaptcha ? TReturnPath::FromCgi(req.CgiParams)
                    : TReturnPath::FromRequest(req);

                bool noSpravka = !req.HasSpravka();
                TResponse resp = TResponse::Redirect(CalcCaptchaPageUrl(req, retPath, again, token, noSpravka));

                if (noSpravka) {
                    const TStringBuf domain = GetCookiesDomainFromHost(req.Host);
                    TSpravka::TDegradation degradation;
                    TSpravka expiredSpravka = TSpravka::Generate(req.UserAddr, TInstant::Now() - TDuration::Days(365), domain, degradation);
                    resp.AddHeader("Set-Cookie", expiredSpravka.AsCookie());
                }

                const auto banReasons = GetBanReasons(rc);

                captchaStat.UpdateOnRedirect(req, token, again, banReasons);

                if (req.IsPartnerRequest()) {
                    rc.Env.PartnersCaptchaStat.Inc(EPartnersCaptchaCounter::Redirects);
                }

                NAntirobotEvClass::TCaptchaRedirect event = {
                    /* THeader */      req.MakeLogHeader(),
                    /* Again */        again,
                    /* IsXmlPartner */ false,
                    /* IsRandom */     false,
                    /* Token */        token.AsString,
                    /* ClientType */   req.ClientType,
                    /* CaptchaType */  TString(),
                    /* BanReasons */   banReasons,
                    /* HostType */     req.HostType,
                    /* Key */          TString()
                };
                NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(event);
                ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);

                resp.SetExternalRequestsFlag(withExternalRequests);
                return NThreading::MakeFuture(resp);
            } catch (const TReturnPath::TInvalidRetPathException& ex) {
                return TBadRequestHandler(HTTP_BAD_REQUEST)(rc, ex.what());
            }
        } else {
            try {
                bool nonBrandedPartner = false;
                if (req.ClientType == CLIENT_XML_PARTNER) {
                    if (!ANTIROBOT_DAEMON_CONFIG.ConfByTld(req.Tld).PartnerCaptchaType) {
                        auto response = NOT_A_ROBOT;
                        response.SetExternalRequestsFlag(withExternalRequests);
                        return NThreading::MakeFuture(response);
                    }

                    nonBrandedPartner = rc.Env.NonBrandedPartners.UserInList(req.CgiParams.Get(TStringBuf("user")));
                }

                auto captchaFuture = MakeCaptchaAsync(token, rc.Req, captchaStat, settings, nonBrandedPartner);
                TReturnPath::FromRequest(req); // throws TInvalidRetPathException, need to check valid retpath to prevent throwing in GenCaptchaResponse

                return captchaFuture.Apply([&rc, again, fromCaptcha, token](const auto& future) {
                    const TRequest& req = *rc.Req;
                    TCaptchaDescriptor captcha;
                    if (TError err = future.GetValue().PutValueTo(captcha); err.Defined()) {
                        rc.Env.CaptchaStat->IncErrorCount();
                        rc.Env.RD->GetTrainSetGenerator(req.Tld).ProcessCaptchaGenerationError(rc);

                        if (fromCaptcha) {
                            TSpravka::TDegradation degradation;
                            const TSpravka spravka = MakeSpravka(req, degradation, TCaptchaSettingsPtr{});
                            // TODO: почему CurrentExceptionMessage?
                            EVLOG_MSG << req << "Captcha generate error (" <<  CurrentExceptionMessage() << ") spravka granted: " << spravka.AsCookie();
                            return CaptchaSuccessReply(rc, spravka, /* withExternalRequests := */ true);
                        } else {
                            EVLOG_MSG << req << "Captcha generate error (" <<  CurrentExceptionMessage() << ")";
                            auto response = NOT_A_ROBOT;
                            response.SetExternalRequestsFlag(true);
                            return NThreading::MakeFuture(response);
                        }
                    }

                    const auto banReasons = GetBanReasons(rc);

                    rc.Env.CaptchaStat->UpdateOnRedirect(req, token, again, banReasons);

                    if (req.IsPartnerRequest()) {
                        rc.Env.PartnersCaptchaStat.Inc(EPartnersCaptchaCounter::Redirects);
                    }

                    TCaptchaPageParams params(rc, captcha, again);

                    NAntirobotEvClass::TCaptchaRedirect event = {
                        /* Header */       req.MakeLogHeader(),
                        /* Again */        again,
                        /* IsXmlPartner */ req.ClientType == CLIENT_XML_PARTNER,
                        /* IsRandom */     false,
                        /* Token */        token.AsString,
                        /* ClientType */   req.ClientType,
                        /* CaptchaType */  captcha.CaptchaType,
                        /* BanReasons */   banReasons,
                        /* HostType */     req.HostType,
                        /* Key */          captcha.Key.ToString()
                    };
                    NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(event);
                    ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);

                    auto response = CaptchaFormat(req).GenCaptchaResponse(params);
                    response.SetExternalRequestsFlag(true);
                    return NThreading::MakeFuture(response);
                });
            } catch (TFullReqInfo::TBadRequest&) {
                throw;
            } catch (const TReturnPath::TInvalidRetPathException& ex) {
                return TBadRequestHandler(HTTP_BAD_REQUEST)(rc, ex.what());
            } catch(...) {
                rc.Env.CaptchaStat->IncErrorCount();
                rc.Env.RD->GetTrainSetGenerator(req.Tld).ProcessCaptchaGenerationError(rc);

                if (fromCaptcha) {
                    TSpravka::TDegradation degradation;
                    const TSpravka spravka = MakeSpravka(*rc.Req, degradation, TCaptchaSettingsPtr{});
                    EVLOG_MSG << req << "Captcha generate error (" <<  CurrentExceptionMessage() << ") spravka granted: " << spravka.AsCookie();
                    return CaptchaSuccessReply(rc, spravka, /* withExternalRequests := */ true);
                } else {
                    EVLOG_MSG << req << "Captcha generate error (" <<  CurrentExceptionMessage() << ")";
                    auto response = NOT_A_ROBOT;
                    response.SetExternalRequestsFlag(true);
                    return NThreading::MakeFuture(response);
                }
            }
        }
    } else {
        auto response = NOT_A_ROBOT;
        response.SetExternalRequestsFlag(withExternalRequests);
        return NThreading::MakeFuture(response);
    }
}

NThreading::TFuture<TResponse> HandleShowCaptcha(TRequestContext& rc) {
    const TRequest& req = *rc.Req;

    try {
        CheckCaptchaPageRequestSigned(req);

        TCaptchaToken token = TCaptchaToken::Parse(req.CgiParams.Get("t"));

        if (token.Timestamp + ANTIROBOT_DAEMON_CONFIG.CaptchaRedirectTimeOut < TInstant::Now()
            && !req.UserAddr.IsYandex())
        {
            NAntirobotEvClass::TCaptchaTokenExpired event {req.MakeLogHeader(), token.AsString};
            NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(event);
            ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);

            return NThreading::MakeFuture(TResponse::Redirect(TReturnPath::FromCgi(req.CgiParams).GetURL()));
        }

        bool again = req.CgiParams.Has("status", TStringBuf("failed"));
        const TCheckCookiesResult ccr(req);

        TString returnPath = req.CgiParams.Get(TReturnPath::CGI_PARAM_NAME);

        auto captchaFuture = MakeCaptchaAsync(token, rc.Req, *rc.Env.CaptchaStat, TCaptchaSettingsPtr{}, false);

        return captchaFuture.Apply([&rc, token, again, returnPath, ccr](const auto& future) {
            const TRequest& req = *rc.Req;

            TCaptchaDescriptor visibleCaptcha;

            if (TError err = future.GetValue().PutValueTo(visibleCaptcha); err.Defined()) {
                rc.Env.CaptchaStat->IncErrorCount();
                rc.Env.RD->GetTrainSetGenerator(rc.Req->Tld).ProcessCaptchaGenerationError(rc);

                // put spravka as gift instead 504 - error page
                TSpravka::TDegradation degradation;
                const TSpravka spravka = MakeSpravka(*rc.Req, degradation, TCaptchaSettingsPtr{});
                EVLOG_MSG << req << "Captcha generate error (" <<  CurrentExceptionMessage() << ") spravka granted: "
                          << spravka.AsCookie();

                return CaptchaSuccessReply(rc, spravka, /* withExternalRequests := */ true);
            }

            /*
             * We don't have to check the cgi-parameter "retpath" for correctness here.
             * The entire URL of /showcaptcha request is signed with TKeyRing
             * (see CalcCaptchaPageUrl())and this signature is checked
             * in HandleShowCaptcha(). Thus if we get to this method the /showcaptcha
             * URL is correct and therefore the "retpath" is also well-formed.
             */
            TCaptchaPageParams params(rc, visibleCaptcha, again);
            params.ReturnPath = returnPath;
            params.FormActionPath = TStringBuf("/checkcaptcha");
            params.CookiesEnabled = ccr.CookiesEnabled;

            NAntirobotEvClass::TCaptchaShow event = {
                /* Header */            req.MakeLogHeader(),
                /* Again */             again,
                /* IsXmlPartner */      false,
                /* Key */               visibleCaptcha.Key.ToString(),
                /* Token */             token.AsString,
                /* TestCookieSet */     ccr.TestCookieSet,
                /* TestCookieSuccess */ ccr.TestCookieSuccess,
                /* ClientType */        CLIENT_GENERAL,
                /* CaptchaType */       visibleCaptcha.CaptchaType,
                /* BanReasons */        GetBanReasons(rc),
                /* HostType */          req.HostType
            };
            NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(event);
            ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);

            if (token.CaptchaType == ECaptchaType::SmartCheckbox) {
                ++rc.Env.CaptchaStat->ServiceNsCounters.Get(req, TCaptchaStat::EServiceNsCounter::CheckboxShows);
            } else {
                ++rc.Env.CaptchaStat->ServiceNsCounters.Get(req, TCaptchaStat::EServiceNsCounter::AdvancedShows);
            }

            if (req.IsPartnerRequest()) {
                rc.Env.PartnersCaptchaStat.Inc(EPartnersCaptchaCounter::Shows);
            }

            auto response = CaptchaFormat(req).GenCaptchaResponse(params);
            response.SetExternalRequestsFlag(true);
            return NThreading::MakeFuture(response);
        });
    } catch (const TCaptchaPageRequestBad& ex) {
        return TBadRequestHandler(HTTP_BAD_REQUEST)(rc, ex.what());
    } catch (const TReturnPath::TInvalidRetPathException& ex) {
        return TBadRequestHandler(HTTP_BAD_REQUEST)(rc, ex.what());
    } catch (...) {
        rc.Env.CaptchaStat->IncErrorCount();
        rc.Env.RD->GetTrainSetGenerator(rc.Req->Tld).ProcessCaptchaGenerationError(rc);

        // put spravka as gift instead 504 - error page
        TSpravka::TDegradation degradation;
        const TSpravka spravka = MakeSpravka(*rc.Req, degradation, TCaptchaSettingsPtr{});
        EVLOG_MSG << req << "Captcha generate error (" <<  CurrentExceptionMessage() << ") spravka granted: "
                  << spravka.AsCookie();

        return CaptchaSuccessReply(rc, spravka, /* withExternalRequests := */ true);
    }
}

NThreading::TFuture<TResponse> HandleMessengerRequest(TRequestContext&) {
    return NThreading::MakeFuture(TResponse::ToUser(HTTP_NO_CONTENT));
}

TAtomicSharedPtr<TFullReqInfo> MakeTestFullReq(const TRequestContext& rc) {
    TString host = rc.Req->CgiParams.Get("host"_sb);
    if (host.empty()) {
        host = "yandex.ru";
    }
    TString service = rc.Req->CgiParams.Get("service"_sb);
    if (service.empty()) {
        service = ToString(HostToHostType(host));
    }
    TString request = "GET /yandsearch?text=123 HTTP/1.1\r\n"
                     "Host: " + host + "\r\n"
                     "Cookie: " + rc.Req->Headers.Get("Cookie") + "\r\n"
                     "X-Forwarded-For-Y: 127.0.0.1\r\n"
                     "X-Req-Id: 1412093001153547-107617636651740\r\n"
                     "X-Antirobot-Service-Y: " + service + "\r\n"
                     "\r\n";
    TStringInput si(request);
    THttpInput hi(&si);
    return MakeAtomicShared<TFullReqInfo>(
        hi,
        "",
        "",
        rc.Env.ReloadableData,
        TPanicFlags::CreateFake(),
        GetEmptySpravkaIgnorePredicate(),
        &rc.Env.ReqGroupClassifier,
        &rc.Env
    );
}

NThreading::TFuture<TResponse> TestCaptchaPage(TRequestContext& rc) {
    if (!rc.Req->UserAddr.IsYandex() && !ANTIROBOT_DAEMON_CONFIG.Local) {
        return TBadRequestHandler(HTTP_BAD_REQUEST)(rc, "/captchapage request not from Yandex");
    }

    ECaptchaType captchaType = ECaptchaType::SmartCheckbox;

    auto req = MakeTestFullReq(rc);

    const TCaptchaToken token(captchaType, req->UniqueKey);
    bool again = rc.Req->CgiParams.Has(TStringBuf("again"), TStringBuf("true"));
    auto future = MakeCaptchaAsync(token, rc.Req, *rc.Env.CaptchaStat, TCaptchaSettingsPtr{});
    TCaptchaDescriptor captcha;
    if (TError err = future.GetValueSync().PutValueTo(captcha); err.Defined()) {
        err.Throw();
    }
    TCaptchaPageParams params(rc, captcha, again);
    params.ReturnPath = TReturnPath::FromRequest(*req).GetURL();
    params.FormActionPath = TStringBuf("/checkcaptcha");
    params.CookiesEnabled = rc.Req->CgiParams.Has(TStringBuf("cookies"), TStringBuf("enabled"));

    rc.Req.Reset(req);

    return NThreading::MakeFuture(CaptchaFormat(*rc.Req).GenCaptchaResponse(params));
}

NThreading::TFuture<TResponse> TestBlockedPage(TRequestContext& rc) {
    if (!rc.Req->UserAddr.IsYandex() && !ANTIROBOT_DAEMON_CONFIG.Local) {
        return TBadRequestHandler(HTTP_BAD_REQUEST)(rc, "/blockedpage request not from Yandex");
    }

    rc.Req.Reset(MakeTestFullReq(rc));
    return TBlockedHandler()(rc);
}

NThreading::TFuture<TResponse> TestManyRequestsPage(TRequestContext& rc) {
    if (!rc.Req->UserAddr.IsYandex() && !ANTIROBOT_DAEMON_CONFIG.Local) {
        return TBadRequestHandler(HTTP_BAD_REQUEST)(rc, "/manyrequestspage request not from Yandex");
    }

    rc.Req.Reset(MakeTestFullReq(rc));
    return TManyRequestsHandler()(rc);
}

NThreading::TFuture<TResponse> HandleCaptchaPgrd(TRequestContext& rc) {
    if (rc.Env.HypocrisyBundle.Instances.empty()) {
        return NThreading::MakeFuture(TResponse::ToUser(HTTP_NOT_FOUND));
    }

    auto response = TResponse::ToUser(HTTP_OK)
        .DisableCompressionHeader()
        .AddHeader("Cache-Control", Sprintf(
            "public, max-age=%zu, immutable",
            ANTIROBOT_DAEMON_CONFIG.HypocrisyCacheControlMaxAge
        ))
        .AddHeader("Access-Control-Allow-Origin", "*");

    const TVector<TString>* instances = nullptr;

    if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
        // для капчи включен модуль compression на балансере
        instances = &rc.Env.HypocrisyBundle.Instances;
    } else if (rc.Req->SupportsBrotli) {
        instances = &rc.Env.BrotliHypocrisyInstances;
        response.AddHeader("Content-Encoding", "br");
    } else if (rc.Req->SupportsGzip) {
        instances = &rc.Env.GzipHypocrisyInstances;
        response.AddHeader("Content-Encoding", "gzip");
    } else {
        instances = &rc.Env.HypocrisyBundle.Instances;
    }

    const auto& instance = (*instances)[RandomNumber(instances->size())];
    return NThreading::MakeFuture(response.SetContent(instance, "text/javascript"));
}

NThreading::TFuture<TResponse> HandleShowCaptchaImage(TRequestContext& rc) {
    return HandleProxiedUrl(rc, [&rc](
        TStringBuf token, const TProxiedUrlHandlerInfo& info
    ) {
        auto ev = NAntirobotEvClass::TCaptchaImageShow(rc.Req->MakeLogHeader(), ToString(token));
        NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(ev);
        ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);

        rc.Env.CaptchaStat->ServiceNsExpBinCounters.Inc(
            info.Service, UidNsToSimpleUidType(info.Ns), rc.Req->ExperimentBin(),
            TCaptchaStat::EServiceNsExpBinCounter::ImageShows
        );

        if (rc.Req->IsPartnerRequest()) {
            rc.Env.PartnersCaptchaStat.Inc(EPartnersCaptchaCounter::ImageShows);
        }
    });
}

NThreading::TFuture<TResponse> HandleVoice(TRequestContext& rc) {
    return HandleProxiedUrl(rc, [&rc](
        TStringBuf token, const TProxiedUrlHandlerInfo& info
    ) {
        auto ev = NAntirobotEvClass::TCaptchaVoice(rc.Req->MakeLogHeader(), ToString(token));
        NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(ev);
        ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);

        rc.Env.CaptchaStat->ServiceCounters.Inc(info.Service, TCaptchaStat::EServiceCounter::VoiceDownloads);
    });
}

NThreading::TFuture<TResponse> HandleVoiceIntro(TRequestContext& rc) {
    return HandleProxiedUrl(rc, [&rc](
        TStringBuf token, const TProxiedUrlHandlerInfo& info
    ) {
        Y_UNUSED(info);

        auto ev = NAntirobotEvClass::TCaptchaVoiceIntro(rc.Req->MakeLogHeader(), ToString(token));
        NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(ev);
        ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);
    });
}

namespace {
    void LogVerochkaRequest(TRequestContext& rc) {
        const TRequest& request = *rc.Req;
        const TCaptchaRequest& captchaReq = request.CaptchaRequest;

        if (!captchaReq.BrowserJsPrint.HasData || !captchaReq.BrowserJsPrint.ValidJson) {
            ++rc.Env.VerochkaStats.InvalidVerochkaRequests;
        }

        NAntirobotEvClass::TVerochkaRecord logRecord;
        logRecord.MutableHeader()->CopyFrom(request.MakeLogHeader());
        logRecord.SetIsValid(captchaReq.BrowserJsPrint.ValidJson);
        logRecord.SetRequestContent(request.ContentData);
        logRecord.SetRequestString(request.RequestString.Data(), request.RequestString.Size());

        TString verochkaJsonString;
        TStringOutput so(verochkaJsonString);
        NJsonWriter::TBuf verochkaJson(NJsonWriter::HEM_DONT_ESCAPE_HTML, &so);
        captchaReq.BrowserJsPrint.WriteToJson(verochkaJson);

        logRecord.SetJsPrint(verochkaJsonString);

        if (const auto ja3 = request.Ja3(); !ja3.empty()) {
            logRecord.SetJa3(ja3.Data(), ja3.Size());
        }
        if (const auto referer = request.Referer(); !referer.empty()) {
            logRecord.SetReferer(referer.Data(), referer.Size());
        }

        NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(logRecord);
        ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);
    }
} // namespace

NThreading::TFuture<TResponse> HandleGeneralRequest(TRequestContext& rc) {
    const TRequest& req = *rc.Req;

    rc.FirstArrivalTime = req.ArrivalTime;

    rc.Suspiciousness = GetSuspiciousness(rc);
    if (rc.Suspiciousness > 0 && AtomicGet(rc.Env.SuspiciousFlags.Block.GetByService(req.HostType))) {
        rc.WasBlocked = true;
        rc.BlockReason = "SUSPICIOUS";
    }

    TRobotStatus robotStatus = ANTIROBOT_DAEMON_CONFIG.WorkMode == WORK_DISABLED ? TRobotStatus::Normal() : rc.Env.RD->GetRobotStatus(rc);

    CheckCbbCaptchaRegexpFlagAndSetBanIfMatched(req, rc, robotStatus);

    CheckApiAutoVersionAndSetBan(req, rc, robotStatus);

    if (!robotStatus.RegexpCaptcha && !rc.Env.DisablingFlags.IsDisableCatboostWhitelist(req.HostType)) {
        std::tie(rc.CatboostWhitelist, rc.CacherFormulaResult) = rc.Env.CRD->Process(req, rc, robotStatus);
    }

    if (
        robotStatus.IsCaptcha() &&
        (
            rc.Env.DisablingFlags.IsNeverBanForService(req.HostType) ||
            NotBanRequest(rc)
        )
    ) {
        rc.Env.DisablingStat.AddNotBannedRequest();
        robotStatus.Reset();
    }

    rc.RedirectType = GetCaptchaRedirectType(robotStatus);
    rc.RobotnessHeaderValue = IsRobotAlready(rc) || rc.MatchedRules->MaxRobotness ? 1.0f : 0.0f;

    if (!CutRequestForwarding(rc)) {
        rc.Env.ForwardRequest(rc);
    }

    if (rc.MatchedRules->Mark) {
        rc.Env.BlockResponsesStats->UpdateMarkedBanned(rc);
    }

    if (req.HasJws) {
        rc.Env.JwsCounters.Inc(req, JwsStateToCounter(req.JwsPlatform, req.JwsState));
    }

    if (req.HasYandexTrust) {
        ++rc.Env.YandexTrustCounters.Get(req, YandexTrustStateToCounter(req.YandexTrustState));
    }

    if (rc.Degradation) {
        rc.Env.ServiceNsExpBinCounters.Inc(req, TEnv::EServiceNsExpBinCounter::WithDegradationRequests);
        rc.Env.ServiceNsGroupCounters.Inc(req, TEnv::EServiceNsGroupCounter::WithDegradationRequests);
    }

    if (rc.Suspiciousness > 0) {
        rc.Env.ServiceNsExpBinCounters.Inc(req, TEnv::EServiceNsExpBinCounter::WithSuspiciousnessRequests);
        rc.Env.ServiceNsGroupCounters.Inc(req, TEnv::EServiceNsGroupCounter::WithSuspiciousnessRequests);

        if (rc.Req->TrustedUser) {
            rc.Env.ServiceNsGroupCounters.Inc(req, TEnv::EServiceNsGroupCounter::WithSuspiciousnessTrustedUsersRequests);
        }

        const bool enableManyRequests = AtomicGet(rc.Env.SuspiciousFlags.ManyRequests.GetByService(req.HostType)) && !req.Host.starts_with(MOBILE_PREFIX);
        const bool enableManyRequestsMobile = AtomicGet(rc.Env.SuspiciousFlags.ManyRequestsMobile.GetByService(req.HostType)) && req.Host.starts_with(MOBILE_PREFIX);
        if (enableManyRequests || enableManyRequestsMobile) {
            rc.Env.ManyRequestsStats.Update(req);
            return TManyRequestsHandler()(rc);
        }

        if (AtomicGet(rc.Env.SuspiciousFlags.Block.GetByService(req.HostType))) {
            rc.Env.BlockResponsesStats->Update(rc);
            return TBlockedHandler()(rc);
        }
    }

    if (robotStatus.IsCaptcha()) {
        // TODO: мы не можем сразу перевести всех AJAX на SmartCheckbox, потому как некоторые из них спользуют картинку в JSON
        // только опредленному списку старых сервисов отдаём картинку
        // постепенно нужно перевести все: https://st.yandex-team.ru/CAPTCHA-2910
        ECaptchaType captchaType = req.ClientType == CLIENT_AJAX && IsIn(DEPRECATED_AJAX_HOST_TYPES, req.HostType) || req.ClientType == CLIENT_XML_PARTNER
            ? ECaptchaType::SmartAdvanced
            : ECaptchaType::SmartCheckbox;
        TCaptchaToken token(captchaType, req.UniqueKey);
        return RedirectToCaptcha(rc, /* again := */ false, /* fromCaptcha := */ false, token);
    }

    if (ANTIROBOT_DAEMON_CONFIG.InitialChinaRedirectEnabled && IsUnwantedChinaTraffic(req)) {
        AtomicIncrement(rc.Env.ChinaRedirectStats.RequestsFromChinaNotLoginned);
        if (AtomicGet(rc.Env.IsChinaRedirectEnabled)) {
            AtomicIncrement(rc.Env.ChinaRedirectStats.RedirectedToLoginRequestsFromChina);
            if (AtomicGet(rc.Env.IsChinaUnauthorizedEnabled) && req.Doc.StartsWith("/portal/ntp/notifications")) {
                AtomicIncrement(rc.Env.ChinaRedirectStats.UnauthorizedRequestsFromChina);
                return NThreading::MakeFuture(TResponse::ToUser(HTTP_UNAUTHORIZED));
            }
            auto response = TResponse::ToUser(HTTP_FOUND);
            TString encodedRequest = TString::Join(ToString(req.Scheme), req.HostWithPort, req.RequestString);
            static const char* safeCharacters = "";
            const TString domain = TString{req.Tld};
            Quote(encodedRequest, safeCharacters);
            response.AddHeader("Location", Sprintf(ANTIROBOT_DAEMON_CONFIG.ChinaUrlNotLoginnedRedirect.c_str(), domain.c_str(), encodedRequest.c_str()));
            return NThreading::MakeFuture(response);
        }
    }

    rc.Env.ServiceNsExpBinCounters.Inc(req, TEnv::EServiceNsExpBinCounter::RequestsPassedToService);
    rc.Env.ServiceNsGroupCounters.Inc(req, TEnv::EServiceNsGroupCounter::RequestsPassedToService);
    return NThreading::MakeFuture(NOT_A_ROBOT);
}

NThreading::TFuture<TResponse> HandleCheckCaptchaApi(TRequestContext& rc) {
    const TRequest& req = *rc.Req;
    if (req.RequestMethod != "get"sv && req.RequestMethod != "post"sv) {
        return NThreading::MakeFuture(TResponse::ToUser(HTTP_METHOD_NOT_ALLOWED).AddHeader("Allow", "GET, POST"));
    }

    struct TCheckSiteKeyResult {
        bool Status = false;
        TCaptchaSettingsPtr Settings{};
    };

    auto checkSiteKey = [&rc]() -> NThreading::TFuture<TCheckSiteKeyResult> {
        const TRequest& req = *rc.Req;
        if (const auto& siteKey = req.CaptchaRequest.SiteKey; siteKey) {
            return rc.Env.CloudApiClient->GetSettingsByClientKey(siteKey).Apply([&rc, &siteKey] (const auto& future) -> NThreading::TFuture<TCheckSiteKeyResult> {
                const TRequest& req = *rc.Req;
                TMaybeCaptchaSettings settings;
                if (TError err = future.GetValue().PutValueTo(settings); err.Defined()) {
                    // в случае ошибки пользователи не должны страдать
                    // доверяем sitekey, который нам передали, чтобы выдать справку на него
                    EVLOG_MSG << EVLOG_ERROR << req << "GetSettingsByClientKey error: " << err->what();
                    rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::CheckApiFailedRequests);
                    return NThreading::MakeFuture(TCheckSiteKeyResult{.Status = true, .Settings = TCaptchaSettingsPtr::CreateWithSiteKey(siteKey)});
                }
                if (!settings) {
                    rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::CheckInvalidSitekeyRequests);
                    return NThreading::MakeFuture(TCheckSiteKeyResult{});
                }
                rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::CheckValidSitekeyRequests);
                return NThreading::MakeFuture(TCheckSiteKeyResult{true, TCaptchaSettingsPtr(settings)});
            });
        } else {
            // есть клиенты без sitekey, подключенные через лего
            // TODO: нужно как-то закрыть доступ для остальных клиентов, чтобы не абьюзили
            rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::CheckUnauthorizedRequests);
            return NThreading::MakeFuture(TCheckSiteKeyResult{.Status = true});
        }
    };

    return checkSiteKey().Apply([&rc] (const auto& future) {
        const TCheckSiteKeyResult res = future.GetValue();
        if (!res.Status) {
            return NThreading::MakeFuture(TResponse::ToUser(HTTP_FORBIDDEN));
        }
        return HandleCheckCaptchaCommon(rc, std::move(res.Settings));
    });
}

NThreading::TFuture<TResponse> HandleCheckCaptcha(TRequestContext& rc) {
  return HandleCheckCaptchaCommon(rc, TCaptchaSettingsPtr{});
}

NThreading::TFuture<TResponse> HandleCheckCaptchaCommon(TRequestContext& rc, TCaptchaSettingsPtr settings) {
    const TRequest& req = *rc.Req;
    if (req.RequestMethod != "get"sv && req.RequestMethod != "post"sv) {
        return NThreading::MakeFuture(TResponse::ToUser(HTTP_METHOD_NOT_ALLOWED).AddHeader("Allow", "GET, POST"));
    }

    try {
        LogVerochkaRequest(rc);

        if (req.Doc == "/tmgrdfrendc") {
            return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK).SetContent("{}"));
        }

        const TString& keyStr = req.CaptchaRequest.Key;
        const bool isApiFirstRequest = !keyStr && req.ClientType == CLIENT_CAPTCHA_API;

        TCaptchaKey key;
        if (isApiFirstRequest) {
            key.Token = TCaptchaToken(ECaptchaType::SmartKeys, req.UniqueKey);
        } else {
            key = TCaptchaKey::Parse(keyStr, req.Host);
        }

        return IsCaptchaGoodAsync(key, rc, ANTIROBOT_DAEMON_CONFIG, settings).Apply([&rc](const auto& future) -> TCaptchaCheckResult {
            try {
                auto result = future.GetValue();
                for (const auto& errMsg : result.ErrorMessages) {
                    EVLOG_MSG << "Captcha check error (" << errMsg << ")";
                }

                if (result.WasApiCaptchaError) {
                    rc.Env.Counters.Inc(TEnv::ECounter::CaptchaCheckErrors);
                }
                if (!result.WarningMessages.empty()) {
                    rc.Env.Counters.Inc(TEnv::ECounter::CaptchaCheckBadRequests);
                }
                if (result.WasFuryError) {
                    rc.Env.Counters.Inc(TEnv::ECounter::FuryCheckErrors);
                }
                if (result.WasFuryPreprodError) {
                    rc.Env.Counters.Inc(TEnv::ECounter::FuryPreprodCheckErrors);
                }

                return result;
            } catch (...) {
                EVLOG_MSG << EVLOG_ERROR << "DEBUG_CAPTCHA-1464 CheckCaptchaAsync " << CurrentExceptionMessage();
                Cerr << "DEBUG_CAPTCHA-1464 CheckCaptchaAsync " << CurrentExceptionMessage() << Endl;
                return TCaptchaCheckResult();
            }
        }).Apply([key, &rc, keyStr, settings](const auto& future) {
            const auto& req = *rc.Req;
            const auto& captchaCheckResult = future.GetValue();
            rc.Env.CaptchaStat->UpdateOnInput(key, rc, captchaCheckResult, settings);

            if (req.IsPartnerRequest()) {
                rc.Env.PartnersCaptchaStat.Inc(
                    captchaCheckResult.Success ?
                        EPartnersCaptchaCounter::CorrectInputs :
                        EPartnersCaptchaCounter::IncorrectInputs
                );
            }

            rc.Env.ForwardCaptchaInput(rc, captchaCheckResult.Success);

            req.LogRequestData(*ANTIROBOT_DAEMON_CONFIG.EventLog);

            auto LogCaptchaCheckEvent = [&](const TString& spravkaStr) {
                NAntirobotEvClass::TCaptchaCheck event = {
                    /* Header */                  req.MakeLogHeader(),
                    /* Key */                     keyStr,
                    /* Token */                   key.Token.AsString,
                    /* Success */                 captchaCheckResult.Success,
                    /* NewSpravka */              spravkaStr.data(), spravkaStr.size(),
                    /* EnteredHiddenImage */      false,
                    /* CaptchaConnectError */     captchaCheckResult.WasApiCaptchaError,
                    /* PreprodSuccess */          captchaCheckResult.PreprodSuccess,
                    /* FuryConnectError */        captchaCheckResult.WasFuryError,
                    /* FuryPreprodConnectError */ captchaCheckResult.WasFuryPreprodError,
                    /* WarningMessages */         JoinStrings(captchaCheckResult.WarningMessages, "\n"),
                    /* SiteKey */                 settings ? settings->Getclient_key() : "",
                    /* ClientType */              req.ClientType
                };
                NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(event);
                ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);
            };


            if (
                captchaCheckResult.Success ||
                rc.Env.DisablingFlags.IsNeverBanForService(req.HostType) // TODO: check is API
            ) {
                if (!captchaCheckResult.Success) {
                    rc.Env.DisablingStat.AddNotBannedRequest();
                }
                const TSpravka spravka = MakeSpravka(*rc.Req, captchaCheckResult.Degradation, settings);
                LogCaptchaCheckEvent(spravka.ToString());
                return CaptchaSuccessReply(rc, spravka, /* withExternalRequests := */ true);
            } else {
                LogCaptchaCheckEvent("-");
                TCaptchaToken token = key.Token;

                bool again = true;
                if (token.CaptchaType == ECaptchaType::SmartCheckbox) {
                    token = token.WithType(ECaptchaType::SmartAdvanced);
                    again = false;
                } else if (token.CaptchaType == ECaptchaType::SmartKeys) {
                    token = token.WithType(ECaptchaType::SmartCheckbox);
                    again = false;
                }
                return RedirectToCaptcha(rc, /* again := */ again, /* fromCaptcha := */ true, token, /* withExternalRequests := */ true, settings);
            }
        });
    } catch (const TCaptchaKey::TParseError&) {
        EVLOG_MSG << req << "Captcha check error: " << CurrentExceptionMessage();
        return NThreading::MakeFuture(TResponse::ToUser(HTTP_BAD_REQUEST));
    }
}

enum class ECanValidateStatus {
    Allow,
    Deny,
    ForcePassed,
};

struct TCanValidateResponse {
    ECanValidateStatus Status;
    TString Message;
    TCaptchaSettingsPtr Settings = {};
};

struct TCaptchaValidateResponse {
    bool Passed;
    HttpCodes Code;
    TString Message;
    TCaptchaSettingsPtr Settings = {};
};

enum class ECaptchaIframeStatus {
    Allowed,
    InvalidKey,
    WrongDomain,
};

struct TCaptchaIframeResult {
    ECaptchaIframeStatus Status;
    TCaptchaSettingsPtr Settings = {};
};

void ProcessBillingEvent(TRequestContext& rc, const TCaptchaSettingsPtr& settings) {
    if (!settings) {
        return;
    }
    ui64 timestamp = TInstant::Now().Seconds();

    TString logString;
    TStringOutput so(logString);
    NJsonWriter::TBuf json(NJsonWriter::HEM_DONT_ESCAPE_HTML, &so);
    TString id = ToString(rc.Req->ReqId);
    if (!id) {
        id = CreateGuidAsString();
    }
    json.BeginObject();
    {
        json.WriteKey("folder_id").WriteString(settings->Getfolder_id());
        json.WriteKey("resource_id").WriteString(settings->Getcaptcha_id());
        json.WriteKey("id").WriteString(id);
        json.WriteKey("version").WriteString("v1");
        json.WriteKey("source_id").WriteString(ShortHostName());
        json.WriteKey("source_wt").WriteULongLong(timestamp);
        json.WriteKey("tags").BeginObject().EndObject();
        json.WriteKey("usage").BeginObject();
        {
            json.WriteKey("quantity").WriteString("1");
            json.WriteKey("start").WriteULongLong(timestamp);
            json.WriteKey("finish").WriteULongLong(timestamp);
            json.WriteKey("unit").WriteString("unit");
            json.WriteKey("type").WriteString("delta");
        }
        json.EndObject();
        json.WriteKey("schema").WriteString("smart_captcha.check.requests.v1");
    }
    json.EndObject();
    rc.Env.BillingLogJson << logString << Endl;
}

NThreading::TFuture<TResponse> HandleCaptchaValidate(TRequestContext& rc) {
    const TRequest& req = *rc.Req;

    auto getValidateStatus = [&req, &rc]() -> NThreading::TFuture<TCanValidateResponse> {
        rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::ValidateRequests);
        auto ticket = req.Headers.Get(X_YA_SERVICE_TICKET);
        if (ticket) {
            try {
                auto clientInfo = rc.Env.Tvm->CheckClientTicket(ticket);
                rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::ValidateTvmSuccessRequests);
                return NThreading::MakeFuture(TCanValidateResponse{ECanValidateStatus::Allow, ""});
            } catch (const TInvalidTvmClientException&) {
                EVLOG_MSG << req << "Captcha validate error [forbidden] (" << CurrentExceptionMessage() << ")";
                rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::ValidateTvmDeniedRequests);
                return NThreading::MakeFuture(TCanValidateResponse{ECanValidateStatus::Deny, "Access denied. Invalid ticket."});
            } catch (...) {
                EVLOG_MSG << req << "Captcha validate error [unknown] (" << CurrentExceptionMessage() << ") spravka approved.";
                rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::ValidateTvmUnknownErrors);
                return NThreading::MakeFuture(TCanValidateResponse{ECanValidateStatus::Allow, "Granted due to an error."});
            }
        } else if (auto secret = req.CgiParams.Get("secret"); secret) {
            return rc.Env.CloudApiClient->GetSettingsByServerKey(secret).Apply([&req, &rc] (const auto& future) {
                TMaybeCaptchaSettings settings;
                if (TError err = future.GetValue().PutValueTo(settings); err.Defined()) {
                    // в случае ошибки пользователи не должны страдать
                    // поскольку для проверки необходим rc.CaptchaSettings, мы вынуждены отвечать Passed на этот запрос
                    EVLOG_MSG << EVLOG_ERROR << req << "GetSettingsByServerKey error: " << err->what();
                    rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::ValidateApiFailedRequests);
                    return NThreading::MakeFuture(TCanValidateResponse{ECanValidateStatus::ForcePassed, "Granted due to an error."});
                }
                if (!settings.Defined()) {
                    rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::ValidateInvalidSecretRequests);
                    return NThreading::MakeFuture(TCanValidateResponse{ECanValidateStatus::Deny, "Authentication failed. Invalid secret."});
                }
                rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::ValidateValidSecretRequests);
                return NThreading::MakeFuture(TCanValidateResponse{ECanValidateStatus::Allow, "", TCaptchaSettingsPtr(settings)});
            });
        } else {
            rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::ValidateUnauthenticatedRequests);
            return NThreading::MakeFuture(TCanValidateResponse{ECanValidateStatus::Deny, "Authentication failed. Secret has not provided."});
        }
    };

    return getValidateStatus().Apply([&req, &rc] (const auto& future) -> NThreading::TFuture<TCaptchaValidateResponse> {
        auto canValidate = future.GetValue();
        if (canValidate.Status == ECanValidateStatus::ForcePassed) {
            return NThreading::MakeFuture(TCaptchaValidateResponse{true, HTTP_OK, canValidate.Message, canValidate.Settings});
        }
        if (canValidate.Status == ECanValidateStatus::Deny) {
            return NThreading::MakeFuture(TCaptchaValidateResponse{false, HTTP_FORBIDDEN, canValidate.Message, canValidate.Settings});
        }

        try {
            TStringBuf spravkaStr = req.CgiParams.Has("token") ? req.CgiParams.Get("token") : req.CgiParams.Get("spravka");
            TStringBuf ipStr = req.CgiParams.Get("ip");
            TSpravka spravka;

            if (!ipStr) {
                return NThreading::MakeFuture(TCaptchaValidateResponse{false, HTTP_OK, "invaid ip", canValidate.Settings});
            }

            if (canValidate.Settings && canValidate.Settings->Getsuspend()) {
                rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::ValidateSuspended);
                return NThreading::MakeFuture(TCaptchaValidateResponse{true, HTTP_OK, "Passed due to suspension.", canValidate.Settings});
            }

            // здесь наступает момент, когда мы пользователю отдаём полезную информацию
            // является ли запрос роботным (является ли справка валидной)
            // за это мы берём деньги
            ProcessBillingEvent(rc, canValidate.Settings);

            if (!ParseSpravka(req, spravka, spravkaStr, canValidate.Settings)) {
                rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::ValidateInvalidSpravkaRequests);
                return NThreading::MakeFuture(TCaptchaValidateResponse{false, HTTP_OK, "Token invaid or expired.", canValidate.Settings});
            }

            return rc.Env.SpravkaSessionsStorage->CheckSession(req, spravka).Apply([settings = canValidate.Settings] (const auto& future) {
                bool passed = future.GetValue();
                return NThreading::MakeFuture(TCaptchaValidateResponse{passed, HTTP_OK, "", settings});
            });
        } catch (...) {
            EVLOG_MSG << req << "Captcha validate error [unknown] (" << CurrentExceptionMessage() << ") spravka approved.";
            rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::ValidateUnknownErrors);
            return NThreading::MakeFuture(TCaptchaValidateResponse{true, HTTP_OK, "Granted due to an error.", canValidate.Settings});
        }
    }).Apply([&rc] (const auto& future) {
        auto status = future.GetValue();
        TString outputString;
        TStringOutput so(outputString);
        NJsonWriter::TBuf json(NJsonWriter::HEM_DONT_ESCAPE_HTML, &so);
        json.BeginObject();
        json.WriteKey("status").WriteString(status.Passed ? "ok" : "failed");
        json.WriteKey("message").WriteString(status.Message);
        json.EndObject();
        if (status.Passed) {
            rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::ValidateSuccess);
            rc.Env.ResourceMetricsLog.Track(ECaptchaUserMetricKey::ValidateSuccess, status.Settings);
        } else {
            rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::ValidateFailed);
            rc.Env.ResourceMetricsLog.Track(ECaptchaUserMetricKey::ValidateFailed, status.Settings);
        }
        return NThreading::MakeFuture(TResponse::ToUser(status.Code).SetContent(outputString, "application/json"));
    });
}

NThreading::TFuture<TResponse> HandleCaptchaIframeHtml(TRequestContext& rc) {
    const TRequest& req = *rc.Req;

    auto getStatus = [&req, &rc]() -> NThreading::TFuture<TCaptchaIframeResult> {
        const TString& siteKey = req.CgiParams.Get("sitekey");
        if (!siteKey) {
            return NThreading::MakeFuture(TCaptchaIframeResult{ECaptchaIframeStatus::InvalidKey});
        }

        return rc.Env.CloudApiClient->GetSettingsByClientKey(siteKey).Apply([&req, &rc] (const auto& future) {
            TString refererHost = TString(GetHost(req.Referer()));
            TMaybeCaptchaSettings settings;
            if (TError err = future.GetValue().PutValueTo(settings); err.Defined()) {
                // в случае ошибки пользователи не должны страдать, разрешаем показ
                EVLOG_MSG << EVLOG_ERROR << req << "GetSettingsByClientKey error: " << err->what();
                rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::IframeApiFailedRequests);
                return NThreading::MakeFuture(TCaptchaIframeResult{ECaptchaIframeStatus::Allowed});
            }
            if (!settings.Defined()) {
                return NThreading::MakeFuture(TCaptchaIframeResult{ECaptchaIframeStatus::InvalidKey});
            }

            for (const auto& site : settings->Getallowed_sites()) {
                if (site == "*" || refererHost == site || refererHost.EndsWith("." + site)) {
                    return NThreading::MakeFuture(TCaptchaIframeResult{ECaptchaIframeStatus::Allowed, TCaptchaSettingsPtr(settings)});
                }
            }
            return NThreading::MakeFuture(TCaptchaIframeResult{ECaptchaIframeStatus::WrongDomain});
        });
    };

    return getStatus().Apply([&req, &rc] (const auto& future) {
        TCaptchaIframeResult result = future.GetValue();

        switch (result.Status) {
        case ECaptchaIframeStatus::Allowed:
            rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::IframeAllowedRequests);
            break;
        case ECaptchaIframeStatus::WrongDomain:
            rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::IframeWrongDomainRequests);
            break;
        case ECaptchaIframeStatus::InvalidKey:
            rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::IframeInvalidSitekeyRequests);
            break;
        }
        bool suspended = result.Settings && result.Settings->Getsuspend();
        if (suspended) {
            rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::IframeSuspendedRequests);
        }
        if (req.Doc.StartsWith("/checkbox")) {
            rc.Env.ResourceMetricsLog.Track(ECaptchaUserMetricKey::CheckboxShows, result.Settings);
            rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::IframeCheckboxShows);
        } else {
            rc.Env.ResourceMetricsLog.Track(ECaptchaUserMetricKey::AdvancedShows, result.Settings);
            rc.Env.CaptchaStat->CaptchaApiServiceCounters.Inc(TCaptchaStat::ECaptchaApiServiceCounter::IframeAdvancedShows);
        }

        NAntirobotEvClass::TCaptchaIframeShow event = {
            /* Header */            req.MakeLogHeader(),
            /* SiteKey */           req.CgiParams.Get("sitekey"),
            /* Status */            static_cast<ui32>(result.Status)
        };
        NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(event);
        ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);

        TStringStream contentOutput;
        TStaticData::Instance().PathHandle(ToString(req.Doc).c_str(), contentOutput);

        THttpInput httpInput(&contentOutput);

        auto resp = TResponse::ToUser(HTTP_OK);
        for (const auto& header : httpInput.Headers()) {
            if (IsIn({"Cache-Control", "Content-Length"}, header.Name())) {
                continue;
            }
            resp.AddHeader(header);
        }
        resp.AddHeader("Cache-Control", "no-cache, no-store, max-age=0, must-revalidate");
        resp.AddHeader("Access-Control-Allow-Origin", "*");

        TString errorPlaceholder = "__errorFlagBackend";
        TString content = httpInput.ReadAll();

        TStringBuf left;
        TStringBuf right;

        if (TStringBuf(content).TrySplit(errorPlaceholder, left, right)) {
            switch (result.Status) {
            case ECaptchaIframeStatus::WrongDomain:
                errorPlaceholder = "\"wrong-domain\"";
                break;
            case ECaptchaIframeStatus::InvalidKey:
                errorPlaceholder = "\"invalid-key\"";
                break;
            case ECaptchaIframeStatus::Allowed:
                if (suspended) {
                    errorPlaceholder = "\"suspend\"";
                }
                break;
            }
            content = left + errorPlaceholder + right;
        }

        if (TStringBuf(content).TrySplit("__cssTokensBackend", left, right)) {
            TString style = result.Settings ? result.Settings->Getstyle_json() : "";
            if (style == "") {
                style = "{}";
            }
            content = left + style + right;
        }

        resp.SetContent(content);
        return NThreading::MakeFuture(std::move(resp));
    });
}

NThreading::TFuture<TResponse> HandleCaptchaDemo(TRequestContext& rc) {
    NThreading::TFuture<bool> validateFuture;
    TCgiParameters cgi(rc.Req->ContentData);
    bool isValidateRequest = rc.Req->RequestMethod == "post";
    if (isValidateRequest) {
        THostAddr validateHost = CreateHostAddr("localhost:" + ToString(ANTIROBOT_DAEMON_CONFIG.Port));
        auto httpRequest = HttpGet(validateHost, "/validate");
        httpRequest.AddHeader("X-Forwarded-For-Y", ToString(rc.Req->UserAddr))
                   .AddCgiParam("ip", ToString(rc.Req->UserAddr))
                   .AddCgiParam("secret", GetEnv("SMARTCAPTCHA_SECRET_KEY"))
                   .AddCgiParam("token", cgi.Get("smart-token"));

        validateFuture = FetchHttpDataAsync(&TGeneralNehRequester::Instance(), validateHost, httpRequest, TDuration::MilliSeconds(300), "http")
            .Apply([](const auto& future) -> TErrorOr<bool> {
                NNeh::TResponseRef response;
                if (TError err = future.GetValue().PutValueTo(response); err.Defined()) {
                    return TError(__LOCATION__ + yexception() << "Failed to send request (" << err->what() << ")");
                }
                if (response && !response->IsError()) {
                    NJson::TJsonValue jsonValue;
                    if (!NJson::ReadJsonTree(response->Data, &jsonValue)) {
                        return TError(__LOCATION__ + yexception() << "invalid json: " << response->Data);
                    }
                    return !jsonValue.Has("status") || !jsonValue["status"].IsString() || jsonValue["status"].GetString() == "ok";
                }
                if (!response) {
                    return TError(__LOCATION__ + yexception() << "Failed to send request (null response)");
                } else {
                    return TError(__LOCATION__ + yexception() << "Failed to send request "
                                                                << "(type=" << static_cast<int>(response->GetErrorType())
                                                                << "; code=" << response->GetErrorCode()
                                                                << "; system code=" << response->GetSystemErrorCode()
                                                                << "): " << response->GetErrorText()
                                                                << "; " << response->Data.substr(0, 2048));
                }
            }).Apply([](const auto& future) -> bool {
                bool result;
                if (TError err = future.GetValue().PutValueTo(result); err.Defined()) {
                    EVLOG_MSG << EVLOG_ERROR << "Demo validation error: " << err->what();
                    return true;
                }
                return result;
            });
    } else {
        validateFuture = NThreading::MakeFuture<bool>(true);
    }

    TString name = cgi.Get("name");
    if (!name) {
        name = "user";
    }
    return validateFuture.Apply([isValidateRequest, name, &rc](const auto& future) {
        TStringStream contentOutput;
        TStaticData::Instance().PathHandle("/demo.html", contentOutput);
        THttpInput httpInput(&contentOutput);

        auto resp = TResponse::ToUser(HTTP_OK);
        for (const auto& header : httpInput.Headers()) {
            if (IsIn({"Cache-Control", "Content-Length"}, header.Name())) {
                continue;
            }
            resp.AddHeader(header);
        }
        resp.AddHeader("Cache-Control", "no-cache, no-store, max-age=0, must-revalidate");

        TString errorPlaceholder = "__errorPassCaptcha";
        TString content = httpInput.ReadAll();

        TStringBuf left, right;
        if (TStringBuf(content).TrySplit(errorPlaceholder, left, right)) {
            if (isValidateRequest) {
                if (future.GetValue()) {
                    TCgiParameters cgi(rc.Req->ContentData);
                    errorPlaceholder = Sprintf("<div class=\"greeting\">Hello, <strong>%s</strong>!</div>", EncodeHtmlPcdata(name).c_str());
                } else {
                    errorPlaceholder = "<div class=\"error\">Captcha validation failed</div>";
                }
            } else {
                errorPlaceholder = "<!-- placeholder -->";
            }
            content = left + errorPlaceholder + right;
        }

        if (TStringBuf(content).TrySplit("__user", left, right)) {
            content = left + EncodeHtmlPcdata(name) + right;
        }

        resp.SetContent(content);
        return NThreading::MakeFuture(std::move(resp));
    });
}

NThreading::TFuture<TResponse> HandleProcessRequest(TRequestContext& rc) {
    TMeasureDuration md(rc.Env.ProcessServerTimeStatsHandle);
    try {
        NCacheSyncProto::TMessage message;
        if (!message.ParseFromString(rc.Req->ContentData)) {
            ythrow yexception() << "Cache sync message parse error";
        }
        if (!message.HasRequest()) {
            ythrow yexception() << "The message doesn't contain request";
        }

        const auto& pbRequest = message.GetRequest();
        THolder<TRequest> unwrappedRequest = ParseFullreq(pbRequest, rc.Env);
        TRequestContext newContext = {
            rc.Env,
            unwrappedRequest.Release(),
            message.GetRequest().GetIsTraining(),
            message.GetRequest().GetCacherHost(),
            static_cast<ECaptchaRedirectType>(message.GetRequest().GetRedirectType()),
            static_cast<TInstant>(TInstant::MicroSeconds(message.GetRequest().GetFirstArrivalTime())),
            message.GetRequest().GetWasBlocked(),
            message.GetRequest().GetBlockReason(),
            message.GetRequest().GetRobotnessHeaderValue(),
            message.GetRequest().GetSuspiciousness(),
            message.GetRequest().GetCbbWhitelist(),
            message.GetRequest().GetSuspiciousBan(),
            message.GetRequest().GetCatboostWhitelist(),
            message.GetRequest().GetDegradation(),
            message.GetRequest().GetBanSourceIp(),
            message.GetRequest().GetBanFWSourceIp(),
            message.GetRequest().GetCacherFormulaResult(),
            message.GetRequest().GetCatboostBan(),
            message.GetRequest().GetApiAutoVersionBan(),
        };
        rc.Env.RequestProcessingQueue->Add(newContext);

        NCacheSyncProto::TMessage response;
        WriteBlockSyncResponse(*rc.Env.Blocker, newContext.Req->Uid, response.MutableBlocks());
        rc.Env.Robots->WriteSyncResponse(
            newContext.Req->Uid,
            ParseBanActions(newContext.Req->Uid, message.GetBans()),
            response.MutableBans()
        );

        ApplyBlockSyncRequest(message.GetBlocks(), &*rc.Env.Blocker);
        rc.Env.Robots->ApplySyncRequest(message.GetBans());

        return NThreading::MakeFuture(std::move(TResponse::ToUser(HTTP_OK).SetContent(response.SerializeAsString())));
    } catch (yexception&) {
        const auto exceptionMessage = CurrentExceptionMessage();
        EVLOG_MSG << EVLOG_ERROR << "Process failure " << rc.Req->RequesterAddr << " " << exceptionMessage;
        return NThreading::MakeFuture(TResponse::ToUser(HTTP_BAD_REQUEST).SetContent(exceptionMessage));
    }
}

NThreading::TFuture<TResponse> HandleProcessCaptchaInput(TRequestContext& rc) {
    TMeasureDuration md(rc.Env.ProcessServerTimeStatsHandle);

    try {
        NCacheSyncProto::TCaptchaInput message;
        if (!message.ParseFromString(rc.Req->ContentData)) {
            ythrow yexception() << "Captcha input message parse error";
        }

        THolder<TRequest> unwrappedRequest = ParseFullreq(message.GetRequest(), rc.Env);
        TRequestContext newContext = { rc.Env, unwrappedRequest.Release() };
        rc.Env.CaptchaInputProcessingQueue->Add({newContext, message.GetIsCorrect()});

        return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK));
    } catch (yexception&) {
        const auto exceptionMessage = CurrentExceptionMessage();
        EVLOG_MSG << EVLOG_ERROR << "Captcha input process failure " << rc.Req->RequesterAddr << " " << exceptionMessage;
        return NThreading::MakeFuture(TResponse::ToUser(HTTP_BAD_REQUEST).SetContent(exceptionMessage));
    }
}

bool CutRequestForwarding(TRequestContext& rc) {
    if (rc.MatchedRules->CutRequest) {
        rc.Env.ServiceExpBinCounters.Inc(*rc.Req, TEnv::EServiceExpBinCounter::CutRequests);
        return true;
    }
    return false;
}

bool NotBanRequest(TRequestContext& rc) {
    if (rc.MatchedRules->NotBanned) {
        rc.CbbWhitelist = true;
        rc.Env.DisablingStat.AddNotBannedRequestCbb(rc.Req->ExperimentBin());
    }
    return !rc.MatchedRules->NotBanned.empty();
}


}
