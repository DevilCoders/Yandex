#include "storage.h"

TVSStorageConfig::TFactory::TRegistrator<TVSStorageConfig> TVSStorageConfig::Registrator("VStorage");

IDTasksStorage::TPtr TVSStorageConfig::Construct() const {
    return MakeAtomicShared<TVSTasksStorage>(*this);
}

