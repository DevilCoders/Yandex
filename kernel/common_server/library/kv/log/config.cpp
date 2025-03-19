#include "config.h"
#include "storage.h"

namespace NCS {
    namespace NKVStorage {
        TLogConfig::TFactory::TRegistrator<TLogConfig> TLogConfig::Registrator("log");

        IStorage::TPtr TLogConfig::DoBuildStorage() const {
            return new TLogStorage(GetStorageName(), GetThreadsCount(), MemoryMapUsageFlag);
        }

    }
}
