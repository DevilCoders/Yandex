#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/noncopyable.h>
#include <util/memory/pool.h>

namespace NDups {
    class TAppendOnlyDenseVectorPool: private TNonCopyable {
    public:
        using THashType = ui32;
        using TArrayType = TArrayRef<THashType>;
        using TManagedArrayType = TArrayType;

        TAppendOnlyDenseVectorPool(TMemoryPool* externalPool);
        ~TAppendOnlyDenseVectorPool();

        TArrayType GrowArray(TManagedArrayType& array, const size_t additionalLength);
        size_t WastedItems() const;
        size_t WastedBytes() const;

    private:
        class TImpl;
        THolder<TImpl> Impl;
    };
}
