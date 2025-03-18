#include "captcha_check.h"
#include "captcha_key.h"
#include "captcha_signature_data.h"
#include "environment.h"
#include "fullreq_info.h"

#include <antirobot/lib/test_server.h>
#include <antirobot/lib/uri.h>
#include <antirobot/daemon_lib/ut/utils.h>

#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/server/response.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/string/builder.h>

#include <library/cpp/string_utils/base64/base64.h>

using namespace NAntiRobot;


Y_UNIT_TEST_SUITE_IMPL(TTestCaptchaCheck, TTestAntirobotMediumBase) {
    const TCaptchaKey InvalidKey = {"something invalid", TCaptchaToken(ECaptchaType::SmartAdvanced, ""), ""};
    const TCaptchaKey ValidKey = {"003AWu2dra6Gi8JPT41DePVoFvtbiA1I", TCaptchaToken(ECaptchaType::SmartAdvanced, ""), ""};
    const TString CorrectRep = "snnciie 05022017";
    const TString UserAgent = "User-Agent: Opera/9.80 (Windows NT 5.1) Presto/2.12.388 Version/12.11";
    const TString JsPrintUserAgent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.101 YaBrowser/20.7.0.1230 Yowser/2.5 Safari/537.36";
    const TString Referer = "http://yandex.ru/search?text=cats";
    const TString YandexUid = "345234523456456";
    const TString UserIp = "1.1.1.1";
    const TString UrlSign = "29a99098a0a88f4737d5a9aa1908fdfc";

    THolder<TRequest> CreateCaptchaRequestContext(TEnv& env, const TString& cgi = "", const TString& requestBody = "") {
        TString url = "/checkcaptcha"
            "?key=" + ValidKey.ImageKey + "_0/1602148765/6d61e99d8c6eab2e1fed14df2d7a6b65_1723fcce8121d60c30c0bc45130b53aa"
            "&retpath=" + EncodeUriComponent(Base64EncodeUrl(Referer)) + "_" + UrlSign;

        const TString post = \
            "POST " + url + cgi + " HTTP/1.1\r\n"
            "User-Agent: " + UserAgent + "\r\n"
            "Host: yandex.ru\r\n"
            "Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, */*;q=0.1\r\n"
            "Accept-Language: ru-RU,ru;q=0.9,en;q=0.8\r\n"
            "Accept-Encoding: gzip, deflate\r\n"
            "Referer: " + Referer + "\r\n"
            "Cookie: yandexuid=" + YandexUid + ";\r\n"
            "Connection: Keep-Alive\r\n"
            "X-Forwarded-For-Y: " + UserIp + "\r\n"
            "X-Source-Port-Y: 54542\r\n"
            "X-Start-Time: 1354193658054839\r\n"
            "X-Req-Id: 1354193658054839-6563412409256195394\r\n"
            "\r\n" + requestBody;

        TStringInput stringInput{post};
        THttpInput input{&stringInput};

        return MakeHolder<TFullReqInfo>(input, post, "0.0.0.0", env.ReloadableData, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
    }

    TString EncodeVerochka(TStringBuf verochkaData) {
        TCaptchaSignatureData signatureData(UserIp);
        const auto encryptedVerochkaData = signatureData.EncryptEncode(verochkaData);

        return TStringBuilder()
            << "d=" << EncodeUriComponent(signatureData.EncodedEncryptionKey) << '&'
            << "k=" << EncodeUriComponent(signatureData.Signature) << '&'
            << "rdata=" << EncodeUriComponent(encryptedVerochkaData);
    }

    struct TApiCaptchaTestReplier: public TRequestReplier {
        TDuration Timeout;

        TApiCaptchaTestReplier(TDuration timeout = TDuration::Zero())
            : Timeout(timeout)
        {
        }

        bool DoReply(const TReplyParams& params) override {
            TParsedHttpFull request(params.Input.FirstLine());
            if (request.Path != "/check") {
                params.Output << THttpResponse(HTTP_NOT_FOUND);
                return true;
            }
            TCgiParameters parameters(request.Cgi);
            if (!parameters.Has("key") || !parameters.Has("rep") || parameters.Get("key") != ValidKey.ImageKey) {
                params.Output << THttpResponse(HTTP_BAD_REQUEST);
                return true;
            }

            if (Timeout > TDuration::Zero()) {
                Sleep(Timeout);
            }

            const TString& rep = parameters.Get("rep");

            if (rep == CorrectRep) {
                params.Output << THttpResponse(HTTP_OK).SetContent("{\"status\":\"ok\"}");
            } else {
                params.Output << THttpResponse(HTTP_OK).SetContent("{\"status\":\"failed\"}");
            }

            return true;
        }

        TApiCaptchaTestReplier* Clone() const {
            return new TApiCaptchaTestReplier(Timeout);
        }
    };

    struct TFuryTestReplier: public TRequestReplier {
        TDuration Timeout;
        TString* InputRequestBuf;

        TFuryTestReplier(TDuration timeout = TDuration::Zero(), TString* inputRequestBuf = nullptr)
            : Timeout(timeout)
            , InputRequestBuf(inputRequestBuf)
        {
        }

        bool DoReply(const TReplyParams& params) override {
            TParsedHttpFull request(params.Input.FirstLine());

            TCgiParameters parameters(request.Cgi);
            Y_UNUSED(parameters);

            if (Timeout > TDuration::Zero()) {
                Sleep(Timeout);
            }

            auto requestContent = params.Input.ReadAll();
            if (InputRequestBuf) {
                *InputRequestBuf = requestContent;
            }

            NJson::TJsonValue jsonValue;
            if (!NJson::ReadJsonTree(requestContent, &jsonValue)) {
                params.Output << THttpResponse(HTTP_BAD_REQUEST);
                return true;
            }

            params.Output << THttpResponse(HTTP_OK).SetContent("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":[]}");
            return true;
        }

        TFuryTestReplier* Clone() const {
            return new TFuryTestReplier(Timeout, InputRequestBuf);
        }
    };

    template <typename ClientRequest>
    struct TCloneCallback: public THttpServer::ICallBack {
        THolder<ClientRequest> Replier;

        TCloneCallback(THolder<ClientRequest> replier)
            : Replier(std::move(replier))
        {
        }

        TClientRequest* CreateClient() override {
            return Replier->Clone();
        }
    };

    Y_UNIT_TEST(CorrectAnswer) {
        TCloneCallback<TApiCaptchaTestReplier> callback(MakeHolder<TApiCaptchaTestReplier>());
        TTestServer captchaApiServer(callback);

        auto& cfg = ANTIROBOT_DAEMON_CONFIG_MUTABLE;
        cfg.CaptchaApiHost = captchaApiServer.Host;
        cfg.CaptchaCheckTimeout = TDuration::Seconds(1);
        cfg.FuryEnabled = false;

        TEnv env;
        auto req = CreateCaptchaRequestContext(env, "&rep=" + EncodeUriComponent(CorrectRep));
        TRequestContext rc{env, req.Release()};

        auto future = IsCaptchaGoodAsync(ValidKey, rc, cfg, TCaptchaSettingsPtr{});
        TCaptchaCheckResult result = future.GetValueSync();
        for (auto& err : result.ErrorMessages) {
            Cerr << err << Endl;
        }
        UNIT_ASSERT(!result.WasApiCaptchaError);
        UNIT_ASSERT(!result.WasFuryError);
        UNIT_ASSERT(result.ErrorMessages.size() == 0);
        UNIT_ASSERT(result.Success);
    }

    Y_UNIT_TEST(IncorrectAnswer) {
        TCloneCallback<TApiCaptchaTestReplier> callback(MakeHolder<TApiCaptchaTestReplier>());
        TTestServer captchaApiServer(callback);

        auto& cfg = ANTIROBOT_DAEMON_CONFIG_MUTABLE;
        cfg.CaptchaApiHost = captchaApiServer.Host;
        cfg.CaptchaCheckTimeout = TDuration::Seconds(1);
        cfg.FuryEnabled = false;

        TEnv env;
        auto req = CreateCaptchaRequestContext(env, "&rep=" + EncodeUriComponent("something incorrect"));
        TRequestContext rc{env, req.Release()};

        auto future = IsCaptchaGoodAsync(ValidKey, rc, cfg, TCaptchaSettingsPtr{});
        TCaptchaCheckResult result = future.GetValueSync();

        for (auto& err : result.ErrorMessages) {
            Cerr << err << Endl;
        }
        UNIT_ASSERT(!result.WasApiCaptchaError);
        UNIT_ASSERT(!result.WasFuryError);
        UNIT_ASSERT(result.ErrorMessages.size() == 0);
        UNIT_ASSERT(!result.Success);
    }

    Y_UNIT_TEST(Timeout) {
        TCloneCallback<TApiCaptchaTestReplier> callback(MakeHolder<TApiCaptchaTestReplier>(TDuration::MilliSeconds(100)));
        TTestServer captchaApiServer(callback);

        auto& cfg = ANTIROBOT_DAEMON_CONFIG_MUTABLE;
        cfg.CaptchaApiHost = captchaApiServer.Host;
        cfg.CaptchaCheckTimeout = TDuration::MilliSeconds(50);
        cfg.FuryEnabled = false;

        TEnv env;
        auto req = CreateCaptchaRequestContext(env, "&rep=" + EncodeUriComponent(CorrectRep));
        TRequestContext rc{env, req.Release()};

        auto future = IsCaptchaGoodAsync(ValidKey, rc, cfg, TCaptchaSettingsPtr{});
        TCaptchaCheckResult result = future.GetValueSync();

        UNIT_ASSERT(result.WasApiCaptchaError);
        UNIT_ASSERT(result.ErrorMessages.size() == 1);
        UNIT_ASSERT(!result.WasFuryError);
        UNIT_ASSERT(result.Success);
    }

    Y_UNIT_TEST(BadRequest) {
        TCloneCallback<TApiCaptchaTestReplier> callback(MakeHolder<TApiCaptchaTestReplier>());
        TTestServer captchaApiServer(callback);

        auto& cfg = ANTIROBOT_DAEMON_CONFIG_MUTABLE;
        cfg.CaptchaApiHost = captchaApiServer.Host;
        cfg.CaptchaCheckTimeout = TDuration::Seconds(1);
        cfg.FuryEnabled = false;

        TEnv env;
        auto req = CreateCaptchaRequestContext(env, "&rep=" + EncodeUriComponent(CorrectRep));
        TRequestContext rc{env, req.Release()};

        auto future = IsCaptchaGoodAsync(InvalidKey, rc, cfg, TCaptchaSettingsPtr{});
        TCaptchaCheckResult result = future.GetValueSync();

        UNIT_ASSERT(result.WasApiCaptchaError);
        UNIT_ASSERT(result.ErrorMessages.size() == 1);
        UNIT_ASSERT(!result.WasFuryError);
        UNIT_ASSERT(result.Success);
    }

    Y_UNIT_TEST(BadAddr) {
        TCloneCallback<TApiCaptchaTestReplier> callback(MakeHolder<TApiCaptchaTestReplier>());
        TTestServer captchaApiServer(callback);
        captchaApiServer.Server->Shutdown();
        auto badAddr = captchaApiServer.Host;

        auto& cfg = ANTIROBOT_DAEMON_CONFIG_MUTABLE;
        cfg.CaptchaApiHost = badAddr;
        cfg.CaptchaCheckTimeout = TDuration::Seconds(1);
        cfg.FuryEnabled = false;

        TEnv env;
        auto req = CreateCaptchaRequestContext(env, "&rep=" + EncodeUriComponent(CorrectRep));
        TRequestContext rc{env, req.Release()};

        auto future = IsCaptchaGoodAsync(ValidKey, rc, cfg, TCaptchaSettingsPtr{});
        TCaptchaCheckResult result = future.GetValueSync();

        UNIT_ASSERT(result.WasApiCaptchaError);
        UNIT_ASSERT(result.ErrorMessages.size() == 1);
        UNIT_ASSERT(!result.WasFuryError);
        UNIT_ASSERT(result.Success);
    }

    Y_UNIT_TEST(DontSpillExceptionsFromNeh) {
        // Induce throw inside of Neh by passing bad protocol name

        TCloneCallback<TApiCaptchaTestReplier> callback(MakeHolder<TApiCaptchaTestReplier>());
        TTestServer captchaApiServer(callback);
        captchaApiServer.Server->Shutdown();
        auto badAddr = captchaApiServer.Host;

        auto& cfg = ANTIROBOT_DAEMON_CONFIG_MUTABLE;
        cfg.CaptchaApiHost = badAddr;
        cfg.CaptchaApiProtocol = "bad_protocol";
        cfg.CaptchaCheckTimeout = TDuration::Seconds(1);
        cfg.FuryEnabled = false;

        TEnv env;
        auto req = CreateCaptchaRequestContext(env, "&rep=" + EncodeUriComponent("asd"));
        TRequestContext rc{env, req.Release()};

        auto future = IsCaptchaGoodAsync(ValidKey, rc, cfg, TCaptchaSettingsPtr{});
        auto result = future.ExtractValueSync();

        UNIT_ASSERT(result.WasApiCaptchaError);
        UNIT_ASSERT(result.ErrorMessages.size() == 1);
        UNIT_ASSERT(!result.WasFuryError);
        UNIT_ASSERT(result.Success);
        UNIT_ASSERT(result.ErrorMessages[0].Contains("unsupported scheme"));
    }

    Y_UNIT_TEST(CorrectAnswerWithFury) {
        TCloneCallback<TApiCaptchaTestReplier> apiCaptchaReplier(MakeHolder<TApiCaptchaTestReplier>());
        TTestServer captchaApiServer(apiCaptchaReplier);
        TString furyInputRequest;
        TCloneCallback<TFuryTestReplier> furyReplier(MakeHolder<TFuryTestReplier>(TDuration::Zero(), &furyInputRequest));
        TTestServer furyServer(furyReplier);

        auto& cfg = ANTIROBOT_DAEMON_CONFIG_MUTABLE;
        cfg.CaptchaApiHost = captchaApiServer.Host;
        cfg.CaptchaCheckTimeout = TDuration::Seconds(1);
        cfg.FuryEnabled = true;
        cfg.FuryHost = furyServer.Host;
        cfg.FuryBaseTimeout = TDuration::Seconds(1);

        TEnv env;
        TString jsPrintStr = "{\
            \"a1\": \"ru\",\
            \"a2\": \"Google Inc.\",\
            \"a3\": \"20030107\",\
            \"a4\": \"ru.en\",\
            \"a5\": \"Gecko\",\
            \"a6\": 4,\
            \"a7\": true,\
            \"a8\": true,\
            \"a9\": true,\
            \"b1\": true,\
            \"b2\": true,\
            \"b3\": true,\
            \"b4\": true,\
            \"b5\": true,\
            \"b6\": true,\
            \"b7\": true,\
            \"b8\": true,\
            \"b9\": true,\
            \"c1\": 33,\
            \"c2\": \"954x1680\",\
            \"c3\": \"1680x1050\",\
            \"c4\": true,\
            \"c5\": true,\
            \"c6\": 24,\
            \"c7\": true,\
            \"c8\": 8,\
            \"d2\": 8,\
            \"d3\": true,\
            \"d4\": true,\
            \"d5\": true,\
            \"d6\": \"tz\",\
            \"d7\": -180,\
            \"d8\": \"0.false.false\",\
            \"d9\": true,\
            \"e1\": true,\
            \"e2\": true,\
            \"e3\": true,\
            \"e4\": true,\
            \"e5\": true,\
            \"e6\": true,\
            \"e7\": true,\
            \"e8\": true,\
            \"e9\": true,\
            \"f1\": true,\
            \"f2\": true,\
            \"f3\": true,\
            \"f4\": true,\
            \"f5\": true,\
            \"f6\": true,\
            \"f7\": true,\
            \"f8\": true,\
            \"f9\": true,\
            \"g1\": true,\
            \"g2\": true,\
            \"g3\": true,\
            \"g4\": true,\
            \"g5\": true,\
            \"g6\": true,\
            \"g7\": true,\
            \"g8\": true,\
            \"g9\": true,\
            \"h1\": true,\
            \"h2\": true,\
            \"h3\": true,\
            \"h4\": true,\
            \"h5\": true,\
            \"h6\": true,\
            \"h7\": \"audiocontext\",\
            \"h8\": true,\
            \"h9\": true,\
            \"i1\": true,\
            \"i2\": true,\
            \"i3\": true,\
            \"i4\": true,\
            \"i5\": true,\
            \"z3\": \"z3\",\
            \"z4\": \"z4\",\
            \"z5\": true,\
            \"z6\": \"z6\",\
            \"z7\": \"z7\",\
            \"z8\": \"z8\",\
            \"z9\": \"z9\",\
            \"y1\": \"y1\",\
            \"y2\": \"y2\",\
            \"y3\": \"y3\",\
            \"y4\": \"y4\",\
            \"y5\": \"y5\",\
            \"y6\": \"y6\",\
            \"y7\": \"y7\",\
            \"y8\": \"y8\",\
            \"y9\": \"y9\",\
            \"y10\": \"y10\",\
            \"x1\": \"x1\",\
            \"x2\": \"x2\",\
            \"x3\": \"x3\",\
            \"x4\": \"x4\",\
            \"x5\": \"x5\",\
            \"v\": \"1.1.1\",\
            \"z1\": \"ddd4548797fea7a1b8f0533709e890ec\",\
            \"z2\": \"db7196c6a1db31450938d39450b997b7\"\
        }";
        auto req = CreateCaptchaRequestContext(env, "&rep=" + EncodeUriComponent(CorrectRep), EncodeVerochka(jsPrintStr));
        TRequestContext rc{env, req.Release()};

        auto future = IsCaptchaGoodAsync(ValidKey, rc, cfg, TCaptchaSettingsPtr{});
        TCaptchaCheckResult result = future.GetValueSync();

        NJson::TJsonValue furyJson;
        UNIT_ASSERT(NJson::ReadJsonTree(furyInputRequest, &furyJson));

        UNIT_ASSERT_STRINGS_EQUAL(furyJson["jsonrpc"].GetStringSafe(), "2.0");
        UNIT_ASSERT_STRINGS_EQUAL(furyJson["method"].GetStringSafe(), "process");
        UNIT_ASSERT_STRINGS_EQUAL(furyJson["id"].GetStringSafe(), "1");

        auto furyJsonParams = furyJson["params"];
        UNIT_ASSERT_STRINGS_EQUAL(furyJsonParams["service"].GetStringSafe(), "captcha");
        UNIT_ASSERT_STRINGS_EQUAL(furyJsonParams["key"].GetStringSafe(), ValidKey.ImageKey);

        auto body = furyJsonParams["body"];
        UNIT_ASSERT_STRINGS_EQUAL(body["rep"].GetStringSafe(), CorrectRep);
        UNIT_ASSERT_VALUES_EQUAL(body["captcha_result"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(body["random_captcha"].GetBooleanSafe(), false);
        UNIT_ASSERT_STRINGS_EQUAL(body["host_type"].GetStringSafe(), "web");
        UNIT_ASSERT_STRINGS_EQUAL(body["user_agent"].GetStringSafe(), UserAgent);
        UNIT_ASSERT_STRINGS_EQUAL(body["referer"].GetStringSafe(), Referer);
        UNIT_ASSERT_STRINGS_EQUAL(body["host"].GetStringSafe(), "yandex.ru");
        UNIT_ASSERT_STRINGS_EQUAL(body["req_type"].GetStringSafe(), "other");
        UNIT_ASSERT_STRINGS_EQUAL(body["uid"].GetStringSafe(), "1-16843009");
        UNIT_ASSERT_STRINGS_EQUAL(body["yandexuid"].GetStringSafe(), YandexUid);
        UNIT_ASSERT_STRINGS_EQUAL(body["icookie"].GetStringSafe(), "");
        UNIT_ASSERT_STRINGS_EQUAL(body["spravka_addr"].GetStringSafe(), UserIp);
        UNIT_ASSERT_STRINGS_EQUAL(body["user_addr"].GetStringSafe(), UserIp);
        UNIT_ASSERT_VALUES_EQUAL(body["has_valid_icookie"].GetBooleanSafe(), false);
        UNIT_ASSERT_VALUES_EQUAL(body["has_valid_fuid"].GetBooleanSafe(), false);
        UNIT_ASSERT_VALUES_EQUAL(body["has_valid_lcookie"].GetBooleanSafe(), false);
        UNIT_ASSERT_VALUES_EQUAL(body["has_valid_spravka"].GetBooleanSafe(), false);
        UNIT_ASSERT_VALUES_EQUAL(body["has_valid_spravka_hash"].GetBooleanSafe(), false);

        auto jsPrint = body["js_print"];
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["valid_json"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["has_data"].GetBooleanSafe(), true);

        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_language"].GetStringSafe(), "ru");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_vendor"].GetStringSafe(), "Google Inc.");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_productSub"].GetStringSafe(), "20030107");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_languages"].GetStringSafe(), "ru.en");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_product"].GetStringSafe(), "Gecko");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_plugins_pluginsLength"].GetIntegerSafe(), 4);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_navigator_hasWebDriver"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_plugins_isAdblockUsed"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_ie9OrLower"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_isChromium"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_isTrident"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_isDesktopSafari"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_isChromium86OrNewer"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_isEdgeHTML"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_isGecko"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_isWebKit"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_isOpera"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_isBrave"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_getEmptyEvalLength"].GetIntegerSafe(), 33);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_availableScreenResolution"].GetStringSafe(), "954x1680");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_screenResolution"].GetStringSafe(), "1680x1050");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_getSessionStorage"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_getLocalStorage"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_getColorDepth"].GetIntegerSafe(), 24);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_areCookiesEnabled"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_deviceMemory"].GetIntegerSafe(), 8);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_cpuClass"].GetStringSafe(), "");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_oscpu"].GetStringSafe(), "");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_getHardwareConcurrency"].GetIntegerSafe(), 8);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_getErrorFF"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_getIndexedDB"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_openDatabase"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_getTimezone"].GetStringSafe(), "tz");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_getTimezoneOffset"].GetIntegerSafe(), -180);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_getTouchSupport"].GetStringSafe(), "0.false.false");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_bind"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_buffer"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_chrome"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_deviceMotionEvent"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_deviceOrientationEvent"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_emit"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_eventListener"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_innerWidth"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_outerWidth"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_pointerEvent"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_spawn"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_TouchEvent"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_xDomainRequest"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_xMLHttpRequest"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_callPhantom"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_activeXObject"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_documentMode"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_webstore"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_onLine"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_installTrigger"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_hTMLElementConstructor"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_rTCPeerConnection"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_mozInnerScreenY"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_vibrate"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_getBattery"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_forEach"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_imperva_fileReader"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_navigator_iframeChrome"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_imperva_driver"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_imperva_magicString"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_imperva_selenium"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_imperva_webdriver"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_imperva_xPathResult"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_fingerprintJsPro_runBotDetection"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_audiocontext"].GetStringSafe(), "audiocontext");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_badgingApi"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_canvasToBlob"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_intlNumberFormat"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_intlDatetimeFormat"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_navigatorLevel"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_notificationToString"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_sandboxedIframe"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_sensorsAccelerometer"].GetStringSafe(), "z3");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_speechVoices"].GetStringSafe(), "z4");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_widevineDrm"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionGeolocation"].GetStringSafe(), "z6");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionPush"].GetStringSafe(), "z7");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionMidi"].GetStringSafe(), "z8");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionCamera"].GetStringSafe(), "z9");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionMicrophone"].GetStringSafe(), "y1");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionSpeakerSelection"].GetStringSafe(), "y2");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionDeviceInfo"].GetStringSafe(), "y3");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionBackgroundFetch"].GetStringSafe(), "y4");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionBackgroundSync"].GetStringSafe(), "y5");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionBluetooth"].GetStringSafe(), "y6");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionPersistentStorage"].GetStringSafe(), "y7");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionAmbientLightSensor"].GetStringSafe(), "y8");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionGyroscope"].GetStringSafe(), "y9");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionMagnetometer"].GetStringSafe(), "y10");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionClipboardRead"].GetStringSafe(), "x1");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionClipboardWrite"].GetStringSafe(), "x2");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionDisplayCapture"].GetStringSafe(), "x3");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionNfc"].GetStringSafe(), "x4");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_permissionNotifications"].GetStringSafe(), "x5");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["env_fontsAvailable"].GetStringSafe(), "ddd4548797fea7a1b8f0533709e890ec");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["env_getCanvasFingerprint"].GetStringSafe(), "db7196c6a1db31450938d39450b997b7");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["version"].GetStringSafe(), "1.1.1");

        for (auto& err : result.ErrorMessages) {
            Cerr << err << Endl;
        }
        UNIT_ASSERT(!result.WasApiCaptchaError);
        UNIT_ASSERT(!result.WasFuryError);
        UNIT_ASSERT(result.ErrorMessages.size() == 0);
        UNIT_ASSERT(result.Success);
    }

    Y_UNIT_TEST(HackedData) {
        TCloneCallback<TApiCaptchaTestReplier> apiCaptchaReplier(MakeHolder<TApiCaptchaTestReplier>());
        TTestServer captchaApiServer(apiCaptchaReplier);
        TString furyInputRequest;
        TCloneCallback<TFuryTestReplier> furyReplier(MakeHolder<TFuryTestReplier>(TDuration::Zero(), &furyInputRequest));
        TTestServer furyServer(furyReplier);

        auto& cfg = ANTIROBOT_DAEMON_CONFIG_MUTABLE;
        cfg.CaptchaApiHost = captchaApiServer.Host;
        cfg.CaptchaCheckTimeout = TDuration::Seconds(1);
        cfg.FuryEnabled = true;
        cfg.FuryHost = furyServer.Host;
        cfg.FuryBaseTimeout = TDuration::Seconds(1);

        TEnv env;
        TString jsPrintStr = "{\
            \"a1\": 42,\
            \"a7\": \"asd\",\
            \"c8\": 100000000000000000000000000000000,\
            \"c1\": 1000000000000000000,\
            \"v\": \"1.1.1\"\
        }";
        auto req = CreateCaptchaRequestContext(env, "&rep=" + EncodeUriComponent(CorrectRep), EncodeVerochka(jsPrintStr));
        TRequestContext rc{env, req.Release()};

        auto future = IsCaptchaGoodAsync(ValidKey, rc, cfg, TCaptchaSettingsPtr{});
        TCaptchaCheckResult result = future.GetValueSync();

        NJson::TJsonValue furyJson;
        UNIT_ASSERT(NJson::ReadJsonTree(furyInputRequest, &furyJson));

        auto furyJsonParams = furyJson["params"];
        auto body = furyJsonParams["body"];
        auto jsPrint = body["js_print"];

        UNIT_ASSERT_VALUES_EQUAL(jsPrint["valid_json"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["has_data"].GetBooleanSafe(), true);

        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_language"].GetStringSafe(), "");
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["headless_navigator_hasWebDriver"].GetBooleanSafe(), false);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_navigator_deviceMemory"].GetIntegerSafe(), 0);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["browser_features_getEmptyEvalLength"].GetIntegerSafe(), 1000000000000000000ll);
        UNIT_ASSERT_VALUES_EQUAL(jsPrint["version"].GetStringSafe(), "1.1.1");

        for (auto& err : result.ErrorMessages) {
            Cerr << err << Endl;
        }
        UNIT_ASSERT(!result.WasApiCaptchaError);
        UNIT_ASSERT(!result.WasFuryError);
        UNIT_ASSERT(result.ErrorMessages.size() == 0);
        UNIT_ASSERT(result.Success);
    }

    Y_UNIT_TEST(VerochkaInvalidRequest) {
        TCloneCallback<TApiCaptchaTestReplier> apiCaptchaReplier(MakeHolder<TApiCaptchaTestReplier>());
        TTestServer captchaApiServer(apiCaptchaReplier);
        TString furyInputRequest;
        TCloneCallback<TFuryTestReplier> furyReplier(MakeHolder<TFuryTestReplier>(TDuration::Zero(), &furyInputRequest));
        TTestServer furyServer(furyReplier);

        auto& cfg = ANTIROBOT_DAEMON_CONFIG_MUTABLE;
        cfg.CaptchaApiHost = captchaApiServer.Host;
        cfg.CaptchaCheckTimeout = TDuration::Seconds(1);
        cfg.FuryEnabled = true;
        cfg.FuryHost = furyServer.Host;
        cfg.FuryBaseTimeout = TDuration::Seconds(1);

        TEnv env;
        {
            TString jsPrintStr = "{invalid_json";
            auto req = CreateCaptchaRequestContext(env, "&rep=" + EncodeUriComponent(CorrectRep), EncodeVerochka(jsPrintStr));
            TRequestContext rc{env, req.Release()};

            auto future = IsCaptchaGoodAsync(ValidKey, rc, cfg, TCaptchaSettingsPtr{});
            TCaptchaCheckResult result = future.GetValueSync();

            NJson::TJsonValue furyJson;
            UNIT_ASSERT(NJson::ReadJsonTree(furyInputRequest, &furyJson));
            auto jsPrint = furyJson["params"]["body"]["js_print"];
            UNIT_ASSERT_VALUES_EQUAL(jsPrint["valid_json"].GetBooleanSafe(), false);
            UNIT_ASSERT_VALUES_EQUAL(jsPrint["has_data"].GetBooleanSafe(), true);

            UNIT_ASSERT(!result.WasApiCaptchaError);
            UNIT_ASSERT(!result.WasFuryError);
            UNIT_ASSERT(result.ErrorMessages.size() == 0);
            UNIT_ASSERT(result.Success);
        }
        {
            auto req = CreateCaptchaRequestContext(env, "&rep=" + EncodeUriComponent(CorrectRep), "");
            TRequestContext rc{env, req.Release()};

            auto future = IsCaptchaGoodAsync(ValidKey, rc, cfg, TCaptchaSettingsPtr{});
            TCaptchaCheckResult result = future.GetValueSync();

            NJson::TJsonValue furyJson;
            UNIT_ASSERT(NJson::ReadJsonTree(furyInputRequest, &furyJson));
            auto jsPrint = furyJson["params"]["body"]["js_print"];
            UNIT_ASSERT_VALUES_EQUAL(jsPrint["valid_json"].GetBooleanSafe(), false);
            UNIT_ASSERT_VALUES_EQUAL(jsPrint["has_data"].GetBooleanSafe(), false);

            UNIT_ASSERT(!result.WasApiCaptchaError);
            UNIT_ASSERT(!result.WasFuryError);
            UNIT_ASSERT(result.ErrorMessages.size() == 0);
            UNIT_ASSERT(result.Success);
        }
    }

    Y_UNIT_TEST(RepFromGetOrPost) {
        TCloneCallback<TApiCaptchaTestReplier> apiCaptchaReplier(MakeHolder<TApiCaptchaTestReplier>());
        TTestServer captchaApiServer(apiCaptchaReplier);
        TString furyInputRequest;
        TCloneCallback<TFuryTestReplier> furyReplier(MakeHolder<TFuryTestReplier>(TDuration::Zero(), &furyInputRequest));
        TTestServer furyServer(furyReplier);

        auto& cfg = ANTIROBOT_DAEMON_CONFIG_MUTABLE;
        cfg.CaptchaApiHost = captchaApiServer.Host;
        cfg.CaptchaCheckTimeout = TDuration::Seconds(1);
        cfg.FuryEnabled = true;
        cfg.FuryHost = furyServer.Host;
        cfg.FuryBaseTimeout = TDuration::Seconds(1);

        TEnv env;
        for (bool fromGet : {false, true}) {
            TString jsPrintStr = "{}";
            auto req = fromGet
                ? CreateCaptchaRequestContext(env, "&rep=" + EncodeUriComponent(CorrectRep), "")
                : CreateCaptchaRequestContext(env, "", "&rep=" + EncodeUriComponent(CorrectRep));
            TRequestContext rc{env, req.Release()};

            auto future = IsCaptchaGoodAsync(ValidKey, rc, cfg, TCaptchaSettingsPtr{});
            TCaptchaCheckResult result = future.GetValueSync();

            NJson::TJsonValue furyJson;
            UNIT_ASSERT(NJson::ReadJsonTree(furyInputRequest, &furyJson));
            auto body = furyJson["params"]["body"];
            UNIT_ASSERT_VALUES_EQUAL(body["rep"].GetStringSafe(), CorrectRep);

            UNIT_ASSERT(!result.WasApiCaptchaError);
            UNIT_ASSERT(!result.WasFuryError);
            UNIT_ASSERT(result.ErrorMessages.size() == 0);
            UNIT_ASSERT(result.Success);
        }
    }
}
