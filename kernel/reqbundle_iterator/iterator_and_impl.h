#pragma once

#include "iterator_impl.h"
#include "bundle_proc.h"
#include "index_proc.h"
#include "saveload.h"
#include "reqbundle_iterator_builder.h"

#include <library/cpp/deprecated/iterators_heap/iterators_heap.h>

namespace NReqBundleIteratorImpl {
    class TBasicIteratorAnd
        : public TIterator
    {
    public:
        using TIterator::TIterator;

        virtual bool HasData() const = 0;
    };

    template<size_t MaxWords>
    class TIteratorAnd
        : public TBasicIteratorAnd
    {
        static_assert(MaxWords <= 64, "MaxWords > 64 in TIteratorAnd");

    private:
        TIteratorPtrs Iterators;
        TDeprecatedIteratorsHeap<TIterator, THoldVector<TIterator>> IteratorsHeap;
        ui64 FullHitsMask = 0;
        ui32 MaxDistance = 0;
        ui32 NumWords;

    public:
        TIteratorAnd(ui32 blockId, ui32 numWords, TMemoryPool& iteratorsMemory, const TBlockBundleData* commonData, TBlockIndexData& blockData)
            : TBasicIteratorAnd(blockId)
            , NumWords(numWords)
        {
            Y_ASSERT(NumWords <= MaxWords);
            Y_ASSERT(commonData);
            Y_ASSERT(blockData.NumWords == NumWords);
            Y_ASSERT(commonData->NumWords == NumWords);

            MaxDistance = commonData->Distance;
            for (size_t i = 0; i < NumWords; i++) {
                if (commonData->Words[i].AnyWord) {
                    continue;
                }
                FullHitsMask |= (ui64(1) << i);
                blockData.Words[i].MakeIterators(
                    Iterators,
                    nullptr,
                    i,
                    iteratorsMemory);
            }
            Initialize();
        }

        TIteratorAnd(ui32 blockId, ui32 numWords, TMemoryPool& iteratorsMemory, IInputStream* rh, NReqBundleIterator::IRBIndexIteratorLoader& loader)
            : TBasicIteratorAnd(blockId)
            , NumWords(numWords)
        {
            Y_ASSERT(NumWords <= MaxWords);
            LoadPodType(rh, FullHitsMask);
            LoadPodType(rh, MaxDistance);
            size_t iteratorsSize = 0;
            LoadPodType(rh, iteratorsSize);
            Iterators.resize(iteratorsSize);
            for (size_t i = 0; i != iteratorsSize; ++i) {
                ui32 blockId2 = 0;
                LoadPodType(rh, blockId2);
                Iterators[i] = TIteratorSaveLoad::LoadIterator(
                    rh,
                    blockId2,
                    iteratorsMemory,
                    loader);
            }
            Initialize();
        }

        TIteratorAnd(
            ui32 blockId,
            ui32 numWords,
            TMemoryPool& iteratorsMemory,
            const NReqBundleIteratorProto::TAndIterator& proto,
            NReqBundleIterator::IRBIteratorDeserializer& deserializer)
            : TBasicIteratorAnd(blockId)
            , NumWords(numWords)
        {
            FullHitsMask = proto.GetFullHitsMask();
            MaxDistance = proto.GetMaxDistance();
            Iterators.reserve(proto.GetIterators().size());
            for (const auto& iter : proto.GetIterators()) {
                Iterators.push_back(deserializer.DeserializeFromProto(iter, iteratorsMemory));
            }
            Initialize();
        }

        void Initialize() {
            if (!Iterators.empty()) {
                IteratorsHeap.Restart(Iterators.data(), Iterators.size(), [](TIteratorPtr* it) -> TIterator* { return it->Get(); });
            }
        }

        bool HasData() const override {
            return !Iterators.empty();
        }

        void InitForDoc(
            ui32 docId,
            bool firstStage,
            const TIteratorOptions& options,
            TSharedIteratorData& sharedData) override
        {
            Y_ASSERT(!Iterators.empty());
            for (auto& it : Iterators)
                it->InitForDoc(docId, firstStage, options, sharedData);

            if (!firstStage) {
                IteratorsHeap.Restart();
                Init(options, &sharedData.HitsStorage);
            }
        }

        void LookupNextDoc(ui32& docId) override {
            if (Iterators.empty())
                return;
            std::array<ui32, MaxWords> perWordsNextDoc;
            for (size_t i = 0; i < NumWords; i++)
                perWordsNextDoc[i] = Max<ui32>();
            for (auto& it : Iterators)
                it->LookupNextDoc(perWordsNextDoc[it->BlockId]);
            ui32 maxNextDoc = 0;
            for (size_t i = 0; i < NumWords; i++)
                maxNextDoc = Max(maxNextDoc, perWordsNextDoc[i]);
            docId = Min(docId, maxNextDoc);
        }

        void LookupNextDoc(TIteratorDecayInfo& info) override {
            if (Iterators.empty())
                return;
            std::array<ui32, MaxWords> perWordsNextDoc;
            for (size_t i = 0; i < NumWords; i++)
                perWordsNextDoc[i] = Max<ui32>();
            for (auto& it : Iterators)
                it->LookupNextDoc(perWordsNextDoc[it->BlockId]);
            ui32 maxNextDoc = 0;
            for (size_t i = 0; i < NumWords; i++)
                maxNextDoc = Max(maxNextDoc, perWordsNextDoc[i]);
            if (maxNextDoc < info.MinDocId) {
                info.MinDocId = maxNextDoc;
                info.Iterator = this;
            }
        }

        TSortOrder GetSortOrder() const override {
            return { Max<ui32>(), Max<ui32>() };
        }

        void SerializeToProto(NReqBundleIteratorProto::TBlockIterator* proto) const override {
            for (const auto& iter : Iterators) {
                iter->SerializeToProto(proto->MutableAndIterator()->AddIterators());
            }
            proto->MutableAndIterator()->SetFullHitsMask(FullHitsMask);
            proto->MutableAndIterator()->SetMaxDistance(MaxDistance);
            proto->MutableAndIterator()->SetNumWords(NumWords);
            proto->SetBlockId(BlockId);
            proto->SetBlockType(BlockType);
        }
    private:
        void Init(const TIteratorOptions& options, THitsStorage* storage) {
            Y_ASSERT(options.FetchLimit > 0);
            std::array<TPosition, MaxWords> wordHits;
            ui64 wordHitsMask = 0;
            TPosition lastHit;
            lastHit.Clear();
            TPosition* beg = storage->Reserve(options.FetchLimit);
            TPosition* end = beg + (options.FetchLimit - 1);
            CurrentPtr = beg;
            while (1) {
                TPosition pos = IteratorsHeap.Current();
                if (!pos.Valid()) {
                    break;
                }
                IteratorsHeap.Next();
                if (!pos.TBreak::EqualUpper(lastHit)
                    || pos.WordPosBeg() - lastHit.WordPosBeg() > MaxDistance)
                {
                    wordHitsMask = 0;
                }
                lastHit = pos;
                ui32 word = pos.BlockId();
                Y_ASSERT(word < NumWords && ((FullHitsMask >> word) & 1));
                wordHits[word] = pos;
                wordHitsMask |= 1ULL << word;
                if (wordHitsMask == FullHitsMask) {
                    for (size_t j = 0; j < NumWords; j++) {
                        if ((FullHitsMask >> j) & 1) {
                            pos.TRelev::SetMin(wordHits[j]);
                            pos.TMatch::SetMax(wordHits[j]);
                            pos.TWordPosBeg::SetMin(wordHits[j]);
                            pos.TWordPosEnd::SetMax(wordHits[j]);
                        }
                    }
                    pos.TLemmId::Set(0);
                    pos.TLowLevelFormId::Set(0);
                    pos.TBlockId::Set(BlockId);
                    if (beg < end) {
                        beg[0] = pos;
                        ++beg;
                    } else {
                        break;
                    }
                    wordHitsMask = 0;
                }
            }
            beg[0].SetInvalid();
            storage->Advance(beg + 1 - CurrentPtr);
            Y_ASSERT(CheckOrder(CurrentPtr, NReqBundleIterator::TLemmId::UpperMask));
        }

        void SaveIteratorImpl(IOutputStream* rh) const override {
            // Header
            ui8 type = ItAnd;
            SavePodType(rh, type);
            ui32 numWords = NumWords;
            SavePodType(rh, numWords);
            // Body
            SavePodType(rh, FullHitsMask);
            SavePodType(rh, MaxDistance);
            size_t iteratorsSize = Iterators.size();
            SavePodType(rh, iteratorsSize);
            for (size_t i = 0; i != iteratorsSize; ++i) {
                ui32 blockId = Iterators[i]->BlockId;
                SavePodType(rh, blockId);
                TIteratorSaveLoad::SaveIterator(rh, *Iterators[i]);
            }
        }

    };

    template<class... Args>
    inline THolder<TBasicIteratorAnd, TDestructor> MakeIteratorAnd(
        size_t numWords,
        ui32 blockId,
        TMemoryPool& iteratorsMemory,
        Args&&... args)
    {
        if (numWords <= 16) {
            return THolder<TBasicIteratorAnd, TDestructor>(iteratorsMemory.New<TIteratorAnd<16>>(blockId, numWords, iteratorsMemory, std::forward<Args>(args)...));
        }
        if (numWords <= 64) {
            return THolder<TBasicIteratorAnd, TDestructor>(iteratorsMemory.New<TIteratorAnd<64>>(blockId, numWords, iteratorsMemory, std::forward<Args>(args)...));
        }
        return nullptr;
    }
} // NReqBundleIteratorImpl
