#pragma once

#include <util/generic/ptr.h>
#include <util/system/rwlock.h>

template <class C, class CLock = TRWMutex, class CGuard = TReadGuard>
class TGuardedPtr: public TPointerBase<TGuardedPtr<C, CLock, CGuard>, C> {
private:
    using TSmartPtr = TAtomicSharedPtr<C>;
    using TObserverPtr = C*;

private:
    CGuard Guard;
    TSmartPtr OwnedPointer;
    TObserverPtr ObserverPointer;

public:
    explicit TGuardedPtr(TAtomicSharedPtr<C> pointer, const CLock& lock) noexcept
        : Guard(lock)
        , OwnedPointer(pointer)
        , ObserverPointer(nullptr)
    {
    }

    explicit TGuardedPtr(C& reference, const CLock& lock) noexcept
        : Guard(lock)
        , OwnedPointer(nullptr)
        , ObserverPointer(&reference)
    {
    }

    C* Get() const noexcept {
        if (OwnedPointer) {
            return OwnedPointer.Get();
        } else {
            return ObserverPointer;
        }
    }
};
