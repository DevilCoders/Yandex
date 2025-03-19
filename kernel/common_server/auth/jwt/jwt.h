#pragma once

#include <kernel/common_server/auth/common/auth.h>
#include <kernel/common_server/auth/jwt/jwk.h>

class TJwtAuthInfo: public IAuthInfo {
private:
    const TString UserId;
    const bool Available;
    const TString ServiceId;
    const TString ErrorMessage;

public:
    TJwtAuthInfo(bool available, const TString& clientId, const TString& serviceId)
        : UserId(clientId)
        , Available(available)
        , ServiceId(serviceId)
    {
    }

    TJwtAuthInfo(const TString& errorMessage)
        : Available(false)
        , ErrorMessage(errorMessage)
    {
    }

    virtual bool IsAvailable() const override {
        return Available;
    }

    virtual const TString& GetUserId() const override {
        return UserId;
    }

    virtual const TString& GetOriginatorId() const override {
        return ServiceId;
    }

    virtual const TString& GetMessage() const override {
        return ErrorMessage;
    }
};

class TJwtAuthConfig;

class TJwtAuthModule: public IAuthModule {
private:
    const TJwtAuthConfig& Config;
    NExternalAPI::TSender::TPtr JwkApi;
    virtual IAuthInfo::TPtr DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const;

    bool Verify(const TString& message, const TString& signature, const TString& kid, const TString& algorithm) const;
    bool VerifyRSA(const TString& message, const TString& signature, const TString& kid) const;
    TVector<TJwk> GetJwkTokens() const;

public:
    TJwtAuthModule(const TJwtAuthConfig& config, NExternalAPI::TSender::TPtr jwkApi)
        : Config(config)
        , JwkApi(jwkApi)
    {
    }
};

class TJwtAuthConfig: public IAuthModuleConfig {
private:
    CSA_READONLY_DEF(TString, HeaderField);
    CSA_READONLY_DEF(TString, DefaultUserId);
    CSA_READONLY_DEF(TString, SkipSuffix);
    CSA_READONLY_DEF(TString, SkipPrefix);
    CSA_READONLY_DEF(bool, SkipDomain);
    CSA_READONLY_DEF(TString, JwksApi);
    CSA_READONLY_DEF(TString, JwksUri);
    CSA_READONLY_DEF(bool, AllowNoneAlg);

    static TFactory::TRegistrator<TJwtAuthConfig> Registrator;

protected:
    virtual void DoInit(const TYandexConfig::Section* section) override;
    virtual void DoToString(IOutputStream& os) const override;
    virtual THolder<IAuthModule> DoConstructAuthModule(const IBaseServer* server) const override;

public:
    TJwtAuthConfig(const TString& name)
        : IAuthModuleConfig(name)
    {
    }
};
