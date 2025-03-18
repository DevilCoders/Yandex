#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include "environment.h"
#include "rule_set.h"
#include "fullreq_info.h"
#include "match_rule_parser.h"
#include "request_features.h"
#include "robot_detector.h"

#include <antirobot/daemon_lib/ut/utils.h>

using namespace NAntiRobot;

namespace {
    TString regularIp = "188.186.202.78";
    TString whitelistedIp = "2.2.2.2";

    THolder<IRobotDetector> CreateDetector(TEnv& env) {
        return THolder(CreateRobotDetector(env, env.IsBlocked.GetArray(), *env.Blocker.Get(), env.CbbIO.Get()));
    }

    THolder<TRequest> CreateOtherRequestContext(TEnv& env, const TString& ip = regularIp) {
        const TString get = TString{} +
                            "GET /not_search_at_all HTTP/1.1\r\n"
                            "User-Agent: Opera/9.80 (Windows NT 5.1) Presto/2.12.388 Version/12.11\r\n"
                            "Host: yandex.ru\r\n"
                            "Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, */*;q=0.1\r\n"
                            "Accept-Language: ru-RU,ru;q=0.9,en;q=0.8\r\n"
                            "Accept-Encoding: gzip, deflate\r\n"
                            "Referer: http://yandex.ru/yandsearch?p=2&text=nude+%22Dany+Carrel%22&clid=47639&lr=48\r\n"
                            "Cookie: yandexuid=2341234123412341; \r\n"
                            "Connection: Keep-Alive\r\n" +
                            "X-Forwarded-For-Y: " + ip + "\r\n" +
                            "X-Source-Port-Y: 54542\r\n"
                            "X-Start-Time: 1354193658054839\r\n"
                            "X-Req-Id: 1354193658054839-6563412409256195394\r\n"
                            "\r\n";

        TStringInput stringInput{get};
        THttpInput input{&stringInput};

        return MakeHolder<TFullReqInfo>(input, "", "0.0.0.0", env.ReloadableData, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
    }

    THolder<TRequest> CreateSearchRequestContext(TEnv& env, const TString& ip = regularIp, const TString& cookie = "") {
        const TString get = TString{} +
                            "GET /search?text=cats HTTP/1.1\r\n"
                            "User-Agent: Opera/9.80 (Windows NT 5.1) Presto/2.12.388 Version/12.11\r\n"
                            "Host: yandex.ru\r\n"
                            "Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, */*;q=0.1\r\n"
                            "Accept-Language: ru-RU,ru;q=0.9,en;q=0.8\r\n"
                            "Accept-Encoding: gzip, deflate\r\n"
                            "Referer: http://yandex.ru/yandsearch?p=2&text=nude+%22Dany+Carrel%22&clid=47639&lr=48\r\n"
                            "Cookie: yandexuid=345234523456456;" + cookie + "\r\n"
                            "Connection: Keep-Alive\r\n" +
                            "X-Forwarded-For-Y: " + ip + "\r\n" +
                            "X-Source-Port-Y: 54542\r\n"
                            "X-Start-Time: 1354193658054839\r\n"
                            "X-Req-Id: 1354193658054839-6563412409256195394\r\n"
                            "\r\n";

        TStringInput stringInput{get};
        THttpInput input{&stringInput};

        return MakeHolder<TFullReqInfo>(
            input, "", "0.0.0.0",
            env.ReloadableData,
            TPanicFlags::CreateFake(),
            GetEmptySpravkaIgnorePredicate(),
            nullptr,
            &env
        );
    }

    void ReloadConfig() {
        TJsonConfigGenerator jsonConf;
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(TString{} +
                                                       "<Daemon>\n"
                                                       "AuthorizeByFuid = 1\n"
                                                       "AuthorizeByICookie = 1\n"
                                                       "AuthorizeByLCookie = 1\n"
                                                       "CaptchaApiHost = ::\n"
                                                       "CbbApiHost = ::\n"
                                                       "CbbEnabled = 0\n"
                                                       "UseTVMClient = 0\n"
                                                       "MinRequestsWithSpravka = 20\n"
                                                       "GeodataBinPath = ./geodata6-xurma.bin\n"
                                                       "FormulasDir = .\n"
                                                       "PartnerCaptchaType = 1\n"
                                                       "ThreadPoolParams = free_min=0; free_max=0; total_max=0; increase=1\n"
                                                       "WhiteList = ./whitelist_ips\n"
                                                       "HypocrisyBundlePath =\n"
                                                       "</Daemon>\n"
                                                       "<Zone>\n"
                                                       "</Zone>\n"
                                                       "<WizardsRemote>\n"
                                                       "RemoteWizards :::8891\n"
                                                       "</WizardsRemote>\n");
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), GetJsonServiceIdentifierStr());
    }

    THolder<TEnv> CreateEnv() {
        ReloadConfig();

        THolder<TEnv> env = MakeHolder<TEnv>();
        env->ReloadableData.RequestClassifier.Set(
            CreateClassifierForTests()
        );

        return env;
    }

    void Ban(TEnv& env, TRequest* req) {
        env.Robots->AddManual(
            req->HostType, TUid::FromAddr(req->UserAddr),
            TInstant::Max(), true, {}, {}
        );
    }

    bool IsRobot(TEnv& env, const TString& ip) {
        auto tmpReq = CreateSearchRequestContext(env, ip);
        tmpReq->Uid.Ns = TUid::SPRAVKA;

        TRequestContext rc(env, tmpReq.Release());
        const TRequest* req = rc.Req.Get();

        TRpsFilter fakeFilter(0, TDuration::Zero(), TDuration::Zero());

        TRequestFeatures::TContext ctx = {
            req,
            nullptr,
            &env.ReloadableData,
            env.Detector.Get(),
            env.AutoruOfferDetector.Get(),
            fakeFilter
        };

        TTimeStatInfoVector emptyVector = {};
        TTimeStats fakeStats{emptyVector, ""};

        TRequestFeatures fakeRequestFeatures(ctx, fakeStats, fakeStats);
        size_t requestsFromUid = 1;
        bool isAlreadyRobot = false;

        return GetRobotInfo(rc, fakeRequestFeatures, false, true, false, requestsFromUid, isAlreadyRobot).IsRobot;
    }
}

