#pragma once

#include <util/memory/alloc.h>
#include <util/system/yassert.h>
#include <util/system/sys_alloc.h>

namespace NMemorySearch {

class TRTDestructAndDeallocate {
public:
    template <class T>
    static inline void Destroy(T* t) noexcept {
        t->~T();
        y_deallocate(t);
    }
};

template <class T, class D = TRTDestructAndDeallocate>
class TShortIntrusivePtrOps {
public:
    static inline void Ref(T* t) noexcept {
        Y_ASSERT(t);
        t->Ref();
    }
    static inline void UnRef(T* t) noexcept {
        Y_ASSERT(t);

        Y_ASSERT(t->GetRefCount() != 0);
        t->UnRef();
        if (!t->GetRefCount()) {
            D::Destroy(t);
        }
    }
};

} // namespace NMemorySearch
