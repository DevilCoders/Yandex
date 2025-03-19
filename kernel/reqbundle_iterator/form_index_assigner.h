#pragma once

#include <kernel/reqbundle/reqbundle.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/memory/pool.h>

namespace NReqBundleIteratorImpl {
    struct TFormIndexAssigner {
        using TForm2Id = THashMap<
            TWtringBuf,
            ui16,
            THash<TWtringBuf>,
            TEqualTo<TWtringBuf>,
            TPoolAllocator>;

        using TId2Form = TVector<TWtringBuf, TPoolAllocator>;

        TForm2Id Form2Id;
        TId2Form Id2Form;

        TFormIndexAssigner(NReqBundle::TConstWordAcc word, TMemoryPool& pool);
    };
} // NReqBundleIteratorImpl
