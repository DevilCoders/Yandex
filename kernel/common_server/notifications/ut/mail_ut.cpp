#include "helpers.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <kernel/common_server/notifications/mail/mail.h>
#include <kernel/common_server/library/async_proxy/ut/helper/fixed_response_server.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <util/system/env.h>

namespace {
    TMailNotificationsConfig GetMailNotificationsConfig() {
        TStringStream ss;
        auto serverPort = Singleton<TPortManager>()->GetPort();
        ss << "NotificationType: mail" << Endl
           << "Host: localhost" << Endl
           << "Port: " << ToString(serverPort) << Endl
           << "IsHttps: false" << Endl
           << "Account: test.account" << Endl
           << "Token: test_token" << Endl;
        return IFrontendNotifierConfig::BuildFromString<TMailNotificationsConfig>(ss.Str());
    }
    TMailNotificationsConfig GetMailNotificationsAdditionalBccRecipientsConfig() {
        TStringStream ss;
        auto serverPort = Singleton<TPortManager>()->GetPort();
        ss << "NotificationType: mail" << Endl
           << "Host: localhost" << Endl
           << "Port: " << ToString(serverPort) << Endl
           << "IsHttps: false" << Endl
           << "Account: test.account" << Endl
           << "AdditionalBccRecipients: first@yandex-team.ru, second@yandex-team.ru" << Endl
           << "Token: test_token" << Endl;
        return IFrontendNotifierConfig::BuildFromString<TMailNotificationsConfig>(ss.Str());
    }

