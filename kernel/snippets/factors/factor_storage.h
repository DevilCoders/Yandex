#pragma once

#include "factorbuf.h"

#include <kernel/factor_storage/factor_storage.h>

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>
#include <util/system/yassert.h>

namespace NSnippets
{
    class TFactorStorage128: private TNonCopyable
    {
    public:
        static const size_t MAX = 128;
    private:
        TMemoryPool Pool;
        TVector<TFactorStorage, TPoolAllocator> Storage;
        TVector<const TFactorStorage*, TPoolAllocator> StoragePtrs;
        size_t Used;

    public:
        explicit TFactorStorage128(const TFactorDomain& domain)
            : Pool((GetFactorStorageInPoolSizeBound<TFactorStorage>(domain.Size()) + sizeof(TFactorStorage*)) * MAX)
            , Storage(&Pool)
            , StoragePtrs(&Pool)
            , Used(0)
        {
            Storage.reserve(MAX);
            for (size_t i = 0; i < MAX; i++) {
                Storage.emplace_back(&domain, &Pool);
                Storage.back().Clear();
            }
            StoragePtrs.reserve(MAX);
            for (size_t i = 0; i < MAX; i++)
                StoragePtrs.push_back(&Storage[i]);
        }

        const TFactorStorage* const* operator~() const
        {
            return StoragePtrs.data();
        }

        void Reset()
        {
            for (size_t i = 0; i < Used; i++)
                Storage[i].Clear();
            Used = 0;
        }

        TFactorStorage& Next()
        {
            return Storage[Used++];
        }

        const TFactorStorage& operator[](size_t at) const {
            Y_ASSERT(at < Used && at < MAX);
            return Storage[at];
        }
        TFactorStorage& operator[](size_t at) {
            Y_ASSERT(at < Used && at < MAX);
            return Storage[at];
        }

        size_t GetUsed() const
        {
            return Used;
        }
    };

}

