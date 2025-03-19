#pragma once

#include <kernel/common_server/auth/common/processor.h>

class TEmptyConfig {
public:
    bool InitFeatures(const TYandexConfig::Section* /*section*/) { return true; }
    void ToStringFeatures(IOutputStream& /*os*/) const {}
};

template <class THandlerConfig>
class TBaseAuthConfig: public IAuthRequestProcessorConfig, public THandlerConfig {
    using TBase = IAuthRequestProcessorConfig;
    using TNativeBase = THandlerConfig;
public:
    using TBase::TBase;
    virtual void ToString(IOutputStream& os) const override {
        TBase::ToString(os);
        TNativeBase::ToStringFeatures(os);
    }

protected:
    virtual bool DoInit(const TYandexConfig::Section* section) override {
        if (!TBase::DoInit(section)) {
            return false;
        }
        if (!TNativeBase::InitFeatures(section)) {
            return false;
        }
        return true;
    }
};

template <class THandlerType, class THandlerConfig>
class TCommonRequestHandlerConfig: public TBaseAuthConfig<THandlerConfig> {
    using TBase = TBaseAuthConfig<THandlerConfig>;
    using TSelf = TCommonRequestHandlerConfig<THandlerType, THandlerConfig>;

public:
    using TFactory = IRequestProcessorConfig::TFactory;
    static const TFactory::TRegistrator<TSelf> Registrator;

    using TBase::TBase;

    static TString GetTypeName() {
        return THandlerType::GetTypeName();
    }

protected:
    virtual IRequestProcessor::TPtr DoConstructAuthProcessor(IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server) const override {
        return MakeAtomicShared<THandlerType>(*this, context, authModule, server);
    }
};

template <class THandlerType, class THandlerConfig>
const TCommonRequestHandlerConfig<THandlerType, THandlerConfig>::TFactory::template TRegistrator<TCommonRequestHandlerConfig<THandlerType, THandlerConfig>> TCommonRequestHandlerConfig<THandlerType, THandlerConfig>::Registrator(THandlerType::GetTypeName());
