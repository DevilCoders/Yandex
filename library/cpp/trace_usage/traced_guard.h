#pragma once

#include "global_registry.h"

namespace NTraceUsage {
    class TAcquireGuard: private NNonCopyable::TNonCopyable {
    protected:
        TTraceRegistryPtr Registry;

    public:
        TAcquireGuard();
        ~TAcquireGuard();
    };

    class TReleaseGuard: private NNonCopyable::TNonCopyable {
    protected:
        TTraceRegistryPtr Registry;

    public:
        TReleaseGuard();
        ~TReleaseGuard();
    };

    template <class T, class TOps = TCommonLockOps<T>>
    struct TTracedOpts {
        static inline void Acquire(T* t) noexcept {
            TAcquireGuard acquireGuard;
            TOps::Acquire(t);
        }

        static inline void Release(T* t) noexcept {
            TReleaseGuard releaseGuard;
            TOps::Release(t);
        }
    };

    template <class T, class TOps = TCommonLockOps<T>>
    using TTracedGuard = TGuard<T, TTracedOpts<T, TOps>>;

}
