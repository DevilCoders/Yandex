#pragma once

#include "iterator_impl.h"
#include "reqbundle_iterator_offroad_impl.h"

#include <kernel/reqbundle_iterator/proto/reqbundle_iterator.pb.h>

#include <kernel/doom/offroad_wad/offroad_wad_searcher.h>

namespace NReqBundleIteratorImpl {
    static const ui8 OffroadIteratorMagicNumber = 42;

    class TIteratorOffroadBase : public TIterator {
    public:
        TIteratorOffroadBase(ui32 blockId, TMemoryPool& pool)
            : TIterator(blockId)
            , Templates(&pool)
        {
        }

        TPositionTemplates& GetTemplatesToFill() {
            return Templates;
        }

    protected:
        TPositionTemplates Templates;
    };

    template<class TOffroadSearcher, EIteratorType IteratorType>
    class TIteratorOffroad
        : public TIteratorOffroadBase
    {
    private:
        typename TOffroadSearcher::TKeyId OffroadTermId;
        bool NeedSecondStage = false;

        template <typename Searcher>
        struct TCanFetchTrait {
            static constexpr bool Value = true;
        };

        template <NDoom::EWadIndexType IndexType, class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer>
        struct TCanFetchTrait<NDoom::TOffroadWadKeySearcher<IndexType, Hit, Vectorizer, Subtractor, PrefixVectorizer>> {
            static constexpr bool Value = false;
        };

    public:
        using TOffroadIterator = typename TOffroadSearcher::IIterator;
        using TTermId = typename TOffroadSearcher::TKeyId;
        using THit = typename TOffroadSearcher::THit;

        TIteratorOffroad(ui32 blockId, TMemoryPool& pool, TTermId offroadTermId)
            : TIteratorOffroadBase(blockId, pool)
            , OffroadTermId(offroadTermId)
        {
        }

        TIteratorOffroad(ui32 blockId, TMemoryPool& pool, IInputStream* rh)
            : TIteratorOffroadBase(blockId, pool)
        {
            LoadPodType(rh, OffroadTermId.Id);
            LoadPodType(rh, OffroadTermId.Range);
            size_t templatesSize = 0;
            LoadPodType(rh, templatesSize);
            Templates.resize(templatesSize);
            LoadIterRange(rh, Templates.begin(), Templates.end());
        }

        void PrefetchDocs(const TVector<ui32>& docIds, TSharedIteratorData& sharedData) override {
            if constexpr (TCanFetchTrait<TOffroadSearcher>::Value) {
                TOffroadSharedIteratorData<TOffroadSearcher>& offroadSharedData = sharedData.As<TOffroadSharedIteratorData<TOffroadSearcher>>();
                TOffroadIterator* iterator = offroadSharedData.OffroadIterator.Get();
                offroadSharedData.OffroadSearcher.PrefetchDocs(docIds, iterator);
            } else {
                Y_ENSURE(false, "This iterator type cannot be used to fetch hits");
            }
        }

        void InitForDoc(
            ui32 docId,
            bool firstStage,
            const TIteratorOptions& options,
            TSharedIteratorData& sharedData) override
        {
            if constexpr (TCanFetchTrait<TOffroadSearcher>::Value) {
                if (firstStage) {
                    SecondStagePtr = nullptr;
                    NeedSecondStage = false;
                }
                if (firstStage || NeedSecondStage) {
                    TOffroadSharedIteratorData<TOffroadSearcher>& offroadSharedData = sharedData.As<TOffroadSharedIteratorData<TOffroadSearcher>>();
                    TOffroadIterator* iterator = offroadSharedData.OffroadIterator.Get();
                    if (!offroadSharedData.OffroadSearcher.Find(docId, OffroadTermId, iterator)) {
                        CurrentPtr = &NReqBundleIterator::TPosition::Invalid;
                        return;
                    }
                    auto fetcher = [iterator](THit* hit) {
                        return iterator->ReadHit(hit);
                    };
                    size_t fetchLimit = Min(options.FirstStageFetchLimit, options.FetchLimit);
                    if (options.FirstStageFetchLimit >= options.FetchLimit) {
                        Y_ASSERT(!NeedSecondStage);
                        CurrentPtr = FetchAndFilterHits<THit, IteratorType, EBreakBufferMode::Ignore>(
                            options.FetchLimit,
                            sharedData,
                            Templates,
                            BlockId,
                            fetcher).Start;
                    } else if (firstStage) {
                        TFetchHitsResult fetchResult = FetchAndFilterHits<THit, IteratorType, EBreakBufferMode::Fill>(
                            fetchLimit,
                            sharedData,
                            Templates,
                            BlockId,
                            fetcher);
                        CurrentPtr = fetchResult.Start;
                        NeedSecondStage = fetchResult.Truncated;
                    } else {
                        for (size_t i = 1; i < fetchLimit; ) {
                            THit indexPos;
                            bool hitFetched = fetcher(&indexPos);
                            if (Y_UNLIKELY(!hitFetched)) {
                                Y_ASSERT(false);
                                return;
                            }
                            if (IteratorType == AttributeIteratorType || Templates[indexPos.Form()].Valid())
                                i++;
                        }
                        SecondStagePtr = FetchAndFilterHits<THit, IteratorType, EBreakBufferMode::Use>(
                            options.FetchLimit - fetchLimit + 1,
                            sharedData,
                            Templates,
                            BlockId,
                            fetcher).Start;
                    }
                }
            } else {
                Y_ENSURE(false, "This iterator type cannot be used to fetch hits");
            }
        }

        void LookupNextDoc(ui32&) override {
            ythrow yexception() << "not implemented";
        }
        void LookupNextDoc(TIteratorDecayInfo&) override {
            ythrow yexception() << "not implemented";
        }

        TSortOrder GetSortOrder() const override {
            return { OffroadTermId.Id, OffroadTermId.Range };
        }

    private:
        void SaveIteratorImpl(IOutputStream* rh) const override {
            // Header
            ui8 type = ItIndex;
            SavePodType(rh, type);
            SavePodType(rh, OffroadIteratorMagicNumber);
            SavePodType(rh, IteratorType);
            // Body
            SavePodType(rh, OffroadTermId.Id);
            SavePodType(rh, OffroadTermId.Range);
            SavePodType(rh, Templates.size());
            SaveIterRange(rh, Templates.begin(), Templates.end());
        }

        void SerializeToProto(NReqBundleIteratorProto::TBlockIterator* proto) const override {
            NReqBundleIteratorProto::TOffroadIterator iter;

            iter.MutableOffroadTermId()->SetId(OffroadTermId.Id);
            iter.MutableOffroadTermId()->SetRange(OffroadTermId.Range);

            iter.MutableTemplates()->MutableRawPositions()->Reserve(Templates.size());
            for (const auto& hit : Templates) {
                iter.MutableTemplates()->MutableRawPositions()->Add(hit.Raw);
            }

            *proto->MutableIndexIterator()->MutableOffroadIterator() = std::move(iter);
            proto->MutableIndexIterator()->SetIteratorType(static_cast<ui32>(IteratorType));
            proto->SetBlockId(BlockId);
            proto->SetBlockType(BlockType);
        }

    };

