#pragma once

#include <kernel/reqbundle_iterator/position.h>

#include <ysite/yandex/posfilter/doc_flags.h>
#include <ysite/yandex/posfilter/dochitsbuf.h>

#include <util/generic/array_ref.h>

#include <array>

namespace NTrOverReqBundleIterator {
    namespace NPrivate {
        class TBreakWordAccumulator {
        public:
            enum {
                CacheBits = 5,
                CacheMask = (ui32(1) << CacheBits) - ui32(1),
                CacheMasksCount = (ui32(1) << CacheBits),
                RelevValuesCount = (ui32(1) << RELEV_LEVEL_Bits)
            };

            using TMaskPositions = std::array<std::array<ui32, CacheBits>, CacheMasksCount>;
            using TBlockWordIdxs = TVector<TVector<std::pair<ui32, EFormClass>>>;

            static_assert(RelevValuesCount <= CacheBits, "RelevValuesCount must be <= CacheBits");

            TBreakWordAccumulator(
                ui32 trIteratorWordCount,
                const TBlockWordIdxs& blockTrIteratorWordIdxs)
                : LimitedWordCount(Min<ui32>(trIteratorWordCount, 64))
                , BlockTrIteratorWordIdxs(blockTrIteratorWordIdxs)
                , BlockLowLevelFormCounts(blockTrIteratorWordIdxs.size())
                , BlockLowLevelFormVersions(blockTrIteratorWordIdxs.size())
            {
            }

            void Reset(
                TArrayRef<TFullPositionEx> hitsBuf,
                ui64* wordMask,
                TArrayRef<ui32> formCounts);

            size_t TrPositionsCount() const {
                return TrPosPtr;
            }

            bool CanAddMorePositions() const {
                return TrPosPtr < HitsBuf.size();
            }

            const TBlockWordIdxs& GetBlockTrIteratorWordIdxs() const {
                return BlockTrIteratorWordIdxs;
            }

            void AddReqBundlePosition(const NReqBundleIterator::TPosition& pos);

            void Finish();

        private:
            void Flush();

            Y_FORCE_INLINE void Flush(TFullPositionEx& trPos) {
                Y_ASSERT(trPos.WordIdx >= 0);
                if (Y_LIKELY(TrPosPtr < HitsBuf.size())) {
                    Y_ASSERT(trPos.WordIdx < 64);
                    if (WordMask) {
                        *WordMask |= (ui64(1) << trPos.WordIdx);
                    }
                    HitsBuf[TrPosPtr++] = trPos;
                }
                trPos.WordIdx = -1;
            }

            // C++ SEVENTEEN CONSTEXPR LAMBDAS
            static constexpr TMaskPositions MaskPositions = []() {
                TMaskPositions result = {{}};
                for (ui32 mask = 0; mask < CacheMasksCount; ++mask) {
                    ui32 ptr = 0;
                    for (ui32 pos = 0; pos < CacheBits; ++pos) {
                        if ((mask >> pos) & 1) {
                            result[mask][ptr++] = pos;
                        }
                    }
                    if (ptr < CacheBits) {
                        result[mask][ptr] = CacheBits;
                    }
                }
                return result;
            }();

            ui32 LimitedWordCount = 0;
            TVector<TVector<std::pair<ui32, EFormClass>>> BlockTrIteratorWordIdxs;

            TArrayRef<TFullPositionEx> HitsBuf;
            ui64* WordMask = nullptr;
            TArrayRef<ui32> FormCounts;

            ui32 CurBreak = Max<ui32>();
            ui32 CurWord = Max<ui32>();

            size_t TrPosPtr = 0;

            ui32 RelevsMask = 0;
            std::array<ui64, RelevValuesCount> RelevWordsMask = {{}};
            std::array<std::array<TFullPositionEx, 64>, RelevValuesCount> RelevPositions;

            ui32 CurrentVersion = 0;
            TVector<ui32> BlockLowLevelFormCounts;
            TVector<TVector<ui32>> BlockLowLevelFormVersions;
        };
    }
} // namespace NTrOverReqBundleIterator
