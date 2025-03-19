#include "helpers.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <kernel/common_server/notifications/push/push.h>
#include <kernel/common_server/library/async_proxy/ut/helper/fixed_response_server.h>
#include <util/system/env.h>

namespace {
    const ui32 PACK_SIZE = 2;

    TPushNotificationsConfig GetPushNotoficationsConfig() {
        TStringStream ss;
        auto serverPort = Singleton<TPortManager>()->GetPort();
        ss << "NotificationType: push" << Endl
           << "Host: localhost" << Endl
           << "Port: " << ToString(serverPort) << Endl
           << "IsHttps: false" << Endl
           << "AuthToken: test_token" << Endl
           << "PackSize: " << PACK_SIZE << Endl
           << "PacksInterval: " << 0 << Endl;
        return IFrontendNotifierConfig::BuildFromString<TPushNotificationsConfig>(ss.Str());
    }
}

Y_UNIT_TEST_SUITE(PushNotifications) {
    Y_UNIT_TEST(Smoke) {
        const IFrontendNotifier::TMessage message("test push");
        const size_t recipientsCount = 5;
        IFrontendNotifier::TRecipients recipients;
        for (size_t i = 0; i < recipientsCount; ++i) {
            recipients.push_back(TUserContacts().SetPassportUid(ToString(i)));
        }

        auto config = GetPushNotoficationsConfig();
        TNotifierHolder<TPushNotifier, TPushNotificationsConfig> notifier(config);
        auto pushServerMock = TFixedResponseServer::BuildAndRun(
                config.GetPort(), {{200, ""}, {400, ""}, {404, ""}});
        auto resp = notifier.SendTestMessage(message, recipients);

        UNIT_ASSERT(resp);
        UNIT_ASSERT(resp->HasErrors());
        UNIT_ASSERT_C(pushServerMock->GetCallsCount() == 3,
                      TStringBuilder() << "Actual calls count:" << pushServerMock->GetCallsCount());
    }
}
