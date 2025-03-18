#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/refcount.h>
#include <util/stream/output.h>
#include <util/system/rwlock.h>
#include <util/system/tls.h>

template <class T>
class TLockableHandle {
private:
    struct TReentrantRWMutex {
        TRWMutex Mutex;

        NTls::TValue<TSimpleCounter> ReadCounter;
        NTls::TValue<TSimpleCounter> WriteCounter;

        inline void AcquireRead() {
            Y_VERIFY(!WriteCounter.Get().Val(), "rwlock demotion is not supported");

            if (!ReadCounter.Get().Val())
                Mutex.AcquireRead();
            ReadCounter.Get().Inc();
        }

        inline void ReleaseRead() {
            if (!ReadCounter.Get().Dec())
                Mutex.ReleaseRead();
        }

        inline void AcquireWrite() {
            Y_VERIFY(!ReadCounter.Get().Val(), "rwlock promotion is not supported");

            if (!WriteCounter.Get().Val())
                Mutex.AcquireWrite();
            WriteCounter.Get().Inc();
        }

        inline void ReleaseWrite() {
            if (!WriteCounter.Get().Dec())
                Mutex.ReleaseWrite();
        }
    };

    struct TImpl: TAtomicRefCount<TImpl>, TNonCopyable {
        THolder<T> Object;

        TReentrantRWMutex ReloadScriptMutex;
        TReentrantRWMutex Mutex;

        explicit TImpl(TAutoPtr<T> object) noexcept
            : Object(object)
        {
        }
    };

    struct TImplReadOps {
        static void Acquire(TImpl* impl) noexcept {
            impl->Mutex.AcquireRead();
        }

        static void Release(TImpl* impl) noexcept {
            impl->Mutex.ReleaseRead();
        }
    };

    struct TImplWriteOps {
        static void Acquire(TImpl* impl) noexcept {
            impl->Mutex.AcquireWrite();
        }

        static void Release(TImpl* impl) noexcept {
            impl->Mutex.ReleaseWrite();
        }
    };

    struct TImplReloadScriptWriteOps {
        static void Acquire(TImpl* impl) noexcept {
            impl->ReloadScriptMutex.AcquireWrite();
            impl->Mutex.AcquireWrite();
        }

        static void Release(TImpl* impl) noexcept {
            impl->Mutex.ReleaseWrite();
            impl->ReloadScriptMutex.ReleaseWrite();
        }
    };

    struct TImplReloadScriptReadOps {
        static void Acquire(TImpl* impl) noexcept {
            impl->ReloadScriptMutex.AcquireRead();
        }

        static void Release(TImpl* impl) noexcept {
            impl->ReloadScriptMutex.ReleaseRead();
        }
    };

    TIntrusivePtr<TImpl> Impl;

public:
    class TReadLock: TNonCopyable {
    private:
        TIntrusivePtr<TImpl> Impl;
        TGuard<TImpl, TImplReadOps> Guard;

    public:
        TReadLock(TLockableHandle handle) noexcept
            : Impl(handle.Impl)
            , Guard(Impl.Get())
        {
        }

        const T* operator->() const noexcept {
            return Impl->Object.operator->();
        }

        const T& operator*() const noexcept {
            return Impl->Object.operator*();
        }

        const T* Get() const noexcept {
            return Impl->Object.Get();
        }
    };

    class TWriteLock: TNonCopyable {
    private:
        TIntrusivePtr<TImpl> Impl;
        TGuard<TImpl, TImplWriteOps> Guard;

    public:
        TWriteLock(TLockableHandle handle) noexcept
            : Impl(handle.Impl)
            , Guard(Impl.Get())
        {
        }

        T* operator->() const noexcept {
            return Impl->Object.operator->();
        }

        T& operator*() const noexcept {
            return Impl->Object.operator*();
        }

        T* Get() const noexcept {
            return Impl->Object.Get();
        }
    };

    class TReloadScriptWriteLock: TNonCopyable {
    private:
        TIntrusivePtr<TImpl> Impl;
        TGuard<TImpl, TImplReloadScriptWriteOps> Guard;

    public:
        TReloadScriptWriteLock(TLockableHandle handle) noexcept
            : Impl(handle.Impl)
            , Guard(Impl.Get())
        {
        }

        T* operator->() const noexcept {
            return Impl->Object.operator->();
        }

        T& operator*() const noexcept {
            return Impl->Object.operator*();
        }

        T* Get() const noexcept {
            return Impl->Object.Get();
        }
    };

    class TReloadScriptReadLock: TNonCopyable {
    private:
        TIntrusivePtr<TImpl> Impl;
        TGuard<TImpl, TImplReloadScriptReadOps> Guard;

    public:
        TReloadScriptReadLock(TLockableHandle handle) noexcept
            : Impl(handle.Impl)
            , Guard(Impl.Get())
        {
        }

        T* operator->() const noexcept {
            return Impl->Object.operator->();
        }

        T& operator*() const noexcept {
            return Impl->Object.operator*();
        }

        T* Get() const noexcept {
            return Impl->Object.Get();
        }
    };

    explicit TLockableHandle(TAutoPtr<T> object)
        : Impl(new TImpl(object))
    {
    }

    bool IsLockedRead() const noexcept {
        return Impl->ReadCounter.Get().Val();
    }

    bool IsLockedWrite() const noexcept {
        return Impl->WriteCounter.Get().Val();
    }
};
