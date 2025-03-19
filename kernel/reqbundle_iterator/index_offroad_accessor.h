#pragma once

#include "index_proc.h"
#include "iterator_offroad_impl.h"
#include "reqbundle_iterator_offroad_impl.h"
#include "word_form_flags.h"

#include <ysite/yandex/posfilter/generic_form_selector.h>

namespace NReqBundleIteratorImpl {
    template <class TOffroadSearcher, class TConvertedKey>
    class TOffroadLemmHits : public ILemmHits {
    private:
        using TTermId = typename TOffroadSearcher::TKeyId;

        struct TOffroadHitInfo {
            TTermId TermId;
            TResizableWordFormFlags Flags;

            TOffroadHitInfo() = default;

            TOffroadHitInfo(TTermId termId, TResizableWordFormFlags&& flags)
                : TermId(termId)
                , Flags(std::move(flags))
            {
            }
        };

        TVector<TOffroadHitInfo> OffroadHits;
        size_t NumForms = 0;
        size_t NumAcceptedForms = 0;
        size_t MaxAcceptedFormsLimit = Max<size_t>();
        bool SkipEmptyForms = false;

    public:
        TOffroadLemmHits(size_t formsLimit, bool skipEmptyForms)
            : MaxAcceptedFormsLimit(formsLimit)
            , SkipEmptyForms(skipEmptyForms)
        {
        }

        void AddKey(const TTermId& termId, TConvertedKey& convertedKey, TLemmIndexData& lemmData, TMemoryPool& pool) {
            NumForms += convertedKey.FormsCount();
            if (NumForms >= NReqBundleIterator::TLowLevelFormId::MaxValue) {
                return;
            }
            if (NumAcceptedForms >= MaxAcceptedFormsLimit) {
                return;
            }

            TResizableWordFormFlags flags(pool);
            const IFormClassifier* formClassifier = lemmData.BundleData->FormClassifier.Get();
            TGenericFormSelector formSelector;

            bool take = false;
            if (lemmData.BundleData->IsAttribute) {
                take = true;
            } else {
                Y_ASSERT(formClassifier);
                size_t acceptedFormsLimit = MaxAcceptedFormsLimit - NumAcceptedForms;
                size_t formsAccepted = formSelector.SetWordFormFlags(*formClassifier, convertedKey, &flags, &lemmData, SkipEmptyForms, acceptedFormsLimit);
                take = formsAccepted != 0;
                NumAcceptedForms += formsAccepted;
            }

            if (take) {
                OffroadHits.emplace_back(termId, std::move(flags));
            }
        }

        void MakeIterators(
            TIteratorPtrs& target,
            size_t blockId,
            TMemoryPool& iteratorsMemory,
            const TLemmIndexData& lemmData) const override
        {
            if (OffroadHits.empty()) {
                return;
            }
            size_t pos = target.size();
            target.resize(pos + OffroadHits.size());
            size_t formDataPtr = 0;
            for (size_t i = 0; i < OffroadHits.size(); i++) {
                TIteratorOffroadPtr it;
                if (lemmData.BundleData->IsAttribute) {
                    it = MakeIteratorOffroad<TOffroadSearcher>(AttributeIteratorType, blockId, iteratorsMemory, OffroadHits[i].TermId);
                } else if (false && lemmData.BundleData->IsStopWord) {
                    it = MakeIteratorOffroad<TOffroadSearcher>(StopWordIteratorType, blockId, iteratorsMemory, OffroadHits[i].TermId);
                } else {
                    it = MakeIteratorOffroad<TOffroadSearcher>(DefaultIteratorType, blockId, iteratorsMemory, OffroadHits[i].TermId);
                }
                if (!lemmData.BundleData->IsAttribute) {
                    lemmData.PrepareTemplatesForKishka(it->GetTemplatesToFill(), OffroadHits[i].Flags, formDataPtr);
                }
                target[pos + i] = std::move(it);
            }
            Y_ASSERT(formDataPtr == lemmData.Forms.size());
        }
    };

    template <class TOffroadSearcher, class TTermKeyConverter>
    class TOffroadIndexAccessor : public IIndexAccessor {
    public:
        using TOffroadIterator = typename TOffroadSearcher::IIterator;

    private:
        TOffroadSearcher& Searcher;
        TOffroadIterator& Iterator;
        TMemoryPool& Pool;

    public:
        TOffroadIndexAccessor(TOffroadSearcher& searcher, TOffroadIterator& iterator, TMemoryPool& pool)
            : Searcher(searcher)
            , Iterator(iterator)
            , Pool(pool)
        {
        }

        void PrepareHits(
                TStringBuf lemmKey,
                TArrayRef<TLemmIndexData*> lemmData,
                const ui64 keyPrefix,
                const NReqBundleIterator::TRBIteratorOptions& options) override
        {
            Y_UNUSED(keyPrefix);

            using TOffroadHits = TOffroadLemmHits<TOffroadSearcher, typename TTermKeyConverter::TConvertedKey>;

            for (TLemmIndexData* lemmDataPtr : lemmData) {
                lemmDataPtr->Hits = MakeHolder<TOffroadHits>(options.FormsPerLemmaLimit, options.SkipEmptyForms);
            }

            TVector<NDoom::TOffroadWadKey> keys;

            if (!Searcher.FindTerms(lemmKey, &Iterator, &keys)) {
                return; // no terms - nothing to do
            }
            TTermKeyConverter converter;
            typename TTermKeyConverter::TConvertedKey convertedKey;

            for (auto& key : keys) {
                bool success = converter(key.PackedKey, &convertedKey);
                Y_ASSERT(success);
                if (!success) {
                    continue;
                }

                for (TLemmIndexData* lemmDataPtr : lemmData) {
                    auto* hitsPtr = static_cast<TOffroadHits*>(lemmDataPtr->Hits.Get());
                    hitsPtr->AddKey(key.Id, convertedKey, *lemmDataPtr, Pool);
                }
            }
        }
    };
} // NReqBundleIteratorImpl
