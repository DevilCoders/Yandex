#pragma once
#include <kernel/common_server/notifications/abstract/abstract.h>

class TLogsNotifierConfig: public IFrontendNotifierConfig {
private:
    using TBase = IFrontendNotifierConfig;
    ELogPriority LogPriority = TLOG_DEBUG;
    static TFactory::TRegistrator<TLogsNotifierConfig> Registrator;

public:
    ELogPriority GetLogPriority() const {
        return LogPriority;
    }

    virtual IFrontendNotifier::TPtr Construct() const override;

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override {
        JREAD_FROM_STRING_OPT(info, "log_priority", LogPriority);
        return TBase::DeserializeFromJson(info);
    }

    virtual NJson::TJsonValue SerializeToJson() const override {
        NJson::TJsonValue result = TBase::SerializeToJson();
        JWRITE_ENUM(result, "log_priority", LogPriority);
        return result;
    }

    virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override {
        NFrontend::TScheme result = TBase::GetScheme(server);
        result.Add<TFSVariants>("log_priority").InitVariants<ELogPriority>();
        return result;
    }

private:
    virtual void DoInit(const TYandexConfig::Section* section) override {
        LogPriority = section->GetDirectives().Value("LogPriority", LogPriority);
    }

    virtual void DoToString(IOutputStream& os) const override {
        os << "LogPriority: " << LogPriority << Endl;
    }
};

class TLogsNotifier: public IFrontendNotifier, public TNonCopyable {
private:
    const TLogsNotifierConfig Config;

public:
    TLogsNotifier(const TLogsNotifierConfig& config)
        : IFrontendNotifier(config)
        , Config(config)
    {}

    virtual bool DoStart(const NCS::IExternalServicesOperator& /*context*/) override;
    virtual bool DoStop() override;

private:
    virtual TResult::TPtr DoNotify(const TMessage& message, const TContext& context) const override;
};
