#pragma once
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/user_auth/abstract/abstract.h>
#include <kernel/common_server/library/searchserver/simple/context/replier.h>

class IAuthInfo {
public:
    using TPtr = TAtomicSharedPtr<IAuthInfo>;
    CSA_DEFAULT(IAuthInfo, TString, ModuleName);
public:
    IAuthInfo() = default;
    virtual ~IAuthInfo() {}

    virtual bool IsAvailable() const = 0;
    virtual const TString& GetUserId() const = 0;
    virtual const TString& GetMessage() const {
        return Default<TString>();
    }
    virtual NJson::TJsonValue GetInfo() const {
        return {};
    }
    virtual int GetCode() const {
        return 0;
    }

    virtual const TString& GetOriginatorId() const {
        return GetUserId();
    }
    TAuthUserLinkId GetAuthId() const {
        return TAuthUserLinkId(ModuleName, GetUserId());
    }
};

class IAuthModule {
private:
    CSA_DEFAULT(IAuthModule, TString, ModuleName);
protected:
    virtual IAuthInfo::TPtr DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const = 0;
public:
    using TPtr = TAtomicSharedPtr<IAuthModule>;

    virtual ~IAuthModule() {}
    virtual IAuthInfo::TPtr RestoreAuthInfo(IReplyContext::TPtr requestContext) const final {
        auto result = DoRestoreAuthInfo(requestContext);
        if (!!result) {
            result->SetModuleName(ModuleName);
        }
        return result;
    }
};

class IAuthModuleConfig {
private:
    CSA_READONLY_DEF(TString, ModuleName);
protected:
    virtual void DoInit(const TYandexConfig::Section* section) = 0;
    virtual void DoToString(IOutputStream& os) const = 0;
    virtual THolder<IAuthModule> DoConstructAuthModule(const IBaseServer* server) const = 0;
public:
    using TPtr = TAtomicSharedPtr<IAuthModuleConfig>;
    using TFactory = NObjectFactory::TParametrizedObjectFactory<IAuthModuleConfig, TString, TString>;

    IAuthModuleConfig(const TString& moduleName)
        : ModuleName(moduleName)
    {
    }

    virtual IAuthModule::TPtr ConstructAuthModule(const IBaseServer* server) const final {
        THolder<IAuthModule> result = DoConstructAuthModule(server);
        if (!!result) {
            result->SetModuleName(ModuleName);
        }
        return result.Release();
    }
    virtual void Init(const TYandexConfig::Section* section) final {
        ModuleName = section->GetDirectives().Value("ModuleName", ModuleName);
        DoInit(section);
    }
    virtual void ToString(IOutputStream& os) const final {
        os << "ModuleName: " << ModuleName << Endl;
        os << "Type: " << ModuleName << Endl;
        DoToString(os);
    }
    virtual ~IAuthModuleConfig() {}

    virtual const TSet<TString>& GetRequiredModules() const {
        return Default<TSet<TString>>();
    }
};
