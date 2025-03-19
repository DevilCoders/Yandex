#pragma once

#include <kernel/common_server/auth/common/auth.h>

class TMetaAuthModule: public IAuthModule {
public:
    using TSubmodules = TVector<std::pair<TString, IAuthModule::TPtr>>;
protected:
    virtual IAuthInfo::TPtr DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const override;
public:
    TMetaAuthModule(TSubmodules&& submodules)
        : Submodules(std::move(submodules))
    {
    }
private:
    TSubmodules Submodules;
};

class TMetaAuthConfig: public IAuthModuleConfig {
protected:
    virtual void DoInit(const TYandexConfig::Section* section) override;
    virtual void DoToString(IOutputStream& os) const override;
    virtual THolder<IAuthModule> DoConstructAuthModule(const IBaseServer* server) const override;
public:
    using IAuthModuleConfig::IAuthModuleConfig;

private:
    TVector<TString> Submodules;

private:
    static TFactory::TRegistrator<TMetaAuthConfig> Registrator;
};
