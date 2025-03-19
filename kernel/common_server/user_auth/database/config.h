#pragma once
#include <kernel/common_server/api/history/config.h>
#include <kernel/common_server/api/history/common.h>
#include <kernel/common_server/user_auth/abstract/abstract.h>
#include <library/cpp/yconf/conf.h>

class TDBAuthUsersManagerConfig: public IAuthUsersManagerConfig {
private:
    static TFactory::TRegistrator<TDBAuthUsersManagerConfig> Registrator;
    CSA_READONLY_DEF(THistoryConfig, HistoryConfig);
    CSA_READONLY_DEF(TString, DBName);
    CSA_READONLY(bool, UseAuthModuleId, false);
public:
    virtual IAuthUsersManager::TPtr BuildManager(const IBaseServer& server) const override;
    virtual void Init(const TYandexConfig::Section* section) override;
    virtual void ToString(IOutputStream& os) const override;
    static TString GetTypeName() {
        return "db";
    }
    virtual TString GetClassName() const override {
        return GetTypeName();
    }
};
