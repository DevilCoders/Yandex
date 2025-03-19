#pragma once

#include <kernel/common_server/auth/common/auth.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/auth/blackbox/client.h>

#include <kernel/common_server/util/network/neh.h>

class TBlackbox2AuthConfig: public IAuthModuleConfig {
public:
    enum class EAuthMethod {
        Any     /* "any" */,
        Cookie  /* "cookie" */,
        OAuth   /* "oauth" */,
    };
protected:
    virtual void DoInit(const TYandexConfig::Section* section) override;
    virtual void DoToString(IOutputStream& os) const override;
    virtual THolder<IAuthModule> DoConstructAuthModule(const IBaseServer* server) const override;
public:
    TBlackbox2AuthConfig(const TString& name);
    ~TBlackbox2AuthConfig();

    TBlackbox2AuthConfig(const TBlackbox2AuthConfig& other) = delete;
    TBlackbox2AuthConfig(TBlackbox2AuthConfig&& other) = default;

    const TString& GetCookieHost() const {
        return CookieHost;
    }
    const TVector<TString>& GetScopes() const {
        return Scopes;
    }
    EAuthMethod GetAuthMethod() const {
        return AuthMethod;
    }
    bool ShouldIgnoreDeviceId() const {
        return IgnoreDeviceId;
    }

    bool AskUserTicket() const {
        return NeedInClientTicket;
    }

private:
    TString SenderName = "blackbox";
    TString CookieHost = "yandex.ru";
    TVector<TString> Scopes;
    EAuthMethod AuthMethod = EAuthMethod::Any;
    bool IgnoreDeviceId = false;
    bool NeedInClientTicket = false;

    mutable TAtomicSharedPtr<NCS::TBlackboxClient> Client;
    TMutex ClientLock;

private:
    static TFactory::TRegistrator<TBlackbox2AuthConfig> Registrator;
};

class TBlackbox2AuthModule : public IAuthModule {
protected:
    virtual IAuthInfo::TPtr DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const override;
public:
    TBlackbox2AuthModule(const TBlackbox2AuthConfig& config, TAtomicSharedPtr<NCS::TBlackboxClient> client);

    TAtomicSharedPtr<NCS::TBlackboxClient> GetClient() const {
        return Client;
    }

private:
    NThreading::TFuture<NCS::TBlackboxClient::TResponsePtr> MakeOAuthRequest(TStringBuf authorization, TStringBuf userIp) const;
    NThreading::TFuture<NCS::TBlackboxClient::TResponsePtr> MakeSessionIdRequest(TStringBuf sessionId, TStringBuf userIp) const;

private:
    const TBlackbox2AuthConfig& Config;
    const TAtomicSharedPtr<NCS::TBlackboxClient> Client;
};