    using TIteratorOffroadPtr = THolder<TIteratorOffroadBase, TDestructor>;

    template<class TOffroadSearcher, class... Args>
    inline TIteratorOffroadPtr MakeIteratorOffroad(
        EIteratorType iteratorType,
        ui32 blockId,
        TMemoryPool& iteratorsMemory,
        Args&&... args)
    {
        if (iteratorType == DefaultIteratorType) {
            return TIteratorOffroadPtr(iteratorsMemory.New<TIteratorOffroad<TOffroadSearcher, DefaultIteratorType>>(blockId, iteratorsMemory, std::forward<Args>(args)...));
        } else if (iteratorType == StopWordIteratorType) {
            return TIteratorOffroadPtr(iteratorsMemory.New<TIteratorOffroad<TOffroadSearcher, StopWordIteratorType>>(blockId, iteratorsMemory, std::forward<Args>(args)...));
        } else if (iteratorType == AttributeIteratorType) {
            return TIteratorOffroadPtr(iteratorsMemory.New<TIteratorOffroad<TOffroadSearcher, AttributeIteratorType>>(blockId, iteratorsMemory, std::forward<Args>(args)...));
        } else {
            Y_ENSURE(false, "Unknown iterator type " << static_cast<ui32>(iteratorType));
        }
    }
} // NReqBundleIteratorImpl
