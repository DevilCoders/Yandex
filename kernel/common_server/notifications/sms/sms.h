#pragma once

#include <kernel/common_server/notifications/abstract/abstract.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>

class TSMSNotificationsConfig: public IFrontendNotifierConfig {
private:
    using TBase = IFrontendNotifierConfig;

    CSA_READONLY_DEF(TString, Sender);
    CSA_READONLY_DEF(TString, Route);
    CSA_READONLY(bool, RequireUid, true);
    CSA_READONLY_DEF(TSet<TString>, WhiteList);
    CSA_READONLY(TString, SenderApiName, "sms-backend");

public:
    virtual void DoInit(const TYandexConfig::Section* section) override;
    virtual void DoToString(IOutputStream& os) const override;
    virtual IFrontendNotifier::TPtr Construct() const override;

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override;
    virtual NJson::TJsonValue SerializeToJson() const override;
    virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override;

private:
    static TFactory::TRegistrator<TSMSNotificationsConfig> Registrator;
};

// TSMSNotifier uses yasms (sms.passport.yandex.ru) to send messages.
class TSMSNotifier: public IFrontendNotifier {
private:
    using TBase = IFrontendNotifier;
    const TSMSNotificationsConfig Config;
    NExternalAPI::TSender::TPtr Sender;
    TAtomicSharedPtr<NTvmAuth::TTvmClient> Tvm;

public:
    TSMSNotifier(const TSMSNotificationsConfig& config);
    ~TSMSNotifier() override;

    virtual bool DoStart(const NCS::IExternalServicesOperator& context) override;
    virtual bool DoStop() override;

private:
    virtual TResult::TPtr DoNotify(const TMessage& message, const TContext& context) const override;
};
