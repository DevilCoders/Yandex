#pragma once

#include <kernel/common_server/auth/common/auth.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/network/neh.h>
#include <kernel/common_server/util/network/neh_request.h>

class TYangAuthInfo: public IAuthInfo {
private:
    const bool Available;
    const TString UserId;

public:
    TYangAuthInfo(bool available, const TString& userId)
        : Available(available)
        , UserId(userId)
    {
    }

    virtual bool IsAvailable() const override {
        return Available;
    }

    virtual const TString& GetUserId() const override {
        return UserId;
    }
};

class TYangAuthConfig;

class TYangAuthModule: public IAuthModule {
public:
    TYangAuthModule(const TYangAuthConfig& config);
protected:
    virtual IAuthInfo::TPtr DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const override;

private:
    TString AuthToken;
    TString VerifierUri;
    NSimpleMeta::TConfig RequestsConfig;
    const bool CheckIsActivePair;

    TAsyncDelivery::TPtr AD;
    THolder<NNeh::THttpClient> Requester;
};

class TYangAuthConfig: public IAuthModuleConfig {
    RTLINE_READONLY_ACCEPTOR(AuthToken, TString, "");
    RTLINE_READONLY_ACCEPTOR(VerifierUri, TString, "");
    RTLINE_READONLY_ACCEPTOR(Host, TString, "");
    RTLINE_READONLY_ACCEPTOR(Port, ui32, 0);
    RTLINE_READONLY_ACCEPTOR(Https, bool, false);
    RTLINE_READONLY_ACCEPTOR(RequestsConfig, NSimpleMeta::TConfig, NSimpleMeta::TConfig());
    RTLINE_READONLY_ACCEPTOR(CheckIsActivePair, bool, true);

private:
    TAsyncDelivery::TPtr AD;
protected:
    virtual void DoInit(const TYandexConfig::Section* section) override;
    virtual void DoToString(IOutputStream& os) const override;
    virtual THolder<IAuthModule> DoConstructAuthModule(const IBaseServer* /*server*/) const override {
        return MakeHolder<TYangAuthModule>(*this);
    }

public:
    using IAuthModuleConfig::IAuthModuleConfig;

    TAsyncDelivery::TPtr GetAsyncDelivery() const {
        return AD;
    }

    ~TYangAuthConfig() {
        AD->Stop();
    }

private:
    static TFactory::TRegistrator<TYangAuthConfig> Registrator;
};
