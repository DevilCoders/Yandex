#pragma once

#include <kernel/remorph/api/remorph.h>

#include <util/generic/ptr.h>
#include <util/system/guard.h>

namespace NRemorphAPI {

namespace NImpl {

class TBase: public virtual IBase, public TAtomicRefCount<TBase> {
    typedef TAtomicRefCount<TBase> TParent;

protected:
    TBase() {
        // Initial count
        TParent::Ref();
    }

    inline long InternalAddRef() {
        TParent::Ref();
        return TParent::RefCount();
    }

    inline long InternalRelease() {
        const long res = TParent::RefCount();
        TParent::UnRef();
        return res - 1;
    }
public:
    virtual ~TBase() {
    }

    long AddRef() override {
        return InternalAddRef();
    }

    long Release() override {
        return InternalRelease();
    }
};

// LockOps to use with TGuard
struct TBaseLockOps {
    static inline void Acquire(IBase* p) noexcept {
        p->AddRef();
    }
    static inline void Release(IBase* p) noexcept {
        p->Release();
    }
};

typedef TGuard<IBase, TBaseLockOps> TLocker;

} // NImpl

} // NRemorphAPI
