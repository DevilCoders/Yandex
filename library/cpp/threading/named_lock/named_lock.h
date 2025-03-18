#pragma once

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NNamedLock {
    class INamedLockManager {
    public:
        class INamedLock {
        public:
            virtual ~INamedLock() {
            }
        };

        typedef TAutoPtr<INamedLock> TNamedLockPtr;

        class ICallback {
        public:
            typedef TAtomicSharedPtr<ICallback> TPtr;
            virtual ~ICallback() {
            }
            virtual void OnLock(TNamedLockPtr lock) = 0;
            virtual bool IsActual() const {
                return true;
            }
        };

        // returns true if the callback has been invoked instantly in the same thread
        static bool StartLock(const TString& name, ICallback::TPtr callback);
    };

    using TNamedLockPtr = INamedLockManager::TNamedLockPtr;

    namespace NPrivate {
        template <class F>
        class TGuardedRunCallback: public INamedLockManager::ICallback {
        public:
            TGuardedRunCallback(F func)
                : Func(func)
            {
            }

            virtual void OnLock(INamedLockManager::TNamedLockPtr /*lock*/) override {
                Func();
            }

        private:
            F Func;
        };
    }

    // returns true if the function has been executed instantly in the same thread
    template <class F>
    inline bool GuardedRun(const TString& name, F func) {
        auto callback = MakeAtomicShared<NPrivate::TGuardedRunCallback<F>>(func);
        return INamedLockManager::StartLock(name, callback);
    }

    TNamedLockPtr TryAcquireLock(const TString& name);
    TNamedLockPtr AcquireLock(const TString& name, TInstant deadline = TInstant::Max());
}

using INamedLockManager = NNamedLock::INamedLockManager;
