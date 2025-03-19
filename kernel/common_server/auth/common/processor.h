#pragma once
#include <kernel/common_server/common/processor.h>
#include "auth.h"

class IAuthRequestProcessorConfig: public IRequestProcessorConfig {
private:
    using TBase = IRequestProcessorConfig;
    TString AuthModuleName = "fake";

protected:
    virtual TString GetAuthModuleName() const;

    virtual IRequestProcessor::TPtr DoConstructProcessor(IReplyContext::TPtr context, const IBaseServer* server) const override final;
    virtual IRequestProcessor::TPtr DoConstructAuthProcessor(IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server) const = 0;
    virtual bool DoInit(const TYandexConfig::Section* section) override;

public:
    using TBase::TBase;

    virtual void ToString(IOutputStream& os) const override;
};

class TAuthRequestProcessor: public IRequestProcessor {
protected:
    virtual void DoAuthProcess(TJsonReport::TGuard& g, IAuthInfo::TPtr authInfo) = 0;
    virtual void DoProcess(TJsonReport::TGuard& g) override final;
    virtual void CheckAuthInfo(TJsonReport::TGuard& g, IAuthInfo::TPtr authInfo);
public:
    TAuthRequestProcessor(const IAuthRequestProcessorConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr auth, const IBaseServer* server);

private:
    IAuthModule::TPtr Auth;
    const IAuthRequestProcessorConfig& AuthConfig;

    virtual TString GenerateRequestLink() const;
};

template <class T>
class TStubRequestProcessorConfig : public IAuthRequestProcessorConfig {
public:
    using IAuthRequestProcessorConfig::IAuthRequestProcessorConfig;

protected:
    virtual IRequestProcessor::TPtr DoConstructAuthProcessor(IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server) const override {
        return MakeAtomicShared<T>(*this, context, authModule, server);
    }
};

template <class T>
using TSimpleProcessorRegistrator = IRequestProcessorConfig::TFactory::TRegistrator<TStubRequestProcessorConfig<T>>;