class TRobotDetectorParams: public TTestBase {
public:
    void SetUp() override {
        SetupTestData();
        TFileOutput("./whitelist_ips").Write(whitelistedIp);
        ReloadConfig();
    }
};

Y_UNIT_TEST_SUITE_IMPL(TRobotDetector, TRobotDetectorParams) {
    Y_UNIT_TEST(TestGetRobotStatus) {
        const auto env = CreateEnv();
        const auto detector = CreateDetector(*env);

        // ForceCaptcha
        {
            auto req = CreateSearchRequestContext(*env);
            req->ForceShowCaptcha = true;
            TRequestContext rc{*env, req.Release()};

            auto status = detector->GetRobotStatus(rc);
            UNIT_ASSERT(status.IsCaptcha());
        }

        // Robot
        // Robot
        {
            auto req = CreateSearchRequestContext(*env, regularIp);
            Ban(*env, req.Get());
            TRequestContext rc{*env, req.Release()};

            auto status = detector->GetRobotStatus(rc);

            UNIT_ASSERT(status.IsCaptcha());

            env->Robots->Clear();
        }

        // Whitelisted
        {
            auto req = CreateSearchRequestContext(*env, whitelistedIp);
            Ban(*env, req.Get());
            TRequestContext rc{*env, req.Release()};

            auto status = detector->GetRobotStatus(rc);

            UNIT_ASSERT(!status.IsCaptcha());

            env->Robots->Clear();
        }

        // Disable whitelisted with cookie
        {
            auto req = CreateSearchRequestContext(*env, whitelistedIp, " gBQeYpMlWFqFExSpsuB8j89vDDlf7HaT=1;");
            Ban(*env, req.Get());
            TRequestContext rc{*env, req.Release()};

            auto status = detector->GetRobotStatus(rc);
            UNIT_ASSERT(status.IsCaptcha());

            env->Robots->Clear();
        }

        // Cannot show captcha
        {
            auto req = CreateOtherRequestContext(*env, regularIp);
            Ban(*env, req.Get());
            TRequestContext rc{*env, req.Release()};

            auto status = detector->GetRobotStatus(rc);

            UNIT_ASSERT(!status.IsCaptcha());

            env->Robots->Clear();
        }
    }

    Y_UNIT_TEST(TestIgnoreSpravka) {
        const auto env = CreateEnv();

        TString bannedIp = "3.3.3.3";
        TString regExp = "header['X-Forwarded-For-Y']=/.*3.3.3.3.*/i";
        const TVector parsedRules = {TPreparedRule::Parse(regExp)};

        env->CbbGroupIdToProperties[ANTIROBOT_DAEMON_CONFIG.CbbFlagIgnoreSpravka]
            .push_back(&TRequestContext::TMatchedRules::IgnoreSpravka);

        for (const auto& rule : parsedRules) {
            env->FastRuleSet.Add(ANTIROBOT_DAEMON_CONFIG.CbbFlagIgnoreSpravka, rule);
        }

        auto detector = CreateDetector(*env);
        UNIT_ASSERT_EQUAL(IsRobot(*env, bannedIp), true);
        UNIT_ASSERT_EQUAL(IsRobot(*env, regularIp), false);
    }
}
