#pragma once

#include "iterator_impl.h"

#include <library/cpp/wordpos/superlong_hit.h>

#include <ysite/yandex/srchmngr/yrequester.h>
#include <ysite/yandex/posfilter/hits_loader.h>
#include <kernel/keyinv/hitlist/rtpositerator.h>

namespace NReqBundleIteratorImpl {
    static const ui8 YndexIteratorMagicNumber = 13;

    class TIteratorYndexBase : public TIterator {
    public:
        TIteratorYndexBase(ui32 blockId, TMemoryPool& pool)
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

    template <EIteratorType IteratorType, class TBaseIter>
    class TIteratorYndex
        : public TIteratorYndexBase
    {
    private:
        TBaseIter Base;
        SUPERLONG NextDoc = 0;
        bool NeedSecondStage = false;

    public:
        TIteratorYndex(ui32 blockId, TMemoryPool& pool, const THitsForRead& hits)
            : TIteratorYndexBase(blockId, pool)
        {
            Base.Init(hits);
        }

        TIteratorYndex(ui32 blockId, TMemoryPool& pool, IInputStream* rh, const IKeysAndPositions* index)
            : TIteratorYndexBase(blockId, pool)
        {
            Y_ASSERT(index);
            Base.RestoreIndexAccess(rh, *index);
            size_t templatesSize = 0;
            LoadPodType(rh, templatesSize);
            Templates.resize(templatesSize);
            LoadIterRange(rh, Templates.begin(), Templates.end());
        }

        void InitForDoc(
            ui32 docId,
            bool firstStage,
            const TIteratorOptions& options,
            TSharedIteratorData& sharedData) override
        {
            if (firstStage) {
                SUPERLONG pos = (SUPERLONG)docId << DOC_LEVEL_Shift;
                Y_ASSERT(pos >= NextDoc);
                NextDoc = pos + (1LL << DOC_LEVEL_Shift);
                if (Base.Valid() && Base.Current() < pos)
                    Base.SkipTo(pos);
                NeedSecondStage = false;
                SecondStagePtr = nullptr;
            }
            if (firstStage || NeedSecondStage) {
                auto fetcher = [this](TSuperlongHit* hit) -> bool {
                    SUPERLONG indexPos = Base.Current();
                    if (indexPos >= NextDoc)
                        return false;
                    Base.Next();
                    *hit = indexPos;
                    return true;
                };
                size_t fetchLimit = Min(options.FirstStageFetchLimit, options.FetchLimit);
                if (options.FirstStageFetchLimit >= options.FetchLimit) {
                    Y_ASSERT(!NeedSecondStage);
                    CurrentPtr = FetchAndFilterHits<TSuperlongHit, IteratorType, EBreakBufferMode::Ignore>(
                        options.FetchLimit,
                        sharedData,
                        Templates,
                        BlockId,
                        fetcher).Start;
                } else if (firstStage) {
                    TFetchHitsResult fetchResult = FetchAndFilterHits<TSuperlongHit, IteratorType, EBreakBufferMode::Fill>(
                        fetchLimit,
                        sharedData,
                        Templates,
                        BlockId,
                        fetcher);
                    CurrentPtr = fetchResult.Start;
                    NeedSecondStage = fetchResult.Truncated;
                } else {
                    SecondStagePtr = FetchAndFilterHits<TSuperlongHit, IteratorType, EBreakBufferMode::Use>(
                        options.FetchLimit - fetchLimit + 1,
                        sharedData,
                        Templates,
                        BlockId,
                        fetcher).Start;
                }
            }
        }

        void LookupNextDoc(ui32& docId) override {
            if (IteratorType == StopWordIteratorType)
                return;
            if (Base.Valid() && Base.Current() < NextDoc)
                Base.SkipTo(NextDoc);

            if (IteratorType != AttributeIteratorType) {
                while (Base.Valid() && !Templates[TWordPosition::Form(Base.Current())].Valid())
                    Base.Next();
            }

            if (Base.Valid())
                docId = Min(docId, Base.Doc());
        }

        void LookupNextDoc(TIteratorDecayInfo& info) override {
            if (IteratorType == StopWordIteratorType)
                return;
            if (Base.Valid() && Base.Current() < NextDoc)
                Base.SkipTo(NextDoc);

            if (IteratorType != AttributeIteratorType) {
                while (Base.Valid() && !Templates[TWordPosition::Form(Base.Current())].Valid())
                    Base.Next();
            }

            if (Base.Valid()) {
                if (Base.Doc() < info.MinDocId) {
                    info.MinDocId = Base.Doc();
                    info.Iterator = this;
                }
            }
        }

        TSortOrder GetSortOrder() const override {
            ythrow yexception() << "not implemented";
        }


        void SerializeToProto(NReqBundleIteratorProto::TBlockIterator* proto) const override {
            Y_UNUSED(proto);
            ythrow yexception() << "Not implemented";
        }

    private:
        void SaveIteratorImpl(IOutputStream* rh) const override {
            // Header
            ui8 type = ItIndex;
            SavePodType(rh, type);
            SavePodType(rh, YndexIteratorMagicNumber);
            SavePodType(rh, IteratorType);
            // Body
            Base.SaveIndexAccess(rh);
            SavePodType(rh, Templates.size());
            SaveIterRange(rh, Templates.begin(), Templates.end());
        }

    };

