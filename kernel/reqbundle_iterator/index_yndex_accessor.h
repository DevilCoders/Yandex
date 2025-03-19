#pragma once

#include "index_proc.h"

#include <ysite/yandex/posfilter/filter_tree.h>
#include <ysite/yandex/posfilter/hits_loader.h>
#include <ysite/yandex/srchmngr/yrequester.h>

namespace NReqBundleIteratorImpl {
    class TYndexLemmHits : public ILemmHits {
    private:
        TLeafHits Hits;
    public:
        TYndexLemmHits(TStringBuf lemmKey, TLemmIndexData& lemmData, const IHitsAccess& index, const ui64 keyPrefix);

        void MakeIterators(
            TIteratorPtrs& target,
            size_t blockId,
            TMemoryPool& iteratorsMemory,
            const TLemmIndexData& lemmData) const override;
    };

    class TYndexIndexAccessor : public IIndexAccessor {
    private:
        THitsLoader Index;
    public:
        TYndexIndexAccessor(
            const TYndexRequester& yr,
            IHitsAccess::EIndexType yrType,
            TLangMask langMask,
            TLangMask flatBastardsLangMask)
            : Index(&yr.YMain(), langMask, flatBastardsLangMask, yr.GetAnchorWordWeight(), yr.GetHitsForReadLoader(), yrType, true)
        {
        }

        void PrepareHits(
            TStringBuf lemmKey,
            TArrayRef<TLemmIndexData*> lemmData,
            const ui64 keyPrefix,
            const NReqBundleIterator::TRBIteratorOptions& options) override;
    };
} // NReqBundleIteratorImpl
