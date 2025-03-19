#include "db.h"
#include <kernel/common_server/abstract/frontend.h>

TDBLocksManagerConfig::TFactory::TRegistrator<TDBLocksManagerConfig> TDBLocksManagerConfig::Registrator(TDBLocksManagerConfig::GetTypeName());

NRTProc::TAbstractLock::TPtr TDBLocksManager::Lock(const TString& key, const bool isReadOnly, const TDuration timeout) const {
    return Database->Lock(key, !isReadOnly, timeout);
}

THolder<ILocksManager> TDBLocksManagerConfig::BuildManager(const IBaseServer& server) const {
    auto db = server.GetDatabase(DBName);
    CHECK_WITH_LOG(db) << "Incorrect database for locks manager: " << DBName << Endl;
    return MakeHolder<TDBLocksManager>(db);
}
