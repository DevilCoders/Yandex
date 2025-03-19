#pragma once

#include "reqbundle_iterator_fwd.h"

#include <kernel/reqbundle_iterator/enums/reqbundle_enums.h>
#include <kernel/reqbundle/block.h>
#include <util/generic/ptr.h>

namespace NReqBundleIterator {
    // TODO. This class is search-specific.
    // Consider moving it to search/*.
    class TRBIteratorsHashers {
    public:
        THolder<IRBIteratorsHasher> TrHasher;
        THolder<IRBIteratorsHasher> LrHasher;
        THolder<IRBIteratorsHasher> AnnHasher;
        THolder<IRBIteratorsHasher> FactorAnnHasher;
        THolder<IRBIteratorsHasher> LinkAnnHasher;

        void Init(EHashersSize hSize);
    };
} // NReqBundleIterator

bool FromString(
    const TStringBuf& text,
    NReqBundleIterator::EHashersSize& value);
