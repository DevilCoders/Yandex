#pragma once

#include <kernel/common_server/auth/common/auth.h>
#include <kernel/common_server/auth/blackbox/info.h>

#include <library/cpp/tvmauth/client/facade.h>

class TTvmAuthInfo: public IAuthInfo {
private:
    TString UserId;
    const bool Available;
    const TString ServiceId;
    const TString ErrorMessage;

public:
    TTvmAuthInfo(bool available, const TString& clientId, const TString& serviceId)
        : UserId(clientId)
        , Available(available)
        , ServiceId(serviceId)
    {
    }

    TTvmAuthInfo(const TString& errorMessage)
        : Available(false)
        , ErrorMessage(errorMessage)
    {
    }

    virtual const TString& GetUserId() const override {
        return UserId;
    }

    TTvmAuthInfo& SetUserId(const TString& value) noexcept {
        UserId = value;
        return *this;
    }

    TTvmAuthInfo& SetUserId(TString&& value) noexcept {
        UserId = std::move(value);
        return *this;
    }

    virtual NJson::TJsonValue GetInfo() const override;

    virtual bool IsAvailable() const override {
        return Available;
    }

    virtual const TString& GetMessage() const override {
        return ErrorMessage;
    }

    virtual const TString& GetOriginatorId() const override {
        return ServiceId ? ServiceId : UserId;
    }
};

class TTvmAuthConfig;

class TTvmAuthModule: public IAuthModule {
public:
    using TClientId = NTvmAuth::TTvmId;
    using TClientIds = TSet<TClientId>;
protected:
    virtual IAuthInfo::TPtr DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const override;
public:
    TTvmAuthModule(TAtomicSharedPtr<NTvmAuth::TTvmClient> client, const TTvmAuthConfig& config);

private:
    TAtomicSharedPtr<NTvmAuth::TTvmClient> Client;
    const TTvmAuthConfig& Config;
};

class TTvmAuthConfig: public IAuthModuleConfig {
public:
    using TClientId = TTvmAuthModule::TClientId;
    using TClientIds = TTvmAuthModule::TClientIds;
protected:
    virtual THolder<IAuthModule> DoConstructAuthModule(const IBaseServer* server) const override;
    virtual void DoInit(const TYandexConfig::Section* section) override;
    virtual void DoToString(IOutputStream& os) const override;
public:
    using IAuthModuleConfig::IAuthModuleConfig;

private:
    CSA_READONLY_DEF(TClientIds, AcceptedClientIds);
    CSA_READONLY_DEF(TString, TvmClientName);
    CSA_READONLY_DEF(TString, ServiceId);
    CSA_READONLY_DEF(TString, UserId);
    CSA_READONLY_DEF(TString, TicketPass);
    CSA_READONLY(TString, UserIdHeader, "X-Yandex-Uid");
    CSA_READONLY_DEF(TString, DefaultUserId);
    CSA_READONLY_DEF(TVector<TString>, BBAttributes);

private:
    static TFactory::TRegistrator<TTvmAuthConfig> Registrator;
};
