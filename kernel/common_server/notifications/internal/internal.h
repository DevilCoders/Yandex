#pragma once
#include <kernel/common_server/notifications/abstract/abstract.h>

class TInternalNotifierConfig: public IFrontendNotifierConfig {
private:
    using TBase = IFrontendNotifierConfig;
    static TFactory::TRegistrator<TInternalNotifierConfig> Registrator;

public:
    virtual IFrontendNotifier::TPtr Construct() const override;

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override {
        return TBase::DeserializeFromJson(info);
    }

    virtual NJson::TJsonValue SerializeToJson() const override {
        NJson::TJsonValue result = TBase::SerializeToJson();
        return result;
    }

    virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override {
        NFrontend::TScheme result = TBase::GetScheme(server);
        return result;
    }

private:
    virtual void DoInit(const TYandexConfig::Section* /*section*/) override {
//        LogPriority = section->GetDirectives().Value("LogPriority", LogPriority);
    }

    virtual void DoToString(IOutputStream& /*os*/) const override {
//        os << "LogPriority: " << LogPriority << Endl;
    }
};

class TInternalNotifier: public IFrontendNotifier, public TNonCopyable {
private:
    const TInternalNotifierConfig Config;

public:
    TInternalNotifier(const TInternalNotifierConfig& config)
        : IFrontendNotifier(config)
        , Config(config)
    {}

    virtual bool DoStart(const NCS::IExternalServicesOperator& /*context*/) override;
    virtual bool DoStop() override;

private:
    virtual TResult::TPtr DoNotify(const TMessage& message, const TContext& context = Default<TContext>()) const override;
};
