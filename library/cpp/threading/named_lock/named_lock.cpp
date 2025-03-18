#include "named_lock.h"

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/list.h>
#include <util/generic/singleton.h>
#include <util/system/event.h>
#include <util/system/mutex.h>
#include <util/system/guard.h>

namespace {
    enum class EOnContention {
        Queue,
        Pass
    };

    class TNamedLockManager {
    public:
        class TNamedLock: public INamedLockManager::INamedLock {
        public:
            ~TNamedLock() override {
                Owner.ReleaseLock(Name);
            }

            TNamedLock(TNamedLockManager& owner, const TString& name)
                : Owner(owner)
                , Name(name)
            {
            }

        private:
            TNamedLockManager& Owner;
            const TString Name;
        };

    private:
        struct TCountedMutex {
            TList<INamedLockManager::ICallback::TPtr> Callbacks;
        };
        typedef THashMap<TString, TSimpleSharedPtr<TCountedMutex>> TLocks;

    public:
        inline bool StartLock(const TString& name, INamedLockManager::ICallback::TPtr callback, EOnContention mode) {
            INamedLockManager::TNamedLockPtr lock;
            with_lock (Mutex) {
                TLocks::iterator i = Locks.find(name);
                if (i == Locks.end()) {
                    i = Locks.insert(TLocks::value_type(name, new TCountedMutex)).first;
                    lock = new TNamedLock(*this, i->first);
                } else if (mode == EOnContention::Queue) {
                    i->second->Callbacks.push_back(callback);
                }
            }

            if (lock) {
                callback->OnLock(lock);
                return true;
            } else {
                return false;
            }
        }

        static bool StartLockStatic(const TString& name, INamedLockManager::ICallback::TPtr callback, EOnContention mode) {
            return Singleton<TNamedLockManager>()->StartLock(name, callback, mode);
        }

    private:
        void ReleaseLock(const TString& name) {
            INamedLockManager::ICallback::TPtr callback;
            with_lock (Mutex) {
                while (true) {
                    TLocks::iterator i = Locks.find(name);
                    Y_ASSERT(i != Locks.end());
                    if (!i->second->Callbacks.empty()) {
                        callback = i->second->Callbacks.front();
                        i->second->Callbacks.pop_front();
                        if (!callback->IsActual()) {
                            callback = nullptr;
                            continue;
                        }
                    } else {
                        Locks.erase(i);
                    }
                    break;
                }
            }

            if (callback) {
                callback->OnLock(new TNamedLock(*this, name));
            }
        }

    private:
        TLocks Locks;
        TMutex Mutex;
    };

    class TTryAcquireLockCallback: public NNamedLock::INamedLockManager::ICallback {
    public:
        void OnLock(NNamedLock::INamedLockManager::TNamedLockPtr lock) override {
            Lock = lock;
        }

        NNamedLock::INamedLockManager::TNamedLockPtr GetLock() {
            return Lock;
        }

    private:
        NNamedLock::INamedLockManager::TNamedLockPtr Lock;
    };

    class TAcquireLockCallback: public TTryAcquireLockCallback {
    public:
        void OnLock(NNamedLock::INamedLockManager::TNamedLockPtr lock) override {
            TTryAcquireLockCallback::OnLock(lock);
            Ev.Signal();
        }

        void Wait(TInstant deadline) {
            Ev.WaitD(deadline);
        }

    private:
        TAutoEvent Ev;
    };
}

bool INamedLockManager::StartLock(const TString& name, INamedLockManager::ICallback::TPtr callback) {
    return TNamedLockManager::StartLockStatic(name, callback, EOnContention::Queue);
}

NNamedLock::TNamedLockPtr NNamedLock::TryAcquireLock(const TString& name) {
    auto callback = MakeAtomicShared<TTryAcquireLockCallback>();
    bool acquired = TNamedLockManager::StartLockStatic(name, callback, EOnContention::Pass);
    return acquired ? callback->GetLock() : nullptr;
}

NNamedLock::TNamedLockPtr NNamedLock::AcquireLock(const TString& name, TInstant deadline) {
    auto callback = MakeAtomicShared<TAcquireLockCallback>();
    TNamedLockManager::StartLockStatic(name, callback, EOnContention::Queue);
    callback->Wait(deadline);
    return callback->GetLock();
}