    NExternalAPI::TSenderConfig GetSenderConfig() {
        TStringBuilder sb;
        auto serverPort = Singleton<TPortManager>()->GetPort();
        sb << "ApiHost: localhost" << Endl
           << "ApiPort: " << ToString(serverPort) << Endl
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

Y_UNIT_TEST_SUITE(MailNotifications) {
    Y_UNIT_TEST(Smoke) {
        NJson::TJsonValue args = NJson::TJsonValue(NJson::JSON_MAP);
        args["name"] = "Robot Carsharing";
        args["template_text"] = "MailNotifications unittest";
        IFrontendNotifier::TMessage message("unittest.message",
                                            args.GetStringRobust());
        auto config = GetMailNotificationsConfig();
        auto senderConfig = GetSenderConfig();

        NCS::TExternalServicesOperator context;
        context.AddService("mail-backend", senderConfig);
        TNotifierHolder<TMailNotifier, TMailNotificationsConfig> notifier(config, context);
        auto pushServerMock = TFixedResponseServer::BuildAndRun(senderConfig.GetPort(), {
            {200, R"({"result" : {"status" : "OK", "task_id" : "id", "message_id" : "id"}})"}
        });
        auto resp = notifier.SendTestMessage(
                message,  {
                    TUserContacts().SetEmail("robot-carsharing@yandex-team.ru")
                });
        UNIT_ASSERT(resp);
        UNIT_ASSERT(!resp->HasErrors());
    }
    Y_UNIT_TEST(Multiple) {
        NJson::TJsonValue args = NJson::TJsonValue(NJson::JSON_MAP);
        args["name"] = "Robot Carsharing";
        args["template_text"] = "MailNotifications unittest";
        IFrontendNotifier::TMessage message("unittest.message",
                                            args.GetStringRobust());
        auto config = GetMailNotificationsConfig();
        auto senderConfig = GetSenderConfig();

        NCS::TExternalServicesOperator context;
        context.AddService("mail-backend", senderConfig);
        TNotifierHolder<TMailNotifier, TMailNotificationsConfig> notifier(config, context);
        auto pushServerMock = TFixedResponseServer::BuildAndRun(senderConfig.GetPort(), {{200, ""}, {400, ""}});
        auto resp = notifier.SendTestMessage(
                message, {
                    TUserContacts().SetEmail("robot-carsharing@yandex-team.ru"),
                    TUserContacts().SetEmail("wrong_email"),
                });
        UNIT_ASSERT(resp);
        UNIT_ASSERT(resp->HasErrors());
        UNIT_ASSERT_C(pushServerMock->GetCallsCount() == 2,
                      TStringBuilder() << "Actual calls count:" << pushServerMock->GetCallsCount());
    }
    Y_UNIT_TEST(AdditionalBccRecipients) {
        NJson::TJsonValue args = NJson::TJsonValue(NJson::JSON_MAP);
        args["name"] = "Robot Carsharing";
        args["template_text"] = "MailNotifications unittest";
        IFrontendNotifier::TMessage message("unittest.message",
                                            args.GetStringRobust());
        auto config = GetMailNotificationsAdditionalBccRecipientsConfig();
        auto senderConfig = GetSenderConfig();

        NCS::TExternalServicesOperator context;
        context.AddService("mail-backend", senderConfig);
        TNotifierHolder<TMailNotifier, TMailNotificationsConfig> notifier(config, context);
        auto pushServerMock = TFixedResponseServer::BuildAndRun(senderConfig.GetPort(), {{200, ""}, {400, ""}});
        auto resp = notifier.SendTestMessage(
                message, {
                    TUserContacts().SetEmail("robot-carsharing@yandex-team.ru"),
                    TUserContacts().SetEmail("wrong_email"),
                });
        UNIT_ASSERT(resp);
        UNIT_ASSERT(resp->HasErrors());
        UNIT_ASSERT_C(pushServerMock->GetCallsCount() == 2,
                      TStringBuilder() << "Actual calls count:" << pushServerMock->GetCallsCount());
    }
    Y_UNIT_TEST(ParseResponseDetails) {
        NJson::TJsonValue args = NJson::TJsonValue(NJson::JSON_MAP);
        args["name"] = "Robot Carsharing";
        args["template_text"] = "MailNotifications unittest";
        IFrontendNotifier::TMessage message("unittest.message",
                                            args.GetStringRobust());
        auto config = GetMailNotificationsConfig();
        auto senderConfig = GetSenderConfig();

        NCS::TExternalServicesOperator context;
        context.AddService("mail-backend", senderConfig);
        TNotifierHolder<TMailNotifier, TMailNotificationsConfig> notifier(config, context);
        auto pushServerMock = TFixedResponseServer::BuildAndRun(senderConfig.GetPort(), {
            {200, R"({"result" : {"status" : "OK", "task_id" : "id", "message_id" : "id"}})"},
            {200, R"({"result" : {"task_id" : "id", "message_id" : "id"}})"},
            {200, R"({"result" : {"status" : "OK", "message_id" : "id"}})"},
            {200, R"({"result" : {"status" : "OK", "task_id" : "id" }})"},
            {200, R"({})"},
            {200, ""}
        });
        TFLEventLog::TContextGuard logContext;
        auto resp = notifier.SendTestMessage(message, {
            TUserContacts().SetEmail("recipient.1@yandex-team.ru"),
            TUserContacts().SetEmail("recipient.2@yandex-team.ru"),
            TUserContacts().SetEmail("recipient.3@yandex-team.ru"),
            TUserContacts().SetEmail("recipient.4@yandex-team.ru"),
            TUserContacts().SetEmail("recipient.5@yandex-team.ru"),
            TUserContacts().SetEmail("recipient.6@yandex-team.ru")
        });
        UNIT_ASSERT(resp);
        UNIT_ASSERT(resp->HasErrors());

        int cntError = 0;
        auto records = logContext.GetJsonReport().GetArray();
        for (auto&& record: records) {
            if (record["text"] == "no_field" && record.Has("field")) {
                cntError++;
            }
        }
        UNIT_ASSERT_C(cntError == 5, TStringBuilder() << "Actual errors count:" << cntError);
        UNIT_ASSERT_C(pushServerMock->GetCallsCount() == 6, TStringBuilder() << "Actual calls count:" << pushServerMock->GetCallsCount());
    }
}
