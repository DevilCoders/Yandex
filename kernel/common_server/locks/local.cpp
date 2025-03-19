#include "local.h"
#include <library/cpp/threading/named_lock/named_lock.h>

TMemoryLocksManagerConfig::TFactory::TRegistrator<TMemoryLocksManagerConfig> TMemoryLocksManagerConfig::Registrator(TMemoryLocksManagerConfig::GetTypeName());

namespace {
    class TNamedLockAdapter: public NRTProc::TAbstractLock {
    private:
        NNamedLock::TNamedLockPtr Lock;
    public:
        TNamedLockAdapter(NNamedLock::TNamedLockPtr lock)
            : Lock(lock) {
        }

        bool IsLockTaken() const override {
            return !!Lock;
        }

        bool IsLocked() const override {
            return !!Lock;
        }
    };
}

NRTProc::TAbstractLock::TPtr TMemoryLocksManager::Lock(const TString& key, const bool /*isReadOnly*/, const TDuration timeout) const {
    return MakeAtomicShared<TNamedLockAdapter>(NNamedLock::AcquireLock(Prefix + key, Now() + timeout));
}

THolder<ILocksManager> TMemoryLocksManagerConfig::BuildManager(const IBaseServer& /*server*/) const {
    return MakeHolder<TMemoryLocksManager>(Prefix);
}
