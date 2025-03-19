#pragma once
#include <kernel/common_server/api/history/db_entities.h>
#include <kernel/common_server/api/links/link.h>
#include <kernel/common_server/api/links/manager.h>
#include <kernel/common_server/user_role/abstract/abstract.h>

class TDBPermissionsManagerConfig: public IPermissionsManagerConfig {
private:
    static TFactory::TRegistrator<TDBPermissionsManagerConfig> Registrator;
    CSA_READONLY_DEF(THistoryConfig, HistoryConfig);
    CSA_READONLY_DEF(TString, DefaultUser);
    CSA_READONLY_DEF(TString, DBName);
protected:
    virtual void DoToString(IOutputStream& os) const override;
    virtual void DoInit(const TYandexConfig::Section* section) override;
public:
    static TString GetTypeName() {
        return "db";
    }
    virtual TString GetClassName() const override {
        return GetTypeName();
    }
    virtual THolder<IPermissionsManager> BuildManager(const IBaseServer& server) const override;
};
