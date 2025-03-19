#include "push.h"

#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/library/json/builder.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/util/network/neh.h>
#include <kernel/common_server/util/network/neh_request.h>

#include <kernel/reqid/reqid.h>

#include <library/cpp/mediator/global_notifications/system_status.h>

#include <util/stream/file.h>
#include <library/cpp/string_utils/quote/quote.h>

bool TPushNotifier::DoStart(const NCS::IExternalServicesOperator& /*context*/) {
    CHECK_WITH_LOG(!Agent);
    ReaskConfig.SetGlobalTimeout(TDuration::Seconds(10)).SetMaxAttempts(1);
    AD = MakeAtomicShared<TAsyncDelivery>();
    AD->Start(1, 8);
    Agent = MakeHolder<NNeh::THttpClient>(AD);
    Agent->RegisterSource("push", Config.GetHost(), Config.GetPort(), ReaskConfig, Config.GetIsHttps());
    return true;
}

IFrontendNotifier::TResult::TPtr TPushNotifier::DoNotify(const TMessage& message, const TContext& context) const {
    if (!context.GetRecipients()) {
        return MakeAtomicShared<TResult>("Empty recipients list");
    }

    const TRecipients& recipients = context.GetRecipients();

    bool hasErrors = false;
    for (size_t i = 0; i < recipients.size();) {
        const size_t endIndex = Min(recipients.size(), i + Config.GetPackSize());
        auto response = NotifyImpl(message, recipients.begin() + i,
                                   recipients.begin() + endIndex);
        hasErrors |= (response.Code() != 200);
        if (endIndex < recipients.size()) {
            Sleep(Config.GetPacksInterval());
        }
        i = endIndex;
    }
    return MakeAtomicShared<TResult>(hasErrors ? "Failed request" : "");
}

NUtil::THttpReply TPushNotifier::NotifyImpl(const TMessage& message,
                                            TRecipients::const_iterator recipientsBegin,
                                            TRecipients::const_iterator recipientsEnd) const {
    CHECK_WITH_LOG(!!Agent);
    TString reqid = ReqIdGenerate("DRIVE");
    NNeh::THttpRequest request;
    request.SetCgiData("token=" + Config.GetAuthToken() + "&event=" + Config.GetEventType() + "&reqid=" + reqid);

    NJson::TJsonValue jsonPost;
    NJson::TJsonValue& recipientsJson = jsonPost.InsertValue("recipients", NJson::JSON_ARRAY);
    for (auto r = recipientsBegin; r != recipientsEnd; ++r) {
        recipientsJson.AppendValue(r->GetPassportUid());
    }
    NJson::TJsonValue& payload = jsonPost.InsertValue("payload", NJson::JSON_MAP);
    if (message.HasAdditionalInfo()) {
        payload.InsertValue("details", message.GetAdditionalInfo());
    }
    payload["body"] = message.GetBody();
    payload["sound"] = "default";
    payload["icon"] = "push";
    payload["title"] = "";

    NJson::TJsonValue& repack = jsonPost.InsertValue("repack", NJson::JSON_MAP);
    {
        NJson::TJsonValue& aps = repack.InsertValue("apns", NJson::JSON_MAP).InsertValue("aps", NJson::JSON_MAP);
        aps["sound"] = "default";
        aps["badge"] = 1;
        aps["alert"] = message.GetBody();
    }

    {
        NJson::TJsonValue& fcm = repack.InsertValue("fcm", NJson::JSON_MAP);
        fcm["priority"] = "high";
        repack.InsertValue("hms", fcm);
    }

    if (!Config.GetAppNames().empty()) {
        NJson::TJsonValue& subscriptions = jsonPost.InsertValue("subscriptions", NJson::JSON_ARRAY);
        NJson::TJsonValue appsArray = NJson::JSON_ARRAY;
        for (auto&& element : Config.GetAppNames()) {
            appsArray.AppendValue(element);
        }
        NJson::TJsonValue appFilter;
        appFilter["app"] = std::move(appsArray);
        subscriptions.AppendValue(std::move(appFilter));
    }

    request.SetUri("/" + Config.GetUri()).SetRequestType("POST").SetPostData(jsonPost.GetStringRobust());
    TFLEventLog::ModuleLog("PushRequest", TLOG_DEBUG)("data", NJson::TMapBuilder
        ("host", Config.GetHost())
        ("event_type", Config.GetEventType())
        ("reqid", reqid)
        ("data", jsonPost)
    );
    if (Config.GetDebugMode()) {
        TFLEventLog::Log(request.GetDebugRequest(), TLOG_INFO);
    }
    auto tgResult = Agent->SendMessageSync(request, Now() + TDuration::Seconds(10));
    TFLEventLog::ModuleLog("PushResponse", TLOG_DEBUG)("data", NJson::TMapBuilder
        ("host", Config.GetHost())
        ("event_type", Config.GetEventType())
        ("reqid", reqid)
        ("code", tgResult.Code())
        ("data", tgResult.Content())
    );
    if (tgResult.Code() == 200) {
        TCSSignals::SignalAdd("frontend-push", "success", 1);
    } else {
        TCSSignals::SignalAdd("frontend-push", "fail", 1);
    }
    TCSSignals::SignalAdd("frontend-push", "codes-" + ::ToString(tgResult.Code()), 1);
    TCSSignals::SignalAdd("frontend-push-" + NotifierName, "codes-" + ::ToString(tgResult.Code()), 1);
    if (tgResult.Code() != 200) {
        TFLEventLog::Log("Push failure",TLOG_ERR)
            ("request", request.GetDebugRequest())
            ("code", tgResult.Code())
            ("content", tgResult.Content())
            ("error message", tgResult.ErrorMessage());
    }
    return tgResult;
}

