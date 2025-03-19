#include "config.h"
#include <kernel/common_server/abstract/frontend.h>

namespace NCS {
    namespace NKVStorage {
        TDBConfig::TFactory::TRegistrator<TDBConfig> TDBConfig::Registrator("db");

        IStorage::TPtr TDBConfig::DoBuildStorage() const {
            auto db = ICSOperator::GetServer<IBaseServer>().GetDatabase(DBName);
            if (!db) {
                TFLEventLog::Error("incorrect db name")("db_name", DBName);
                return nullptr;
            }
            return new TDBStorage(GetStorageName(), GetThreadsCount(), db, DefaultTableName);
        }

    }
}
