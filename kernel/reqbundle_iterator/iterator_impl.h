#pragma once

#include "hits_storage.h"
#include "break_buffer.h"
#include "position.h"
#include "reqbundle_iterator_fwd.h"

#include <kernel/reqbundle_iterator/proto/reqbundle_iterator.pb.h>

#include <kernel/doom/search_fetcher/search_fetcher.h>
#include <kernel/sent_lens/sent_lens.h>

#include <util/memory/pool.h>
#include <util/stream/output.h>
#include <util/generic/cast.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>

class IKeysAndPositions;

namespace NReqBundleIteratorImpl {
    class TIterator;
    class TIteratorSaveLoad;
} // NReqBundleIteratorImpl

namespace NReqBundleIteratorImpl {
    class TSharedSentenceLengths {
    public:
        TSharedSentenceLengths() = default;
        TSharedSentenceLengths(const ISentenceLengthsLenReader* reader)
            : SentenceLengthsReader(reader)
            , SentenceLengthsPreLoader(reader ? reader->CreatePreLoader() : nullptr)
        {
        }

        void AnnounceDocIds(TConstArrayRef<ui32> docIds) {
            if (SentenceLengthsPreLoader) {
                SentenceLengthsPreLoader->AnnounceDocIds(docIds);
            }
        }

        void PreLoadDoc(ui32 docId, const NDoom::TSearchDocLoader& loader) {
            if (SentenceLengthsPreLoader) {
                SentenceLengthsPreLoader->PreLoadDoc(docId, loader);
            }
        }

        void Update(ui32 docId) {
            if (Enabled() && docId != CurrDocId) {
                SentenceLengthsReader->Get(docId, &SentenceLengths, SentenceLengthsPreLoader.Get());
            }
        }

        ui8 GetLength(size_t sentenceNum) const {
            Y_ASSERT(Enabled());
            return SentenceLengths[sentenceNum];
        }

        bool Enabled() const {
            return SentenceLengthsReader != nullptr;
        }

        size_t Size() const {
            return SentenceLengths.size();
        }

        void Reset() {
            CurrDocId = Max<ui32>();
            SentenceLengths.clear();
        }

        private:
            ui32 CurrDocId = Max<ui32>();
            const ISentenceLengthsLenReader* SentenceLengthsReader = nullptr;
            THolder<ISentenceLengthsPreLoader> SentenceLengthsPreLoader;
            TSentenceLengths SentenceLengths;
    };

    class TSharedIteratorData {
    public:
        THitsStorage HitsStorage;
        TSentenceLengths SentenceLengths;
        TSharedSentenceLengths SharedSentenceLengths;
        TBreakBuffer BrkBuffer;

        TSharedIteratorData() = default;
        TSharedIteratorData(const ISentenceLengthsLenReader* sentReader)
            : SharedSentenceLengths(sentReader)
        {
        }

        virtual ~TSharedIteratorData() = default;

        virtual void AnnounceDocIds(TConstArrayRef<ui32> docIds) {
            SharedSentenceLengths.AnnounceDocIds(docIds);
        }

        virtual void PreLoadDoc(ui32 docId, const NDoom::TSearchDocLoader& loader) {
            SharedSentenceLengths.PreLoadDoc(docId, loader);
        }

        virtual void AdviseDocIds(TConstArrayRef<ui32> docIds, std::function<void(ui32)> consumer) {
            for (ui32 id : docIds) {
                consumer(id);
            }
        }

        template<class T>
        T& As() {
            return *CheckedCast<T*>(this);
        }
    };

    enum EIteratorType {
        DefaultIteratorType = 0,
        StopWordIteratorType,
        AttributeIteratorType,
    };

    struct TIteratorDecayInfo {
        ui32 MinDocId = Max<ui32>();
        TIterator* Iterator = nullptr;
    };

    struct TIteratorOptions {
        ui32 FetchLimit = 1000;
        ui32 FirstStageFetchLimit = Max<ui32>();
    };

    struct TSortOrder {
        ui32 TermId = 0;
        ui32 TermRange = 0;
    };

    inline bool operator<(TSortOrder lhs, TSortOrder rhs) {
        return std::tie(lhs.TermId, lhs.TermRange) < std::tie(rhs.TermId, rhs.TermRange);
    }

    class TIterator {
    public:
        enum EIteratorType {
            ItAnd = 0,
            ItOrderedAnd = 1,
            ItIndex = 2,
        };

    public:
        using value_type = NReqBundleIterator::TPosition;

        TIterator(ui32 blockId)
            : BlockId(blockId)
        {
        }

        static TPosition MaxValue() {
            return NReqBundleIterator::TPosition::Invalid;
        }

