#pragma once
#include "abstract.h"
#include <kernel/common_server/library/storage/structured.h>

class TDBLocksManagerConfig: public ILocksManagerConfig {
private:
    static TFactory::TRegistrator<TDBLocksManagerConfig> Registrator;
    TString DBName;
protected:
    void DoInit(const TYandexConfig::Section* section) override {
        DBName = section->GetDirectives().Value("DBName", DBName);
    }
    void DoToString(IOutputStream& os) const override {
        os << "DBName: " << DBName << Endl;
    }
    virtual TString GetType() const override {
        return GetTypeName();
    }
public:

    static TString GetTypeName() {
        return "db";
    }

    virtual THolder<ILocksManager> BuildManager(const IBaseServer& server) const override;
};

class TDBLocksManager: public ILocksManager {
private:
    NStorage::IDatabase::TPtr Database;
public:
    TDBLocksManager(NStorage::IDatabase::TPtr database)
        : Database(database)
    {

    }

    virtual NRTProc::TAbstractLock::TPtr Lock(const TString& key, const bool isReadOnly, const TDuration timeout) const override;
};
