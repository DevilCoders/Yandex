#pragma once

#include <util/generic/ptr.h>

// Alias for THolder which allows using functions as a destructors.
namespace NLibgit2 {
    template<auto DestroyFn>
    class TFnDestroy {
    public:
        template<class T>
        static void Destroy(T *t) noexcept {
            DestroyFn(t);
        }
    };
}

namespace NPrivate {
    template<typename T, auto D>
    using THolder = THolder<T, NLibgit2::TFnDestroy<D>>;
} // namespace NPrivate