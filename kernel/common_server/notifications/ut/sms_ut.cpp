#include "helpers.h"

#include <kernel/common_server/notifications/sms/sms.h>
#include <kernel/common_server/notifications/sms/request.h>
#include <kernel/common_server/library/async_proxy/ut/helper/fixed_response_server.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/system/env.h>

namespace {
    const TString YASMS_OK_RESPONSE =
            TStringBuilder()
            << "<?xml version=\"1.0\" encoding=\"windows-1251\"?>" << Endl
            << "<doc>" << Endl
            << " <message-sent id=\"127000000003456\" />" << Endl
            << "</doc>";

    const TString YASMS_ERROR_RESPONSE =
            TStringBuilder()
             << "<?xml version=\"1.0\" encoding=\"windows-1251\"?>" << Endl
             << "<doc>" << Endl
             << "<error>User does not have an active phone to recieve messages</error>" << Endl
             << "<errorcode>NOCURRENT</errorcode>" << Endl
             << "</doc>";

    TSMSNotificationsConfig GetSMSNotificationsConfig() {
        TStringStream ss;
        ss << "NotificationType: sms" << Endl
           << "Sender: Yandex.Drive" << Endl
           << "Route: drive" << Endl;
        return IFrontendNotifierConfig::BuildFromString<TSMSNotificationsConfig>(ss.Str());
    }

    TSMSNotificationsConfig GetSMSNotificationsConfigWithWhiteList() {
        TStringStream ss;
        ss << "NotificationType: sms" << Endl
           << "Sender: Yandex.Drive" << Endl
           << "Route: drive" << Endl
           << "WhiteList: +71112223344, +75556667788" << Endl;
        return IFrontendNotifierConfig::BuildFromString<TSMSNotificationsConfig>(ss.Str());
    }

    NExternalAPI::TSenderConfig GetSenderConfig() {
        TStringBuilder sb;
        auto serverPort = Singleton<TPortManager>()->GetPort();
        sb << "ApiHost: localhost" << Endl
           << "ApiPort: " << serverPort << Endl
           << "Https: 0" << Endl
           << "<RequestConfig>" << Endl
           << "  GlobalTimeout: 1000" << Endl
           << "  TimeoutSendingms: 100" << Endl
           << "  TimeoutConnectms: 100" << Endl
           << "  MaxAttempts: 2" << Endl
           << "  TasksCheckIntervalms: 30" << Endl
           << "</RequestConfig>" << Endl;

        NExternalAPI::TSenderConfig result;
        TAnyYandexConfig config;
        CHECK_WITH_LOG(config.ParseMemory(sb));
        result.Init(config.GetRootSection(), {});

        return result;
    }
}

