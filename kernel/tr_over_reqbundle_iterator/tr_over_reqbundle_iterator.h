#pragma once

#include "break_word_accumulator.h"
#include "tr_over_reqbundle_hits_buf_pool.h"

#include <kernel/doom/search_fetcher/search_fetcher.h>

#include <kernel/reqbundle_iterator/constraint_checker.h>
#include <kernel/reqbundle_iterator/reqbundle_iterator.h>

#include <util/generic/array_ref.h>

namespace NTrOverReqBundleIterator {
    class TTrOverRBIterator {
    public:
        TTrOverRBIterator(
            TReqBundleIteratorPtr&& iterator,
            const TVector<TVector<std::pair<ui32, EFormClass>>>& blockTrIteratorWordIdxs,
            ui32 trIteratorWordCount,
            THolder<NReqBundleIterator::TConstraintChecker> constraintChecker,
            const ISentenceLengthsLenReader* sentReader,
            const TVector<bool>& blockMustNot)
            : Iterator(std::forward<TReqBundleIteratorPtr>(iterator))
            , ConstraintChecker(std::move(constraintChecker))
            , SentenceLengthsReader(sentReader)
            , SentenceLengthsPreLoader(sentReader ? sentReader->CreatePreLoader() : nullptr)
            , BlockMustNot(blockMustNot)
            , Accumulator(trIteratorWordCount, blockTrIteratorWordIdxs)
        {
        }

        void AnnounceDocIds(TConstArrayRef<ui32> docIds) {
            if (Iterator) {
                Iterator->AnnounceDocIds(docIds);
            }
            if (SentenceLengthsPreLoader) {
                SentenceLengthsPreLoader->AnnounceDocIds(docIds);
            }
        }

        void PreLoadDoc(ui32 docId, const NDoom::TSearchDocLoader& loader) {
            if (Iterator) {
                Iterator->PreLoadDoc(docId, loader);
            }
            if (SentenceLengthsPreLoader) {
                SentenceLengthsPreLoader->PreLoadDoc(docId, loader);
            }
        }

        void AdviseDocIds(TConstArrayRef<ui32> docIds, std::function<void(ui32)> consumer) {
            if (Iterator) {
                Iterator->AdviseDocIds(docIds, consumer);
            } else {
                for (ui32 docId : docIds) {
                    consumer(docId);
                }
            }
        }

        void PrefetchDocs(const TVector<ui32>& docIds) {
            if (Iterator) {
                Iterator->PrefetchDocs(docIds);
            }
        }

        void InitForDoc(ui32 docId) {
            if (Iterator) {
                Iterator->InitForDoc(docId);
            }
        }

        /**
         * reqBundleHits can be nullptr
         */
        bool LoadDocumentHits(
            ui32 docId,
            size_t maxReqBundleHitsCount,
            TDocumentHitsBuf* trHits,
            NReqBundleIterator::TPosBuf* reqBundleHits,
            TTrOverReqBundleHitsBufPool* pool,
            bool useConvert = true);

        bool LoadDocumentHitsPartial(
            ui32 docId,
            TArrayRef<TFullPositionEx> trHitsBuf,
            int* trHitsCount,
            ui64* wordMask,
            TArrayRef<NReqBundleIterator::TPosition> reqBundleHitsBuf,
            size_t* reqBundleHitsCount = nullptr,
            TArrayRef<ui32> formCounts = TArrayRef<ui32>(),
            TArrayRef<ui16> richTreeForms = TArrayRef<ui16>(),
            bool useConvert = true);

        TReqBundleIterator* GetReqBundleIterator() const {
            return Iterator.Get();
        }

    private:
        TReqBundleIteratorPtr Iterator;

        THolder<NReqBundleIterator::TConstraintChecker> ConstraintChecker;
        const ISentenceLengthsLenReader* SentenceLengthsReader = nullptr;
        THolder<ISentenceLengthsPreLoader> SentenceLengthsPreLoader;

        TVector<bool> BlockMustNot;

        NPrivate::TBreakWordAccumulator Accumulator;
    };
} // namespace NTrOverReqBundleIterator

using TTrOverReqBundleIterator = NTrOverReqBundleIterator::TTrOverRBIterator;
using TTrOverReqBundleIteratorPtr = THolder<TTrOverReqBundleIterator>;
