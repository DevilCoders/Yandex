#pragma once
#include <kernel/common_server/user_auth/abstract/abstract.h>

class TTransparentAuthUsersManagerConfig: public IAuthUsersManagerConfig {
private:
    static TFactory::TRegistrator<TTransparentAuthUsersManagerConfig> Registrator;
public:
    virtual IAuthUsersManager::TPtr BuildManager(const IBaseServer& server) const override;
    static TString GetTypeName() {
        return "transparent";
    }
    virtual TString GetClassName() const override {
        return GetTypeName();
    }

    virtual void Init(const TYandexConfig::Section* /*section*/) override {

    }
    virtual void ToString(IOutputStream& /*os*/) const override {

    }
};

class TTransparentAuthUsersManager: public IAuthUsersManager {
private:
    using TBase = IAuthUsersManager;
protected:
    virtual bool DoAuthUserIdToInternalUserId(const TAuthUserLinkId& authId, TString& userId) const override {
        userId = authId.GetAuthUserId();
        return true;
    }
public:
    virtual bool Restore(const TAuthUserLinkId& authId, TMaybe<TAuthUserLink>& data) const override;
    virtual bool Restore(const TString& /*systemUserId*/, TVector<TAuthUserLink>& /*data*/) const override {
        return true;
    }

    virtual bool StartManager() override {
        return true;
    }
    virtual bool StopManager() override {
        return true;
    }

    TTransparentAuthUsersManager(const IBaseServer& server)
        : TBase(server) {
    }
};