Y_UNIT_TEST_SUITE(SMSNotifications) {
    Y_UNIT_TEST(CheckNoDoublingParamsInRequest) {
        TTextTemplateParams params;

        TSmsRequest smsRequest("Sender", "Text Text Text");
        smsRequest
            .SetUID("Uid1").SetUID("Uid2")
            .SetPhone("+79876543210").SetPhone("+79012345678")
            .SetPhoneId("PhoneId1").SetPhoneId("PhoneId2")
            .SetRoute("Route1").SetRoute("Route2")
            .SetUdh("Udh1").SetUdh("Udh2")
            .SetIdentity("Identity1").SetIdentity("Identity2")
            .MutableTextTemplateParams().SetParams({{"key1", "value1"},{"key2", "value2"}});
        NNeh::THttpRequest httpRequest;
        smsRequest.BuildHttpRequest(httpRequest);

        auto data = httpRequest.GetCgiData(false);
        TCgiParameters cgi(data);
        UNIT_ASSERT_EQUAL(cgi.NumOfValues("sender"), 1);
        UNIT_ASSERT_EQUAL(cgi.NumOfValues("text"), 1);
        UNIT_ASSERT_EQUAL(cgi.NumOfValues("uid"), 1);
        UNIT_ASSERT_EQUAL(cgi.NumOfValues("phone_id"), 1);
        UNIT_ASSERT_EQUAL(cgi.NumOfValues("route"), 1);
        UNIT_ASSERT_EQUAL(cgi.NumOfValues("udh"), 1);
        UNIT_ASSERT_EQUAL(cgi.NumOfValues("identity"), 1);
        UNIT_ASSERT_EQUAL(cgi.NumOfValues("text_template_params"), 1);
    }

    Y_UNIT_TEST(SmokeOk) {
        auto config = GetSMSNotificationsConfig();
        auto senderConfig = GetSenderConfig();

        NCS::TExternalServicesOperator context;
        context.AddService("sms-backend", senderConfig);
        TNotifierHolder<TSMSNotifier, TSMSNotificationsConfig> notifier(config, context);
        auto pushServerMock = TFixedResponseServer::BuildAndRun(senderConfig.GetPort(),
            {{200, YASMS_OK_RESPONSE}});
        auto resp = notifier.SendTestMessage(
            IFrontendNotifier::TMessage("Test"),
            {TUserContacts().SetPhone("+71234567890").SetPassportUid("test_uid")
        });
        UNIT_ASSERT(resp);
        UNIT_ASSERT(!resp->HasErrors());
    }

    Y_UNIT_TEST(SmokeError) {
        auto config = GetSMSNotificationsConfig();
        auto senderConfig = GetSenderConfig();

        NCS::TExternalServicesOperator context;
        context.AddService("sms-backend", senderConfig);
        TNotifierHolder<TSMSNotifier, TSMSNotificationsConfig> notifier(config, context);
        auto pushServerMock = TFixedResponseServer::BuildAndRun(senderConfig.GetPort(), {{200, YASMS_ERROR_RESPONSE}});
        auto resp = notifier.SendTestMessage(
                IFrontendNotifier::TMessage("Test"),
                {TUserContacts().SetPhone("+71234567890").SetPassportUid("test_uid")});
        UNIT_ASSERT(resp);
        UNIT_ASSERT(resp->HasErrors());
    }

    Y_UNIT_TEST(MultipleResponse) {
        auto config = GetSMSNotificationsConfig();
        auto senderConfig = GetSenderConfig();

        NCS::TExternalServicesOperator context;
        context.AddService("sms-backend", senderConfig);
        TNotifierHolder<TSMSNotifier, TSMSNotificationsConfig> notifier(config, context);
        auto pushServerMock = TFixedResponseServer::BuildAndRun(senderConfig.GetPort(),
                {{200, YASMS_ERROR_RESPONSE}, {200, YASMS_OK_RESPONSE}, {500, ""}});
        auto resp = notifier.SendTestMessage(
                IFrontendNotifier::TMessage("Test"),
                {TUserContacts().SetPhone("+71234567890").SetPassportUid("test1_uid"),
                 TUserContacts().SetPhone("+71234567891").SetPassportUid("test2_uid"),
                 TUserContacts().SetPhone("+71234567892").SetPassportUid("test3_uid")});
        UNIT_ASSERT(resp);
        UNIT_ASSERT(resp->HasErrors());
        UNIT_ASSERT_EQUAL(pushServerMock->GetCallsCount(), 3);
    }

    Y_UNIT_TEST(WhiteList) {
        auto config = GetSMSNotificationsConfigWithWhiteList();
        auto senderConfig = GetSenderConfig();

        NCS::TExternalServicesOperator context;
        context.AddService("sms-backend", senderConfig);
        TNotifierHolder<TSMSNotifier, TSMSNotificationsConfig> notifier(config, context);
        auto pushServerMock = TFixedResponseServer::BuildAndRun(senderConfig.GetPort(),
                {{200, YASMS_ERROR_RESPONSE}, {200, YASMS_OK_RESPONSE}, {500, ""}});
        auto resp = notifier.SendTestMessage(
                IFrontendNotifier::TMessage("Test"),
                {TUserContacts().SetPhone("+71112223344").SetPassportUid("test1_uid"),
                 TUserContacts().SetPhone("+71234567891").SetPassportUid("test2_uid"),
                 TUserContacts().SetPhone("+75556667788").SetPassportUid("test3_uid")});
        UNIT_ASSERT(resp);
        UNIT_ASSERT(resp->HasErrors());
        UNIT_ASSERT_EQUAL(pushServerMock->GetCallsCount(), 2);
    }
}

