#include "environment.h"
#include "rule_set.h"
#include "fullreq_info.h"
#include "match_rule_parser.h"
#include "request_params.h"
#include "user_reply.h"

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <antirobot/daemon_lib/ut/utils.h>
#include <antirobot/lib/keyring.h>
#include <antirobot/lib/spravka_key.h>

using namespace NAntiRobot;

void ReloadConfig() {
    TJsonConfigGenerator jsonConf;
    ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(TString{} +
                                                   "<Daemon>\n"
                                                   "CaptchaApiHost = ::\n"
                                                   "CbbApiHost = ::\n"
                                                   "CbbEnabled = 0\n"
                                                   "UseTVMClient = 0\n"
                                                   "GeodataBinPath = ./geodata6-xurma.bin\n"
                                                   "FormulasDir = .\n"
                                                   "JsonConfFilePath = " + ArcadiaSourceRoot() + "/antirobot/config/service_config.json\n" +
                                                   "PartnerCaptchaType = 1\n" +
                                                   "ThreadPoolParams = free_min=0; free_max=0; total_max=0; increase=1\n"
                                                   "HypocrisyBundlePath =\n"
                                                   "</Daemon>\n"
                                                   "<Zone>\n"
                                                   "</Zone>\n"
                                                   "<WizardsRemote>\n"
                                                   "RemoteWizards :::8891\n"
                                                   "</WizardsRemote>\n");
    std::array<std::vector<std::pair<TString, TString>>, EHostType::HOST_NUMTYPES> queries;
    queries[EHostType::HOST_WEB].push_back({"/search", "ys"});
    jsonConf.SetReQueries(queries);
    ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), GetJsonServiceIdentifierStr());
}

THolder<TRequest> CreateSearchRequestContext(TEnv& env, const TString& ip, const TString& cookie = "") {
    const TString get = TString{} +
                        "GET /search?text=cats HTTP/1.1\r\n"
                        "X-Forwarded-For-Y: " + ip + "\r\n"
                        "Host: yandex.ru\r\n" +
                        (cookie.Empty() ? "" : "Cookie: " + cookie + "\r\n") +
                        "\r\n";

    TStringInput stringInput{get};
    THttpInput input{&stringInput};

    return MakeHolder<TFullReqInfo>(
        input,
        "", "0.0.0.0",
        env.ReloadableData,
        TPanicFlags::CreateFake(),
        GetEmptySpravkaIgnorePredicate(),
        nullptr,
        &env
    );
}

TString GetResponseData(const TResponse& response) {
    TString responseInText;
    TStringOutput stringOutput(responseInText);
    stringOutput << response;
    return responseInText;
}

bool BannedRequest(TEnv& env, const TString& ip, const TString& cookie = "") {
    auto tmpReq = CreateSearchRequestContext(env, ip, cookie);
    TRequestContext rc(env, tmpReq.Release());

    auto response = HandleGeneralRequest(rc).GetValueSync();
    TString responseInText = GetResponseData(response);
    return !responseInText.StartsWith("HTTP/1.1 200 ");
}

class TTestUserReplyParams: public TTestBase {
public:
    void SetUp() override {
        SetupTestData();
        ReloadConfig();
    }
};

Y_UNIT_TEST_SUITE_IMPL(TTestUserReply, TTestUserReplyParams) {
    Y_UNIT_TEST(TestNotBannedRequest) {
        TEnv env;
        TString regularIp = "1.1.1.1";
        TString bannedIp = "2.2.2.2";
        TString regExp = "header['X-Forwarded-For-Y']=/.*2.2.2.2.*/i";
        const TVector parsedRules = {TPreparedRule::Parse(regExp)};
        env.Robots->AddManual(EHostType::HOST_WEB, TUid::FromAddr(TAddr(bannedIp)), TInstant::Max(), true, {}, {});
        Sleep(TDuration::MilliSeconds(100));

        env.CbbGroupIdToProperties[ANTIROBOT_DAEMON_CONFIG.CbbFlagNotBanned]
            .push_back(&TRequestContext::TMatchedRules::NotBanned);

        for (const auto& rule : parsedRules) {
            env.FastRuleSet.Add(ANTIROBOT_DAEMON_CONFIG.CbbFlagNotBanned, rule);
        }

        UNIT_ASSERT_EQUAL(BannedRequest(env, bannedIp), false);
        UNIT_ASSERT_EQUAL(BannedRequest(env, regularIp), false);
    }

    Y_UNIT_TEST(TestCaptchaBlockerWithSpravka) {
        TEnv env;

        const TString spravkaKey = "4a1faf3281028650e82996f37aada9d97ef7c7c4f3c71914a7cc07a1cbb02d00";
        TStringInput spravkaSI(spravkaKey);
        TSpravkaKey::SetInstance(TSpravkaKey(spravkaSI));

        const TString testKey = "102c46d700bed5c69ed20b7473886468";
        TStringInput keys(testKey);
        TKeyRing::SetInstance(TKeyRing(keys));

        TString regularIp = "1.1.1.1";
        TString bannedIp = "2.2.2.2";
        TString regExp = "header['X-Forwarded-For-Y']=/.*2.2.2.2.*/i";
        const TVector parsedRules = {TPreparedRule::Parse(regExp)};
        env.Robots->AddManual(EHostType::HOST_WEB, TUid::FromAddr(TAddr(bannedIp)), TInstant::Max(), true, {}, {});
        Sleep(TDuration::MilliSeconds(100));

        for (const auto& rule : parsedRules) {
            env.ServiceToFastRuleSet.GetByService(HOST_WEB).Add(TCbbGroupId{262}, rule);
        }

        UNIT_ASSERT_EQUAL(BannedRequest(env, bannedIp), true);
        UNIT_ASSERT_EQUAL(BannedRequest(env, bannedIp, "spravka=dD0xMzczOTAzODIyO2k9MTI3LjAuMC4xO3U9MTM3MzkwMzgyMjIyNDM0MzA2MTtoPTZjZmYxN2JlM2E3MThiN2UzNDJmOTVlMDI2NzU2MjI0"), false);
        UNIT_ASSERT_EQUAL(BannedRequest(env, regularIp), false);
    }

}
