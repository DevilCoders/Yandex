#pragma once
#include <kernel/common_server/user_role/abstract/abstract.h>

class TConfiguredPermissionsManager: public IPermissionsManager {
private:
    using TBase = IPermissionsManager;
    TMap<TString, TSet<TString>> UserRoles;
protected:
    virtual bool Restore(const TString& systemUserId, TUserRolesCompiled& result) const override;
    virtual bool Upsert(const TUserRolesCompiled& result, const TString& userId, bool* isUpdate) const override;
public:
    TConfiguredPermissionsManager(const TMap<TString, TSet<TString>>& roles, const IPermissionsManagerConfig& config, const IBaseServer& server)
        : TBase(server, config)
        , UserRoles(roles)
    {
    }
};

class TConfiguredPermissionsManagerConfig: public IPermissionsManagerConfig {
private:
    static TFactory::TRegistrator<TConfiguredPermissionsManagerConfig> Registrator;
    TMap<TString, TSet<TString>> UserRoles;
protected:
    virtual void DoToString(IOutputStream& os) const override;
    virtual void DoInit(const TYandexConfig::Section* section) override;
public:
    static TString GetTypeName() {
        return "configured";
    }
    virtual TString GetClassName() const override {
        return GetTypeName();
    }
    virtual THolder<IPermissionsManager> BuildManager(const IBaseServer& server) const override {
        return MakeHolder<TConfiguredPermissionsManager>(UserRoles, *this, server);
    }
};
