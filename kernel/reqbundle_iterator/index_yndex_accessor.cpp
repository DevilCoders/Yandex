#include "index_yndex_accessor.h"

#include "iterator_yndex_impl.h"

namespace NReqBundleIteratorImpl {
    TYndexLemmHits::TYndexLemmHits(TStringBuf lemmKey, TLemmIndexData& lemmData, const IHitsAccess& index, const ui64 keyPrefix) {
        if (lemmData.BundleData->IsAttribute) {
            char buffer[MAXKEY_BUF];
            const size_t len = Min(lemmKey.size(), size_t(MAXKEY_LEN));
            memcpy(buffer, lemmKey.data(), len);
            buffer[len] = 0;
            index.LoadExactKey(keyPrefix, buffer, &Hits, false);
            return;
        }

        IHitsAccess::TIntervalLimits limits;
        limits.MaxInterval = Max<ui32>();
        Y_ASSERT(lemmData.BundleData);
        Y_ASSERT(lemmData.BundleData->FormClassifier);
        ILemmerFormSelector* formSelector = CheckedCast<ILemmerFormSelector*>(lemmData.BundleData->FormClassifier.Get());
        index.AddKeys(
            keyPrefix,
            TString(lemmKey),
            limits,
            formSelector,
            &Hits,
            nullptr, // docFreqLnk
            false, // noFormFilter
            &lemmData);
    }

    void TYndexLemmHits::MakeIterators(
        TIteratorPtrs& target,
        size_t blockId,
        TMemoryPool& iteratorsMemory,
        const TLemmIndexData& lemmData) const
    {
        if (!Hits.Size()) {
            return;
        }
        Y_ENSURE(Hits.GetHitFormat() == HIT_FMT_BLK8 || Hits.GetHitFormat() == HIT_FMT_RT,
            "unknown hit format, " << static_cast<int>(Hits.GetHitFormat()));

        size_t pos = target.size();
        target.resize(pos + Hits.Size());

        size_t formDataPtr = 0;

        for (size_t i = 0; i < Hits.Size(); i++) {
            THitsHolder* kishka = Hits.Get(i);
            Y_ASSERT(kishka);
            TIteratorYndexPtr it;
            if (lemmData.BundleData->IsAttribute) {
                it = MakeIteratorYndex(AttributeIteratorType, blockId, iteratorsMemory, kishka->HitsData);
            } else if (false && lemmData.BundleData->IsStopWord) {
                it = MakeIteratorYndex(StopWordIteratorType, blockId, iteratorsMemory, kishka->HitsData);
            } else {
                it = MakeIteratorYndex(DefaultIteratorType, blockId, iteratorsMemory, kishka->HitsData);
            }
            if (!lemmData.BundleData->IsAttribute) {
                lemmData.PrepareTemplatesForKishka(it->GetTemplatesToFill(), kishka->Flags, formDataPtr);
            }
            target[pos + i] = std::move(it);
        }
        Y_ASSERT(formDataPtr == lemmData.Forms.size());
    }

    void TYndexIndexAccessor::PrepareHits(
        TStringBuf lemmKey,
        TArrayRef<TLemmIndexData*> lemmData,
        const ui64 keyPrefix,
        const NReqBundleIterator::TRBIteratorOptions& options)
    {
        Y_UNUSED(options);

        for (TLemmIndexData* lemmDataPtr : lemmData) {
            lemmDataPtr->Hits = MakeHolder<TYndexLemmHits>(lemmKey, *lemmDataPtr, Index, keyPrefix);
        }
    }

} // NReqBundleIteratorImpl
