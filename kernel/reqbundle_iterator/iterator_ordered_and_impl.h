#pragma once

#include "iterator_and_impl.h"
#include "bundle_proc.h"
#include "index_proc.h"
#include "saveload.h"

#include <kernel/sent_lens/sent_lens.h>

#include <library/cpp/deprecated/iterators_heap/iterators_heap.h>

namespace NReqBundleIteratorImpl {
    template<size_t MaxWords>
    class TIteratorOrderedAnd
        : public TBasicIteratorAnd
    {
        static_assert(MaxWords <= 64, "MaxWords > 64 in TIteratorOrderedAnd");

    private:
        TIteratorPtrs Iterators;
        std::array<TDeprecatedIteratorsHeap<TIterator, THoldVector<TIterator>>, MaxWords> IteratorsHeap;
        ui32 NumWords = 0;
        ui64 AnyWordsMask = 0;

    public:
        TIteratorOrderedAnd(ui32 blockId, ui32 numWords, TMemoryPool& iteratorsMemory, const TBlockBundleData* commonData, TBlockIndexData& blockData)
            : TBasicIteratorAnd(blockId)
            , NumWords(numWords)
        {
            Y_ASSERT(0 < NumWords && NumWords <= MaxWords);
            Y_ASSERT(commonData);
            Y_ASSERT(blockData.NumWords == NumWords);
            Y_ASSERT(commonData->NumWords == NumWords);

            for (size_t i = 0; i < NumWords; i++) {
                if (commonData->Words[i].AnyWord) {
                    AnyWordsMask |= (ui64(1) << i);
                    continue;
                }
                size_t offset = Iterators.size();
                blockData.Words[i].MakeIterators(
                    Iterators,
                    nullptr,
                    i,
                    iteratorsMemory);
                IteratorsHeap[i].Restart(Iterators.data() + offset, Iterators.size() - offset, [](TIteratorPtr* it) -> TIterator* { return it->Get(); });;
            }
        }

        TIteratorOrderedAnd(
            ui32 blockId,
            ui32 numWords,
            TMemoryPool& iteratorsMemory,
            ui64 anyWordsMask,
            const NReqBundleIteratorProto::TOrderedAndIterator& proto,
            NReqBundleIterator::IRBIteratorDeserializer& deserializer)
            : TBasicIteratorAnd(blockId)
            , NumWords(numWords)
            , AnyWordsMask(anyWordsMask)
        {
            auto producer = [&](size_t index) {
                Y_ASSERT(index < static_cast<size_t>(proto.GetIterators().size()));
                ui32 blockId = proto.GetIterators(index).GetBlockId();
                auto iteratorPtr = deserializer.DeserializeFromProto(proto.GetIterators(index), iteratorsMemory);
                return std::make_pair(std::move(iteratorPtr), blockId);
            };
            LoadIterators(proto.GetIterators().size(), producer);
        }

        TIteratorOrderedAnd(ui32 blockId1, ui32 numWords, TMemoryPool& iteratorsMemory, IInputStream* rh, NReqBundleIterator::IRBIndexIteratorLoader& loader)
            : TBasicIteratorAnd(blockId1)
            , NumWords(numWords)
        {
            Y_ASSERT(0 < NumWords && NumWords <= MaxWords);

            LoadPodType(rh, AnyWordsMask);
            size_t iteratorsSize = 0;
            LoadPodType(rh, iteratorsSize);
            Iterators.reserve(iteratorsSize);
            auto producer = [&](size_t) {
                ui32 blockId = 0;
                LoadPodType(rh, blockId);
                auto iteratorPtr = TIteratorSaveLoad::LoadIterator(
                    rh,
                    blockId,
                    iteratorsMemory,
                    loader);
                return std::make_pair(std::move(iteratorPtr), blockId);
            };
            LoadIterators(iteratorsSize, producer);
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
            if (firstStage) {
                for (auto& it : Iterators) {
                    it->InitForDoc(docId, firstStage, options, sharedData);
                }
                sharedData.SharedSentenceLengths.Update(docId);
            } else {
                for (size_t i = 0; i < NumWords; ++i) {
                    IteratorsHeap[i].Restart();
                }
                Init(options, sharedData);
            }
        }

        void LookupNextDoc(ui32&) override {
            throw yexception() << "not implemented";
        }

        void LookupNextDoc(TIteratorDecayInfo&) override {
            throw yexception() << "not implemented";
        }

        TSortOrder GetSortOrder() const override {
            return { Max<ui32>(), Max<ui32>() };
        }

        void SerializeToProto(NReqBundleIteratorProto::TBlockIterator* proto) const override {
            for (const auto& iter : Iterators) {
                iter->SerializeToProto(proto->MutableOrderedAndIterator()->AddIterators());
            }
            proto->MutableOrderedAndIterator()->SetNumWords(NumWords);
            proto->MutableOrderedAndIterator()->SetAnyWordsMask(AnyWordsMask);
            proto->SetBlockId(BlockId);
            proto->SetBlockType(BlockType);
        }

    private:
        template <typename IteratorProducer>
        void LoadIterators(ui32 iteratorsSize, IteratorProducer&& producer) {
            size_t lastBlockStart = 0;
            ui32 lastBlockId = 0;
            for (size_t i = 0; i != iteratorsSize; ++i) {
                auto [iteratorPtr, blockId] = producer(i);
                Y_ASSERT(blockId < NumWords);
                Y_ASSERT(lastBlockId <= blockId);

                Iterators.push_back(std::move(iteratorPtr));

                if (lastBlockId != blockId) {
                    IteratorsHeap[lastBlockId].Restart(Iterators.data() + lastBlockStart, i - lastBlockStart, [](TIteratorPtr* it) -> TIterator* { return it->Get(); });
                    for (size_t j = lastBlockId + 1; j < blockId; ++j) {
                        IteratorsHeap[j].Restart(static_cast<TIterator**>(nullptr), 0);
                    }
                    lastBlockId = blockId;
                    lastBlockStart = i;
                }
            }
            IteratorsHeap[lastBlockId].Restart(Iterators.data() + lastBlockStart, iteratorsSize - lastBlockStart, [](TIteratorPtr* it) -> TIterator* { return it->Get(); });
            for (size_t j = lastBlockId + 1; j < NumWords; ++j) {
                IteratorsHeap[j].Restart(static_cast<TIterator**>(nullptr), 0);
            }

        }

        inline bool IsValidBreak(const TPosition& pos, const TSharedSentenceLengths& sentenceLengths) const {
            return sentenceLengths.Enabled() && pos.Break() < sentenceLengths.Size();
        }

        inline void IncrementExpectedPos(TPosition& expectedPos, const TSharedSentenceLengths& sentenceLengths) {
            if (!IsValidBreak(expectedPos, sentenceLengths)) {
                if (Y_LIKELY(expectedPos.WordPosBeg() + 1 <= WORD_LEVEL_Max)) {
                    expectedPos.TWordPosBeg::Set(expectedPos.WordPosBeg() + 1);
                } else {
                    expectedPos.TWordPosBeg::Set(1);
                    expectedPos.TBreak::Set(expectedPos.Break() + 1);
                }
            } else {
                if (expectedPos.WordPosBeg() + 1 > sentenceLengths.GetLength(expectedPos.Break())) {
                    if (expectedPos.Break() + 1 >= sentenceLengths.Size()) {
                        expectedPos.SetInvalid();
                    } else {
                        expectedPos.TWordPosBeg::Set(1);
                        expectedPos.TBreak::Set(expectedPos.Break() + 1);
                    }
                } else {
                    expectedPos.TWordPosBeg::Set(expectedPos.WordPosBeg() + 1);
                }
            }
        }

        void Init(const TIteratorOptions& options, TSharedIteratorData& sharedData) {
            Y_ASSERT(options.FetchLimit > 0);
            TPosition* beg = sharedData.HitsStorage.Reserve(options.FetchLimit);
            TPosition* end = beg + (options.FetchLimit - 1);
            CurrentPtr = beg;

            TPosition expectedPos;
            expectedPos.Clear();

            while (IteratorsHeap[0].Valid()) {
                TPosition pos = IteratorsHeap[0].Current();
                IteratorsHeap[0].Next();
                if (pos.Break() < expectedPos.Break() || (pos.Break() == expectedPos.Break() && pos.WordPosBeg() < expectedPos.WordPosBeg())) {
                    continue;
                }
                expectedPos.TBreak::Set(pos.Break());
                expectedPos.TWordPosBeg::Set(pos.WordPosBeg());
                IncrementExpectedPos(expectedPos, sharedData.SharedSentenceLengths);
                bool failed = false;
                bool emptyWordHits = false;
                for (size_t i = 1; i < NumWords; ++i) {
                    if (!expectedPos.Valid()) {
                        failed = true;
                        break;
                    }
                    if ((AnyWordsMask >> i) & 1) {
                        IncrementExpectedPos(expectedPos, sharedData.SharedSentenceLengths);
                        continue;
                    }
                    TPosition curPos = IteratorsHeap[i].Current();
                    while (curPos.Valid()) {
                        if (expectedPos.Break() < curPos.Break() || (expectedPos.Break() == curPos.Break() && expectedPos.WordPosBeg() <= curPos.WordPosBeg())) {
                            break;
                        }
                        IteratorsHeap[i].Next();
                        curPos = IteratorsHeap[i].Current();
                    }
                    if (curPos.Valid() && curPos.Break() == expectedPos.Break() && curPos.WordPosBeg() == expectedPos.WordPosBeg()) {
                        pos.TRelev::SetMin(curPos);
                        pos.TMatch::SetMax(curPos);
                        if (Y_UNLIKELY(pos.Break() != curPos.Break())) {
                            if (IsValidBreak(pos, sharedData.SharedSentenceLengths)) {
                                pos.TWordPosEnd::Set(sharedData.SharedSentenceLengths.GetLength(pos.Break()));
                            } else {
                                pos.TWordPosEnd::Set(pos.WordPosBeg());
                            }
                        } else {
                            pos.TWordPosEnd::Set(curPos.WordPosEnd());
                        }
                        IncrementExpectedPos(expectedPos, sharedData.SharedSentenceLengths);
                    } else {
                        if (!curPos.Valid()) {
                            emptyWordHits = true;
                        }
                        failed = true;
                        expectedPos.TBreak::Set(pos.Break());
                        expectedPos.TWordPosBeg::Set(0);
                        break;
                    }
                }
                if (!failed) {
                    pos.TLemmId::Set(0);
                    pos.TLowLevelFormId::Set(0);
                    pos.TBlockId::Set(BlockId);
                    if (beg < end) {
                        beg[0] = pos;
                        ++beg;
                    } else {
                        break;
                    }
                } else if (emptyWordHits) {
                    break;
                }
            }
            beg[0].SetInvalid();
            sharedData.HitsStorage.Advance(beg + 1 - CurrentPtr);
            Y_ASSERT(CheckOrder(CurrentPtr, NReqBundleIterator::TLemmId::UpperMask));
        }

        void SaveIteratorImpl(IOutputStream* rh) const override {
            // Header
            ui8 type = ItOrderedAnd;
            SavePodType(rh, type);
            ui32 numWords = NumWords;
            SavePodType(rh, numWords);
            // Body
            SavePodType(rh, AnyWordsMask);
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
    inline THolder<TBasicIteratorAnd, TDestructor> MakeIteratorOrderedAnd(
        size_t numWords,
        ui32 blockId,
        TMemoryPool& iteratorsMemory,
        Args&&... args)
    {
        if (numWords <= 16) {
            return THolder<TBasicIteratorAnd, TDestructor>(iteratorsMemory.New<TIteratorOrderedAnd<16>>(blockId, numWords, iteratorsMemory, std::forward<Args>(args)...));
        }
        if (numWords <= 64) {
            return THolder<TBasicIteratorAnd, TDestructor>(iteratorsMemory.New<TIteratorOrderedAnd<64>>(blockId, numWords, iteratorsMemory, std::forward<Args>(args)...));
        }
        return nullptr;
    }
} // NReqBundleIteratorImpl