        Y_FORCE_INLINE NReqBundleIterator::TPosition Current() const {
            return *CurrentPtr;
        }
        Y_FORCE_INLINE void Next() {
            ++CurrentPtr;
            if (Y_UNLIKELY(SecondStagePtr && *CurrentPtr == NReqBundleIterator::TPosition::Invalid)) {
                CurrentPtr = SecondStagePtr;
                SecondStagePtr = nullptr;
            }
        }

        virtual void PrefetchDocs(const TVector<ui32>& /*docIds*/, TSharedIteratorData& /*sharedData*/) {

        }

        virtual void InitForDoc(ui32 docId, bool firstStage, const TIteratorOptions& options, TSharedIteratorData& shared) = 0;
        virtual void LookupNextDoc(ui32& docId) = 0;
        virtual void LookupNextDoc(TIteratorDecayInfo& info) = 0;
        virtual TSortOrder GetSortOrder() const = 0;
        virtual ~TIterator() {}

        virtual void SerializeToProto(NReqBundleIteratorProto::TBlockIterator* proto) const = 0;


        const NReqBundleIterator::TPosition* CurrentPtr = nullptr;
        const NReqBundleIterator::TPosition* SecondStagePtr = nullptr;
        ui32 BlockId = 0; // used to initialize output positions
        NReqBundleIterator::TBlockType BlockType
            = NReqBundleIterator::DefaultBlockType; // abstract caller-provided value for filtering hits

    protected:
        virtual void SaveIteratorImpl(IOutputStream* rh) const = 0;

        friend class TIteratorSaveLoad;
    };

    using TIteratorPtr = THolder<TIterator, TDestructor>;
    using TIteratorPtrs = TVector<TIteratorPtr>;

    using TPositionTemplates = TVector<NReqBundleIterator::TPosition, TPoolAllocator>;


    bool CheckOrder(const NReqBundleIterator::TPosition *beg, ui64 mask);

    // Accepts pos sequence terminated by invalid position.
    // Returns true iff (pos[i] & mask) are ordered by <=.
    inline bool CheckOrder(
        const NReqBundleIterator::TPosition *beg,
        ui64 mask)
    {
        using namespace NReqBundleIterator;

        TPosition pos;
        pos = 0;
        while (1) {
            TPosition cur = beg[0];
            if (!cur.Valid()) {
                break;
            }
            ++beg;
            if ((cur & mask) < (pos & mask)) {
                return false;
            }
            pos = cur;
        }
        return true;
    }

    enum class EBreakBufferMode {
        Ignore,
        Fill,
        Use,
    };

    struct TFetchHitsResult {
        const NReqBundleIterator::TPosition* Start = nullptr;
        bool Truncated = false;
    };

    // Load positions supplied by fetchFunc into shared buffer.
    // All positions have the same blockId.
    template <class Hit, EIteratorType IteratorType, EBreakBufferMode breakBufferMode, class T>
    Y_FORCE_INLINE TFetchHitsResult FetchAndFilterHits(
        size_t fetchLimit,
        TSharedIteratorData& sharedData,
        const TPositionTemplates& templates,
        ui32 blockId,
        const T& fetchFunc)
    {
        using namespace NReqBundleIterator;

        Y_ASSERT(fetchLimit > 0);

        TPosition* beg = sharedData.HitsStorage.Reserve(fetchLimit);
        TPosition* end = beg + (fetchLimit - 1);
        TPosition* cur = beg;
        TFetchHitsResult result = {beg, false};

        while (1) {
            Hit indexPos;
            if (!fetchFunc(&indexPos)) {
                break;
            }
            if (breakBufferMode == EBreakBufferMode::Use && sharedData.BrkBuffer.EmptyBrk(indexPos.Break())) {
                continue;
            }
            TPosition res;
            if (IteratorType == AttributeIteratorType) {
                res.Clear();
            } else {
                // random basesearch cores from index
                if (indexPos.Form() >= templates.size()) {
                    continue;
                }
                res = templates[indexPos.Form()];
                if (!res.Valid()) {
                    continue;
                }
            }
            if (breakBufferMode == EBreakBufferMode::Fill) {
                sharedData.BrkBuffer.SetBrk(indexPos.Break());
            }

            res.TRelev::SetClean(indexPos.Relevance());
            res.TWordPosBeg::SetClean(indexPos.Word());
            res.TWordPosEnd::SetClean(indexPos.Word());
            res.TBreak::SetClean(indexPos.Break());
            res.TBlockId::SetClean(blockId);
            if (cur < end) {
                cur[0] = res;
                ++cur;
            } else {
                result.Truncated = true;
                break;
            }
        }
        cur[0].SetInvalid();
        sharedData.HitsStorage.Advance(cur + 1 - beg);
        Y_ASSERT(CheckOrder(beg, TRelev::UpperMask));
        return result;
    }
} // NReqBundleIteratorImpl
