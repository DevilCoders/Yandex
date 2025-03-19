#pragma once

#include <util/generic/ptr.h>
#include <library/cpp/yconf/conf.h>
#include <util/stream/output.h>
#include <kernel/common_server/notifications/abstract/abstract.h>
#include <kernel/common_server/library/metasearch/simple/config.h>
#include <kernel/common_server/library/async_proxy/async_delivery.h>
#include <kernel/common_server/util/network/neh.h>
#include <kernel/common_server/util/accessor.h>
#include <util/string/vector.h>

namespace NNeh {
    class THttpClient;
}

class TAsyncDelivery;

class TPushNotificationsConfig: public IFrontendNotifierConfig {
    using TBase = IFrontendNotifierConfig;
    RTLINE_READONLY_ACCEPTOR(Host, TString, "push.yandex.ru");
    RTLINE_READONLY_ACCEPTOR(Port, ui16, 443);
    RTLINE_READONLY_ACCEPTOR(IsHttps, bool, true);
    RTLINE_READONLY_ACCEPTOR(Uri, TString, "v2/batch_send");
    RTLINE_READONLY_ACCEPTOR_DEF(AuthToken, TString);
    RTLINE_READONLY_ACCEPTOR_DEF(TokenPath, TFsPath);
    RTLINE_READONLY_ACCEPTOR(EventType, TString, "push");

    RTLINE_READONLY_ACCEPTOR(ServiceName, TString, "carsharing");
    RTLINE_READONLY_ACCEPTOR(SubscriptionUri, TString, "v2/subscribe/app");
    RTLINE_READONLY_ACCEPTOR(SubscriptionToken, TString, "");
    RTLINE_READONLY_ACCEPTOR(SubscriptionTokenPath, TFsPath, TFsPath());
    RTLINE_READONLY_ACCEPTOR(DebugMode, bool, false);

    RTLINE_READONLY_ACCEPTOR(PackSize, ui32, 1000);
    RTLINE_READONLY_ACCEPTOR(PacksInterval, TDuration, TDuration::Seconds(30));

    RTLINE_READONLY_ACCEPTOR_DEF(AppNames, TVector<TString>);

private:
    static TFactory::TRegistrator<TPushNotificationsConfig> Registrator;

    virtual void DoInit(const TYandexConfig::Section* section) override;

    virtual void DoToString(IOutputStream& os) const override {
        os << "EventType: " << EventType << Endl;
        os << "Host: " << Host << Endl;
        os << "Port: " << Port << Endl;
        os << "IsHttps: " << IsHttps << Endl;
        os << "Uri: " << Uri << Endl;
        os << "TokenPath: " << TokenPath << Endl;

        os << "ServiceName: " << ServiceName << Endl;
        os << "SubscriptionUri: " << SubscriptionUri << Endl;
        os << "SubscriptionTokenPath: " << SubscriptionTokenPath << Endl;
        os << "DebugMode: " << DebugMode << Endl;

        os << "PackSize: " << PackSize << Endl;
        os << "PacksInterval: " << PacksInterval << Endl;

        os << "AppNames: " << JoinStrings(AppNames, ",") << Endl;
    }

public:
    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override {
        JREAD_STRING_OPT(info, "host", Host);
        JREAD_UINT_OPT(info, "port", Port);
        JREAD_BOOL_OPT(info, "is_https", IsHttps);
        JREAD_BOOL_OPT(info, "is_debug_mode", DebugMode);
        JREAD_STRING_OPT(info, "uri", Uri);
        JREAD_STRING_OPT(info, "auth_token", AuthToken);
        JREAD_STRING_OPT(info, "event_type", EventType);
        JREAD_STRING_OPT(info, "service_name", ServiceName);
        JREAD_STRING_OPT(info, "subscription_uri", SubscriptionUri);
        JREAD_STRING_OPT(info, "subscription_token", SubscriptionToken);
        JREAD_UINT_OPT(info, "pack_size", PackSize);
        JREAD_DURATION_OPT(info, "packs_interval", PacksInterval);
        if (!TJsonProcessor::ReadContainer(info, "app_names", AppNames)) {
            return false;
        }
        return TBase::DeserializeFromJson(info);
    }

    virtual NJson::TJsonValue SerializeToJson() const override {
        NJson::TJsonValue result = TBase::SerializeToJson();
        JWRITE(result, "host", Host);
        JWRITE(result, "port", Port);
        JWRITE(result, "is_https", IsHttps);
        JWRITE(result, "uri", Uri);
        JWRITE(result, "auth_token", AuthToken);
        JWRITE(result, "event_type", EventType);
        JWRITE(result, "service_name", ServiceName);
        JWRITE(result, "subscription_uri", SubscriptionUri);
        JWRITE(result, "subscription_token", SubscriptionToken);
        JWRITE(result, "is_debug_mode", DebugMode);
        JWRITE(result, "pack_size", PackSize);
        JWRITE_DURATION(result, "packs_interval", PacksInterval);
        TJsonProcessor::WriteContainerArray(result, "app_names", AppNames);
        return result;
    }

    virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override {
        NFrontend::TScheme result = TBase::GetScheme(server);
        result.Add<TFSString>("host").SetDefault(Host);
        result.Add<TFSNumeric>("port").SetDefault(Port);
        result.Add<TFSBoolean>("is_https").SetDefault(IsHttps);
        result.Add<TFSBoolean>("is_debug_mode").SetDefault(DebugMode);
        result.Add<TFSString>("uri").SetDefault(Uri);
        result.Add<TFSString>("auth_token").SetDefault(AuthToken);
        result.Add<TFSString>("event_type").SetDefault(EventType);
        result.Add<TFSString>("service_name").SetDefault(ServiceName);
        result.Add<TFSString>("subscription_uri").SetDefault(SubscriptionUri);
        result.Add<TFSString>("subscription_token").SetDefault(SubscriptionToken);
        result.Add<TFSNumeric>("pack_size").SetDefault(PackSize);
        result.Add<TFSDuration>("packs_interval").SetDefault(PacksInterval);
        result.Add<TFSArray>("app_names").SetElement<TFSString>();
        return result;
    }

    virtual IFrontendNotifier::TPtr Construct() const override;
};

class TPushNotifier: public IFrontendNotifier {
    using TBase = IFrontendNotifier;
protected:
    TAtomicSharedPtr<TAsyncDelivery> AD;
    THolder<NNeh::THttpClient> Agent;
    NSimpleMeta::TConfig ReaskConfig;
    const TPushNotificationsConfig Config;
public:
    TPushNotifier(const TPushNotificationsConfig& config);
    virtual ~TPushNotifier();

    virtual bool DoStart(const NCS::IExternalServicesOperator& /*context*/) override;
    virtual bool DoStop() override;

    virtual void SubscribeUser(const TString& appName, const TString& platform, const TString& passportUid, const TString& uuid, const TString& pushToken, THolder<NNeh::THttpAsyncReport::ICallback>&& callback) const;

private:
    virtual TResult::TPtr DoNotify(const TMessage& message, const TContext& context = Default<TContext>()) const override;
    NUtil::THttpReply NotifyImpl(const TMessage& message,
                                 TRecipients::const_iterator recipientsBegin,
                                 TRecipients::const_iterator recipientsEnd) const;
};