void TPushNotifier::SubscribeUser(const TString& appName, const TString& platform, const TString& passportUid, const TString& uuid, const TString& pushToken, THolder<NNeh::THttpAsyncReport::ICallback>&& callback) const {
    CHECK_WITH_LOG(!!Agent);
    NNeh::THttpRequest request;

    NJson::TJsonValue postData;
    postData["push_token"] = pushToken;

    request.SetUri("/" + Config.GetSubscriptionUri()).SetRequestType("POST").SetPostData(postData.GetStringRobust());
    request.SetCgiData("token=" + Config.GetSubscriptionToken() + "&service=" + Config.GetServiceName() + "&app_name=" + appName + "&platform=" + platform + "&user=" + passportUid + "&uuid=" + uuid + "&push_token=" + pushToken);

    Agent->Send(request, Now() + TDuration::Seconds(10), std::move(callback));
}

TPushNotifier::TPushNotifier(const TPushNotificationsConfig& config)
    : IFrontendNotifier(config)
    , Config(config)
{
}

TPushNotifier::~TPushNotifier() {
}

bool TPushNotifier::DoStop() {
    if (!!AD) {
        AD->Stop();
    }
    return true;
}

TPushNotificationsConfig::TFactory::TRegistrator<TPushNotificationsConfig> TPushNotificationsConfig::Registrator("push");


void TPushNotificationsConfig::DoInit(const TYandexConfig::Section* section) {
    Host = section->GetDirectives().Value("Host", Host);
    AssertCorrectConfig(!!Host, "Incorrect host in TPushNotificationsConfig");
    Port = section->GetDirectives().Value<ui16>("Port", Port);
    IsHttps = section->GetDirectives().Value<bool>("IsHttps", IsHttps);
    Uri = section->GetDirectives().Value("Uri", Uri);
    DebugMode = section->GetDirectives().Value("DebugMode", DebugMode);
    AssertCorrectConfig(!!Uri, "Incorrect uri in TPushNotificationsConfig");

    EventType = section->GetDirectives().Value("EventType", EventType);

    TokenPath = section->GetDirectives().Value("TokenPath", TokenPath);
    if (TokenPath) {
        AssertCorrectConfig(TokenPath.Exists(), "Can't read token info from " + TokenPath.GetPath());
        AuthToken = Strip(TFileInput(TokenPath).ReadAll());
    } else {
        AuthToken = section->GetDirectives().Value<TString>("AuthToken");
    }
    AssertCorrectConfig(!!AuthToken, "no 'AuthToken'");

    SubscriptionTokenPath = section->GetDirectives().Value("SubscriptionTokenPath", SubscriptionTokenPath);
    if (SubscriptionTokenPath.Exists()) {
        SubscriptionToken = Strip(TFileInput(SubscriptionTokenPath).ReadAll());
    }

    PackSize = section->GetDirectives().Value("PackSize", PackSize);
    PacksInterval = section->GetDirectives().Value("PacksInterval", PacksInterval);
}

IFrontendNotifier::TPtr TPushNotificationsConfig::Construct() const {
    return MakeAtomicShared<TPushNotifier>(*this);
}
