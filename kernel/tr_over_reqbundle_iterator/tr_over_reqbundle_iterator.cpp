#include "tr_over_reqbundle_iterator.h"

namespace NTrOverReqBundleIterator {
    bool TTrOverRBIterator::LoadDocumentHits(
        ui32 docId,
        size_t maxReqBundleHitsCount,
        TDocumentHitsBuf* trHits,
        NReqBundleIterator::TPosBuf* reqBundleHits,
        TTrOverReqBundleHitsBufPool* pool,
        bool useConvert)
    {
        Y_ASSERT(trHits);
        Y_ASSERT(pool);

        trHits->Flags.Clear();

        Y_ASSERT(maxReqBundleHitsCount <= TTrOverReqBundleHitsBufPool::MaxDocHits);

        const size_t reqBundleHitsSpace = Min(pool->GetReqBundleHitsFreeSpaceSize(), maxReqBundleHitsCount);
        const size_t trHitsSpace = pool->GetTrHitsFreeSpaceSize();

        trHits->Pos = pool->GetTrHitPos();
        trHits->HitInfo = pool->GetHitInfoPos();

        TArrayRef<NReqBundleIterator::TPosition> positions =
            TArrayRef<NReqBundleIterator::TPosition>(pool->GetReqBundleHitPos(), reqBundleHitsSpace);

        TArrayRef<ui16> richTreeForms = (reqBundleHits
            ? TArrayRef<ui16>(pool->GetRichTreeFormPos(), reqBundleHitsSpace)
            : TArrayRef<ui16>());

        size_t positionsCount = 0;

        InitForDoc(docId);

        bool success = LoadDocumentHitsPartial(
            docId,
            TArrayRef<TFullPositionEx>(trHits->Pos, trHitsSpace),
            &trHits->Count,
            &trHits->WordMask,
            positions,
            &positionsCount,
            TArrayRef<ui32>(trHits->Flags.FormCounts),
            richTreeForms,
            useConvert);

        if (reqBundleHits) {
            reqBundleHits->Pos = positions.data();
            reqBundleHits->Count = positionsCount;
            reqBundleHits->RichTreeForms = richTreeForms.data();
            pool->AdvanceReqBundleHits(positionsCount);
        }

        pool->AdvanceTrHits(trHits->Count);

        if (ConstraintChecker) {
            trHits->IsConstraintsChecked = true;
            trHits->IsConstraintsPassed = success;
        }

        return success;
    }

    bool TTrOverRBIterator::LoadDocumentHitsPartial(
        ui32 docId,
        TArrayRef<TFullPositionEx> trHitsBuf,
        int* trHitsCount,
        ui64* wordMask,
        TArrayRef<NReqBundleIterator::TPosition> reqBundleHitsBuf,
        size_t* reqBundleHitsCount,
        TArrayRef<ui32> formCounts,
        TArrayRef<ui16> richTreeForms,
        bool useConvert)
    {
        Y_ASSERT(trHitsCount);
        Y_ASSERT(wordMask);

        Y_ENSURE(richTreeForms.empty() || richTreeForms.size() == reqBundleHitsBuf.size());

        *trHitsCount = 0;
        *wordMask = 0;

        if (reqBundleHitsCount) {
            *reqBundleHitsCount = 0;
        }

        bool success = true;

        if (!Iterator) { // no words -> no positions
            if (ConstraintChecker) {
                TArrayRef<NReqBundleIterator::TPosition> emptyPositions;
                TSentenceLengths emptySentenceLengths;
                success = ConstraintChecker->Validate(emptyPositions, emptySentenceLengths);
            }
            return success; 
        }

        size_t count = Iterator->GetDocumentPositionsPartial(
            reqBundleHitsBuf.data(),
            reqBundleHitsBuf.size(),
            nullptr);

        if (useConvert) {
            Accumulator.Reset(trHitsBuf, wordMask, formCounts);

            for (size_t i = 0; i < count && Accumulator.CanAddMorePositions(); ++i) {
                Accumulator.AddReqBundlePosition(reqBundleHitsBuf[i]);
            }
            Accumulator.Finish();
            *trHitsCount = Accumulator.TrPositionsCount();
        } else {
            const NPrivate::TBreakWordAccumulator::TBlockWordIdxs& wordsIdxs = Accumulator.GetBlockTrIteratorWordIdxs();
            for (size_t i = 0; i < count; ++i) {
                ui32 blockId = reqBundleHitsBuf[i].BlockId();
                if (Y_LIKELY(blockId < wordsIdxs.size())) {
                    for (std::pair<ui32, EFormClass> wordAndForm : wordsIdxs[blockId]) {
                        *wordMask |= (ui64(1) << wordAndForm.first);
                    }
                }
            }
        }

        if (!ConstraintChecker) {
            if (!BlockMustNot.empty()) {
                for (size_t i = 0; i < count; ++i) {
                    const auto& pos = reqBundleHitsBuf[i];
                    if (Y_LIKELY(pos.BlockId() < BlockMustNot.size()) && BlockMustNot[pos.BlockId()]) {
                        *trHitsCount = 0;
                        *wordMask = 0;
                        count = 0;
                        success = false;
                        break;
                    }
                }
            }
        } else {
            TArrayRef<NReqBundleIterator::TPosition> positions = reqBundleHitsBuf.Slice(0, count);
            TSentenceLengths sentenceLengths;
            if (ConstraintChecker->GetNeedSentenceLengths() && SentenceLengthsReader) {
                SentenceLengthsReader->Get(docId, &sentenceLengths, SentenceLengthsPreLoader.Get());
            }
            success = ConstraintChecker->Validate(positions, sentenceLengths);
        }

        if (!richTreeForms.empty()) {
            for (size_t i = 0; i < count; ++i) {
                richTreeForms[i] = Iterator->GetRichTreeFormId(
                    reqBundleHitsBuf[i].BlockId(),
                    reqBundleHitsBuf[i].LowLevelFormId());
            }
        }

        if (reqBundleHitsCount) {
            *reqBundleHitsCount = count;
        }

        return success;
    }
} // namespace NReqBundleIterator
