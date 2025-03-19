#pragma once
#include <kernel/common_server/notifications/abstract/abstract.h>

class TNullNotifierConfig: public IFrontendNotifierConfig {
private:
    static TFactory::TRegistrator<TNullNotifierConfig> Registrator;

public:
    virtual IFrontendNotifier::TPtr Construct() const override;

private:
    virtual void DoInit(const TYandexConfig::Section* /*section*/) override {
    }

    virtual void DoToString(IOutputStream& /*os*/) const override {
    }
};

class TNullNotifier: public IFrontendNotifier, public TNonCopyable {
private:
    const TNullNotifierConfig Config;

public:
    TNullNotifier(const TNullNotifierConfig& config)
        : IFrontendNotifier(config)
        , Config(config)
    {
    }

    virtual bool DoStart(const NCS::IExternalServicesOperator& /*context*/) override;
    virtual bool DoStop() override;

private:
    virtual TResult::TPtr DoNotify(const TMessage& message, const TContext& context = Default<TContext>()) const override;
};
