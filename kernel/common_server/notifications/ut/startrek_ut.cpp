#include "helpers.h"

#include <util/string/builder.h>
#include <util/system/env.h>
#include <util/generic/vector.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <kernel/common_server/startrek/issue_requests.h>
#include <kernel/common_server/startrek/comment_requests.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/notifications/abstract/abstract.h>
#include <kernel/common_server/notifications/startrek/startrek.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/library/metasearch/simple/config.h>
#include <kernel/common_server/library/async_proxy/ut/helper/fixed_response_server.h>

namespace {

    NExternalAPI::TSenderConfig GetSenderConfig() {
        TStringBuilder sb;
        auto serverPort = Singleton<TPortManager>()->GetPort();
        sb << "ApiHost: localhost" << Endl
           << "ApiPort: " << serverPort << Endl
           << "Https: false" << Endl
               << "<Headers>" << Endl
               << "Authorization: OAuth TOKEN" << Endl
               << "</Headers>" << Endl
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

    TStartrekNotificationsConfig GetStartrekNotificationsConfig() {
        TStringStream ss;
        ss << "NotificationType: startrek" << Endl
           << "Queue: TESTQUEUE" << Endl;
        return IFrontendNotifierConfig::BuildFromString<TStartrekNotificationsConfig>(ss.Str());
    }
}


Y_UNIT_TEST_SUITE(StartrekClient) {
    Y_UNIT_TEST(AddCommentClient) {
        auto config = GetStartrekNotificationsConfig();
        auto senderConfig = GetSenderConfig();

        TString comment = "New test comment " + TInstant::Now().ToString();
        TString issue = "TESTQUEUE-4";

        NCS::TExternalServicesOperator context;
        context.AddService("startrek-backend", senderConfig);

        auto notifier = TStartrekNotifier(config);
        notifier.Start(context);
        auto pushServerMock = TFixedResponseServer::BuildAndRun(senderConfig.GetPort(),
                                                                {{200, R"({"self" : "selfurl"})"}});
        IFrontendNotifier::TContext cont;
        auto resp = notifier.AddComment(issue, comment);
        notifier.Stop();
        UNIT_ASSERT(resp);
    }

    Y_UNIT_TEST(CreateIssue) {
        auto config = GetStartrekNotificationsConfig();
        auto senderConfig = GetSenderConfig();
        IFrontendNotifier::TMessage message("Another very new lovely issue", "super description");

        NCS::TExternalServicesOperator context;
        context.AddService("startrek-backend", senderConfig);

        TNotifierHolder<TStartrekNotifier, TStartrekNotificationsConfig> notifier(config, context);
        auto pushServerMock = TFixedResponseServer::BuildAndRun(senderConfig.GetPort(),
                                                                {{200, R"({"self" : "selfurl"})"}});
        IFrontendNotifier::TContext cont;
        auto resp = notifier.SendTestMessage(message, cont);
        UNIT_ASSERT(resp);
        UNIT_ASSERT(!resp->HasErrors());
    }

    Y_UNIT_TEST(AddCommentRequest) {
        auto senderConfig = GetSenderConfig();
        NCS::TExternalServicesOperator context;
        context.AddService("startrek-backend", senderConfig);
        auto Sender = context.GetSenderPtr("startrek-backend");
        auto pushServerMock = TFixedResponseServer::BuildAndRun(senderConfig.GetPort(),
                                                                {{200, R"({"self" : "selfurl"})"}});
        auto commentRequest = NCS::NStartrek::TAddCommentStartrekRequest("TESTQUEUE-3", "TESTTEXT");
        auto resp = Sender->SendRequest(commentRequest);
        UNIT_ASSERT(resp.IsSuccess());
    }

    Y_UNIT_TEST(CreateIssueRequest) {
        auto senderConfig = GetSenderConfig();
        NCS::TExternalServicesOperator context;
        context.AddService("startrek-backend", senderConfig);
        auto Sender = context.GetSenderPtr("startrek-backend");
        auto pushServerMock = TFixedResponseServer::BuildAndRun(senderConfig.GetPort(),
                                                                {{200, R"({"self" : "selfurl"})"}});
        auto issueRequest = NCS::NStartrek::TNewIssueStartrekRequest("SUMMARY", "TESTQUEUE", "this is description");
        issueRequest.SetType("bug");
        auto resp = Sender->SendRequest(issueRequest);
        UNIT_ASSERT(resp.IsSuccess());
    }
}
