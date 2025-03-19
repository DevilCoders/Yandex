#pragma once

#include <kernel/common_server/auth/common/auth.h>

class TFakeAuthInfo: public IAuthInfo {
private:
    const TString AuthId;
    const TString ServiceId;

public:
    TFakeAuthInfo(const TString& authId, const TString& serviceId = "")
        : AuthId(authId)
        , ServiceId(serviceId)
    {
    }

    virtual bool IsAvailable() const override {
        return true;
    }

    virtual const TString& GetUserId() const override {
        return AuthId;
    }

    virtual const TString& GetOriginatorId() const override {
        return ServiceId;
    }
};

class TFakeAuthConfig;

class TFakeAuthModule: public IAuthModule {
private:
    const TFakeAuthConfig& Config;
    virtual IAuthInfo::TPtr DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const;
public:
    TFakeAuthModule(const TFakeAuthConfig& config)
        : Config(config)
    {
    }
};

class TFakeAuthConfig: public IAuthModuleConfig {
private:
    CSA_READONLY(bool, CheckXYandexUid, true);
    CSA_READONLY_DEF(TString, DefaultUserId);
    CSA_READONLY_DEF(TString, ServiceId);
    CSA_READONLY_DEF(TString, FixedUserId);

    static TFactory::TRegistrator<TFakeAuthConfig> Registrator;
protected:
    virtual void DoInit(const TYandexConfig::Section* section) override {
        CHECK_WITH_LOG(section);
        const auto& directives = section->GetDirectives();
        DefaultUserId = directives.Value("DefaultUserId", DefaultUserId);
        FixedUserId = directives.Value("FixedUserId", FixedUserId);
        ServiceId = directives.Value("ServiceId", ServiceId);
        CheckXYandexUid = directives.Value("CheckXYandexUid", CheckXYandexUid);
    }

    virtual void DoToString(IOutputStream& os) const override {
        if (DefaultUserId) {
            os << "DefaultUserId: " << DefaultUserId << Endl;
            os << "FixedUserId: " << FixedUserId << Endl;
            os << "ServiceId: " << ServiceId << Endl;
            os << "CheckXYandexUid: " << CheckXYandexUid << Endl;
        }
    }

    virtual THolder<IAuthModule> DoConstructAuthModule(const IBaseServer* /*server*/) const override {
        return MakeHolder<TFakeAuthModule>(*this);
    }
public:
    TFakeAuthConfig(const TString& name)
        : IAuthModuleConfig(name)
    {
    }
};
