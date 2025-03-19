#pragma once

#include <kernel/common_server/notifications/abstract/abstract.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>

class TUCommNotifierConfig: public IFrontendNotifierConfig {
private:
    using TBase = IFrontendNotifierConfig;
    static TFactory::TRegistrator<TUCommNotifierConfig> Registrator;
    CSA_READONLY(TString, ApiName, "ucommunications");
    CSA_READONLY_DEF(TString, Sender);
    CSA_READONLY_DEF(TString, Intent);

public:
    virtual IFrontendNotifier::TPtr Construct() const override;

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override {
        JREAD_FROM_STRING_OPT(info, "api_name", ApiName);
        JREAD_FROM_STRING(info, "sender", Sender);
        JREAD_FROM_STRING(info, "intent", Intent);
        return TBase::DeserializeFromJson(info);
    }

    virtual NJson::TJsonValue SerializeToJson() const override {
        NJson::TJsonValue result = TBase::SerializeToJson();
        JWRITE(result, "api_name", ApiName);
        JWRITE(result, "sender", Sender);
        JWRITE(result, "intent", Intent);
        return result;
    }

    virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override {
        NFrontend::TScheme result = TBase::GetScheme(server);
        result.Add<TFSString>("ApiName").SetDefault("ucommunications");
        result.Add<TFSString>("Sender").SetRequired(true);
        result.Add<TFSString>("Intent").SetRequired(true);
        return result;
    }

    virtual void DoInit(const TYandexConfig::Section* section) override {
        ApiName = section->GetDirectives().Value("ApiName", ApiName);
        Sender = section->GetDirectives().Value("Sender", Sender);
        Intent = section->GetDirectives().Value("Intent", Intent);
    }

    virtual void DoToString(IOutputStream& os) const override {
        os << "ApiName: " << ApiName << Endl;
        os << "Sender: " << Sender << Endl;
        os << "Intent: " << Intent << Endl;
    }
};

class TUCommNotifier: public IFrontendNotifier, public TNonCopyable {
private:
    using TBase = IFrontendNotifier;
    const TUCommNotifierConfig Config;
    NExternalAPI::TSender::TPtr Sender = nullptr;

public:
    TUCommNotifier(const TUCommNotifierConfig& config);
    ~TUCommNotifier() override;

    virtual bool DoStart(const NCS::IExternalServicesOperator& context) override;
    virtual bool DoStop() override;

private:
    virtual TResult::TPtr DoNotify(const TMessage& message, const TContext& context) const override;
};