    using TIteratorYndexPtr = THolder<TIteratorYndexBase, TDestructor>;
    using TBaseIterDefault = TPosIterator<DecoderFallBack<CHitDecoder2, true, HIT_FMT_BLK8>>;

    template <EIteratorType IteratorType>
    inline TIteratorYndexPtr DoMakeIteratorYndex(ui32 blockId, TMemoryPool& iteratorsMemory, const THitsForRead& hits)
    {
        if (hits.GetHitFormat() == HIT_FMT_RT) {
            return TIteratorYndexPtr(iteratorsMemory.New<TIteratorYndex<IteratorType, TRTPosIterator>>(blockId, iteratorsMemory, hits));
        } else {
            return TIteratorYndexPtr(iteratorsMemory.New<TIteratorYndex<IteratorType, TBaseIterDefault>>(blockId, iteratorsMemory, hits));
        }
    }

    template <EIteratorType IteratorType>
    inline TIteratorYndexPtr DoMakeIteratorYndex(ui32 blockId, TMemoryPool& iteratorsMemory, IInputStream* rh, const IKeysAndPositions* index)
    {
        if (index->GetVersion() == YNDEX_VERSION_RT) {
            return TIteratorYndexPtr(iteratorsMemory.New<TIteratorYndex<IteratorType, TRTPosIterator>>(blockId, iteratorsMemory, rh, index));
        } else {
            return TIteratorYndexPtr(iteratorsMemory.New<TIteratorYndex<IteratorType, TBaseIterDefault>>(blockId, iteratorsMemory, rh, index));
        }
    }

    template <class... Args>
    inline TIteratorYndexPtr MakeIteratorYndex(
        EIteratorType iteratorType,
        ui32 blockId,
        TMemoryPool& iteratorsMemory,
        Args&&... args)
    {
        if (iteratorType == DefaultIteratorType) {
            return DoMakeIteratorYndex<DefaultIteratorType>(blockId, iteratorsMemory, std::forward<Args>(args)...);
        } else if (iteratorType == StopWordIteratorType) {
            return DoMakeIteratorYndex<StopWordIteratorType>(blockId, iteratorsMemory, std::forward<Args>(args)...);
        } else if (iteratorType == AttributeIteratorType) {
            return DoMakeIteratorYndex<AttributeIteratorType>(blockId, iteratorsMemory, std::forward<Args>(args)...);
        } else {
            Y_ENSURE(false, "Unknown iterator type " << static_cast<ui32>(iteratorType));
        }
    }
} // NReqBundleIteratorImpl
