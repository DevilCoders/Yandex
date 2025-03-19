#include "break_word_accumulator.h"

namespace NTrOverReqBundleIterator {
    namespace NPrivate {
        void TBreakWordAccumulator::Reset(
            TArrayRef<TFullPositionEx> hitsBuf,
            ui64* wordMask,
            TArrayRef<ui32> formCounts)
        {
            HitsBuf = hitsBuf;
            WordMask = wordMask;
            FormCounts = formCounts;

            TrPosPtr = 0;
            RelevsMask = 0;
            CurBreak = Max<ui32>();
            CurWord = Max<ui32>();
            for (ui32 relev = 0; relev < RelevValuesCount; ++relev) {
                RelevWordsMask[relev] = 0;
                for (ui32 wordIdx = 0; wordIdx < LimitedWordCount; ++wordIdx) {
                    RelevPositions[relev][wordIdx].WordIdx = -1;
                }
            }
            ++CurrentVersion;
            FillN(BlockLowLevelFormCounts.data(), BlockLowLevelFormCounts.size(), 0);
        }

        void TBreakWordAccumulator::AddReqBundlePosition(const NReqBundleIterator::TPosition& pos) {
            if (TrPosPtr >= HitsBuf.size()) {
                return;
            }

            ui32 breakId = pos.Break();
            ui32 wordBeg = pos.WordPosBeg();
            if (breakId != CurBreak || wordBeg != CurWord) {
                if (CurBreak != Max<ui32>()) {
                    Y_ASSERT(CurBreak < breakId || (CurBreak == breakId && CurWord < wordBeg));
                }
                Flush();
                CurBreak = breakId;
                CurWord = wordBeg;
            }

            ui32 blockId = pos.BlockId();

            if (blockId < BlockTrIteratorWordIdxs.size()
                && !BlockTrIteratorWordIdxs[blockId].empty()) {
                ui32 relev = pos.Relev();
                Y_ASSERT(relev < RelevValuesCount);
                RelevsMask |= (ui32(1) << relev);

                i32 beg = 0;
                TWordPosition::SetBreak(beg, breakId);
                TWordPosition::SetWord(beg, wordBeg);
                TWordPosition::SetRelevLevel(beg, relev);

                /**
                 *  It's ok, see filter_ops.cpp
                 *  EFormClass formClass = ...
                 *  TWordPosition::SetWordForm(wordPos, formClass);
                 */
                i32 end = beg;
                TWordPosition::SetWordForm(end, pos.Match());
                TWordPosition::SetWord(end, pos.WordPosEnd());

                ui32 lowLevelFormId = pos.LowLevelFormId();
                auto& formVersions = BlockLowLevelFormVersions[blockId];
                if (Y_UNLIKELY(lowLevelFormId >= formVersions.size())) {
                    formVersions.resize(lowLevelFormId + 1, 0);
                    formVersions[lowLevelFormId] = CurrentVersion;
                    ++BlockLowLevelFormCounts[blockId];
                } else if (formVersions[lowLevelFormId] != CurrentVersion) {
                    formVersions[lowLevelFormId] = CurrentVersion;
                    ++BlockLowLevelFormCounts[blockId];
                }

                for (std::pair<ui32, EFormClass> wordAndForm : BlockTrIteratorWordIdxs[blockId]) {
                    Y_ASSERT(wordAndForm.first < LimitedWordCount);
                    TFullPositionEx& curPos = RelevPositions[relev][wordAndForm.first];
                    i32 wordEnd = end;
                    TWordPosition::SetWordForm(wordEnd, Max(pos.Match(), wordAndForm.second));
                    if (curPos.WordIdx == -1) {
                        RelevWordsMask[relev] |= (ui64(1) << wordAndForm.first);
                        curPos.WordIdx = wordAndForm.first;
                        curPos.Pos.Beg = beg;
                        curPos.Pos.End = wordEnd;
                    } else {
                        Y_ASSERT(curPos.Pos.Beg == beg);
                        curPos.Pos.End = (Max(curPos.Pos.End ^ NFORM_LEVEL_Max, wordEnd ^ NFORM_LEVEL_Max) ^ NFORM_LEVEL_Max);
                    }
                }
            }
        }

        void TBreakWordAccumulator::Finish() {
            Flush();
            if (!FormCounts.empty()) {
                for (ui32 blockId = 0; blockId < BlockTrIteratorWordIdxs.size(); ++blockId) {
                    ui32 formsCount = BlockLowLevelFormCounts[blockId];
                    for (const auto& word : BlockTrIteratorWordIdxs[blockId]) {
                        if (word.first < FormCounts.size()) {
                            FormCounts[word.first] += formsCount;
                        }
                    }
                }
            }
        }

        void TBreakWordAccumulator::Flush() {
            for (ui32 i = 0; i < RelevValuesCount; ++i) {
                ui32 relev = MaskPositions[RelevsMask][i];
                if (relev == CacheBits) {
                    break;
                }
                ui64 wordsMask = RelevWordsMask[relev];
                for (ui32 idxOffset = 0; wordsMask > 0; idxOffset += CacheBits) {
                    ui32 cacheMask = (wordsMask & CacheMask);
                    for (ui32 j = 0; j < CacheBits; ++j) {
                        ui32 wordIdx = MaskPositions[cacheMask][j];
                        if (wordIdx == CacheBits) {
                            break;
                        }
                        Flush(RelevPositions[relev][wordIdx + idxOffset]);
                    }
                    wordsMask >>= CacheBits;
                }
                RelevWordsMask[relev] = 0;
            }
            RelevsMask = 0;
        }
    }
} // namespace NTrOverReqBundleIterator
