#include "fake.h"

TFakeLocksManagerConfig::TFactory::TRegistrator<TFakeLocksManagerConfig> TFakeLocksManagerConfig::Registrator(TFakeLocksManagerConfig::GetTypeName());

NRTProc::TAbstractLock::TPtr TFakeLocksManager::Lock(const TString& key, const bool /*isReadOnly*/, const TDuration /*timeout*/) const {
    if (MakeRealObjects) {
        return MakeAtomicShared<NRTProc::TFakeLock>(key);
    } else {
        return nullptr;
    }
}

THolder<ILocksManager> TFakeLocksManagerConfig::BuildManager(const IBaseServer& /*server*/) const {
    return MakeHolder<TFakeLocksManager>(MakeRealObjects);
}
