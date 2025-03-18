#include "captcha_gen.h"

#include "captcha_key.h"
#include "captcha_page_params.h"
#include "captcha_parse.h"
#include "config_global.h"
#include "eventlog_err.h"
#include "neh_requesters.h"
#include "host_ops.h"
#include "request_classifier.h"
#include "return_path.h"

#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/http_helpers.h>
#include <antirobot/lib/keyring.h>
#include <antirobot/lib/kmp_skip_search.h>
#include <antirobot/lib/spravka.h>

#include <util/generic/xrange.h>
#include <util/generic/yexception.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/system/yassert.h>

namespace NAntiRobot {
    namespace {
        NThreading::TFuture<TErrorOr<TCaptcha>> GenerateCaptchaAsync(const THttpRequest& request) {
            const auto timeout = ANTIROBOT_DAEMON_CONFIG.CaptchaGenTimeout;
            const auto& future = FetchHttpDataAsync(&TCaptchaNehRequester::Instance(), ANTIROBOT_DAEMON_CONFIG.CaptchaApiHost, request, timeout, "http");

            return future.Apply([](const NThreading::TFuture<TErrorOr<NNeh::TResponseRef>>& future) -> TErrorOr<TCaptcha> {
                NNeh::TResponseRef response;
                if (TError err = future.GetValue().PutValueTo(response); err.Defined()) {
                    return err;
                }
                if (response && !response->IsError()) {
                    return TryParseApiCaptchaResponse(response->Data);
                }
                if (!response) {
                    return TError(__LOCATION__ + TCaptchaGenerationError() << "Failed to send request (null response)");
                } else {
                    return TError(__LOCATION__ + TCaptchaGenerationError() << "Failed to send request "
                                                                           << "(type=" << static_cast<int>(response->GetErrorType())
                                                                           << "; code=" << response->GetErrorCode()
                                                                           << "; system code=" << response->GetSystemErrorCode()
                                                                           << "): " << response->GetErrorText());
                }
            });
        }
    } // anonymous namespace

    TString GetCaptchaTypeByTld(TStringBuf tld, bool nonBrandedPartner,
                                EHostType hostType, EClientType clientType) {

        const TAntirobotDaemonConfig::TZoneConf& conf = ANTIROBOT_DAEMON_CONFIG.ConfByTld(tld);
        const auto& captchaTypes = conf.CaptchaTypes;

        TVector<TString> keysByPriority = {ToString(hostType), DEFAULT_CAPTCHA_TYPE_SERVICE};
        if (nonBrandedPartner) {
            keysByPriority.insert(keysByPriority.begin(), NON_BRANDED_PARTNER_CAPTCHA_TYPE_SERVICE);
        }

        for (const auto& key : keysByPriority) {
            if (clientType == EClientType::CLIENT_AJAX) {
                TString ajaxKey = key + ":ajax";
                if (captchaTypes.contains(ajaxKey)) {
                    return captchaTypes.find(ajaxKey)->second.Choose();
                }
            }
            if (clientType == EClientType::CLIENT_XML_PARTNER) {
                TString xmlKey = key + ":xml";
                if (captchaTypes.contains(xmlKey)) {
                    return captchaTypes.find(xmlKey)->second.Choose();
                }
            }
            if (captchaTypes.contains(key)) {
                return captchaTypes.find(key)->second.Choose();
            }
        }
        Y_ASSERT(false);
        return captchaTypes.find(DEFAULT_CAPTCHA_TYPE_SERVICE)->second.Choose();
    }

    TString GetCaptchaType(const TRequest& req, bool nonBrandedPartner, TCaptchaSettingsPtr settings) {
        if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
            auto complexity = NCloudApi::CaptchaComplexity::CAPTCHA_COMPLEXITY_UNSPECIFIED;
            if (settings) {
                complexity = settings->Getcomplexity();
            }
            TString type;
            switch (complexity) {
            case NCloudApi::CaptchaComplexity::EASY:
                type = "v2";
                break;
            case NCloudApi::CaptchaComplexity::HARD:
                type = "v3";
                break;
            default:
                type = "v1";
            }
            return req.Lang == LANG_RUS ? "txt_" + type : "txt_" + type + "_en";
        }
        return GetCaptchaTypeByTld(req.Tld, nonBrandedPartner, req.HostType, req.ClientType);
    }

    NThreading::TFuture<TCaptcha> TryGenerateCaptcha(int attempt, const THttpRequest& request, TCaptchaStat& captchaStat) {
        if (attempt >= ANTIROBOT_DAEMON_CONFIG.CaptchaGenTryCount) {
            try {
                ythrow TCaptchaGenerationError() << "Failed to generate captcha after "
                                                 << ANTIROBOT_DAEMON_CONFIG.CaptchaGenTryCount << " tries";
            } catch (...) {
                return NThreading::MakeErrorFuture<TCaptcha>(std::current_exception());
            }
        }
        auto res = GenerateCaptchaAsync(request);
        return res.Apply([attempt, request, &captchaStat](const NThreading::TFuture<TErrorOr<TCaptcha>>& future) -> NThreading::TFuture<TCaptcha> {
            TCaptcha captcha;
            if (TError err = future.GetValue().PutValueTo(captcha); err.Defined()) {
                EVLOG_MSG << "Captcha access error (step " << attempt + 1 << ", " << err->what() << ")";
                return TryGenerateCaptcha(attempt + 1, request, captchaStat);
            }
            captchaStat.UpdateTriesStat(attempt);
            return NThreading::MakeFuture(captcha);
        });
    }

    TString GetVoiceLanguage(ELanguage lang) {
        switch (lang) {
            case LANG_RUS:
            case LANG_UKR:
            case LANG_BEL:
            case LANG_KAZ:
            case LANG_UZB:
                return "ru";
            case LANG_TUR:
                return "tr";
            default:
                return "en";
        }
    }

    NThreading::TFuture<TErrorOr<TCaptcha>> MakeAdvancedCaptchaAsync(const TRequest& req,
                                                                     bool nonBrandedPartner,
                                                                     TCaptchaStat& captchaStat,
                                                                     TCaptchaSettingsPtr settings)
    {
        static const TString method = "/generate";

        auto voiceLang = GetVoiceLanguage(req.Lang);

        auto request = HttpGet(ANTIROBOT_DAEMON_CONFIG.CaptchaApiHost, method);
        TString captchaType = GetCaptchaType(req, nonBrandedPartner, settings);

        request.AddCgiParam("type", captchaType)
            .AddCgiParam("vtype", voiceLang)
            .AddCgiParam("client", "antirobot")
            .AddCgiParam("service-for", ToString(req.HostType))
            .AddCgiParam("client-type", ToString(req.ClientType))
            .AddCgiParam("json", "1");

        if (req.Scheme == EScheme::HttpSecure) {
            request.AddCgiParam("https", "on");
        }

        auto addCaptchaType = [captchaType](TCaptcha&& c) {
            c.CaptchaType = captchaType;
            return std::move(c);
        };

        captchaStat.IncRequestCount();
        auto measureDuration = MakeAtomicShared<TMeasureDuration>(captchaStat.GetApiCaptchaGenerateResponseTimeStats());

        return TryGenerateCaptcha(0, request, captchaStat).Apply(
            [addCaptchaType, measureDuration](const auto& future) mutable -> TErrorOr<TCaptcha> {
            TCaptcha captcha;
            measureDuration.Drop();
            try {
                captcha = future.GetValue();
            } catch (const TCaptchaGenerationError& ex) {
                return TError(ex);
            } catch (...) {
                return TError(__LOCATION__ + yexception() << CurrentExceptionMessage());
            }
            return addCaptchaType(std::move(captcha));
        });
    }

    /*
     * Captcha page URL related functions
     */
    namespace {

    TStringBuf GetCaptchaHost(const TRequest& req) {
        if (ANTIROBOT_DAEMON_CONFIG.CaptchaRedirectToSameHost) {
            return req.HostWithPort;
        }

        TStringBuf yandexDomain = GetYandexDomainFromHost(req.Host);

        if (!yandexDomain || yandexDomain.EndsWith(TStringBuf(".net"))) {
            return "yandex.ru"sv;
        }

        return yandexDomain;
    }

    } // anonymous namespace

    TString CalcCaptchaPageUrl(const TRequest& req, const TReturnPath& retPath,
                              bool again, const TCaptchaToken& token, bool noSpravka)
    {
        TString location;
        TStringOutput locOut(location);

        TCgiParameters cgi;

        retPath.AddToCGI(cgi);
        cgi.InsertUnescaped(TStringBuf("t"), token.AsString);
        cgi.InsertUnescaped(TStringBuf("u"), req.UniqueKey);
        if (again) {
            cgi.InsertUnescaped(TStringBuf("status"), TStringBuf("failed"));
        }
        if (noSpravka) {
            cgi.InsertUnescaped(TStringBuf("cc"), TStringBuf("1"));
        }

        locOut << req.Scheme << GetCaptchaHost(req) << "/showcaptcha?"sv;
        locOut << cgi.Print();

        const TString locationSignature = TKeyRing::Instance()->SignHex(CutHttpPrefix(location));
        locOut << "&s=" << locationSignature;
        return location;
    }

    void CheckCaptchaPageRequestSigned(const TRequest& req) {
        static const TKmpSkipSearch searchSignature("&s=");

        TStringBuf signature = searchSignature.SearchInText(req.RequestString);
        if (signature.empty())
            ythrow TCaptchaPageRequestBad() << "Captcha page request has no signature";

        TStringBuf strippedRequest(req.RequestString.data(), signature.data());
        const TString& realSign = req.CgiParams.Get(TStringBuf("s"));

        const TStringBuf& host = req.HostWithPort;

        TTempBuf buf(host.size() + strippedRequest.size());
        buf.Append(host.data(), host.size());
        buf.Append(strippedRequest.data(), strippedRequest.size());

        if (!req.UserAddr.IsYandex() && !TKeyRing::Instance()->IsSignedHex(TStringBuf(buf.Data(), buf.Filled()), realSign))
            ythrow TCaptchaPageRequestBad() << "Captcha page request has bad signature";
    }

    /*
     * TCheckCookiesResult
     */
    TCheckCookiesResult::TCheckCookiesResult(const TRequest& req)
        : CookiesEnabled(true)
        , TestCookieSet(false)
        , TestCookieSuccess(false)
    {
        if (req.CgiParams.Has(TStringBuf("cc"), TStringBuf("1"))) {
            TestCookieSet = true;
            if (!req.HasSpravka()) {
                CookiesEnabled = false;
            } else {
                TestCookieSuccess = true;
            }
        }
    }
} // namespace NAntiRobot
