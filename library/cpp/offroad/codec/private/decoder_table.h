#pragma once

#include "prefix_group.h"
#include "utility.h"

#include <array>

namespace NOffroad {
    namespace NPrivate {
        class TDecoderTable {
        public:
            TDecoderTable() {
            }

            void Reset(const TPrefixGroupList& groups) {
                Y_ENSURE(groups.size() <= Size);

#if defined(_msan_enabled_)
                memset(Levels.data(), 0, Size);
#endif

                if (groups.empty()) {
                    /* We're constructing decoder table from an empty model, and this
                     * should still work. So we do the most sensible default. */
                    Reset(TWeightedPrefixGroupSet::DefaultSortedGroups(32));
                    return;
                }

                for (size_t i = 0; i < groups.size(); ++i) {
                    const TPrefixGroup& group = groups[i];
                    Levels[i] = group.Level();
                    Msk[i] = VectorMask(group.Level());
                    Add[i] = group.Prefix() << group.Level();
                }
            }

            template <class BlockInput>
            size_t inline Load(BlockInput* stream, ui32 scheme, ui8 level, TVec4u* result) const {
                size_t bits = stream->Read(result, level);
                *result &= Msk[scheme];
                *result = *result + Add[scheme];
                return bits;
            }

            /**
             * Reads four vectors from the provided block input.
             *
             * Templated to break a dependency on `TBlockInput`.
             */
            template <class BlockInput>
            size_t inline Read1(BlockInput* stream, ui32 scheme, TVec4u* result) const {
                ui8 level = Levels[scheme];
                if (level) {
                    return Load(stream, scheme, level, result);
                } else {
                    *result = Add[scheme];
                    return 0;
                }
            }

            template <class BlockInput>
            size_t inline Read4(BlockInput* stream, TVec4u scheme, TVec4u* r0, TVec4u* r1, TVec4u* r2, TVec4u* r3) const {
                size_t result = 0;

                ui32 schemes[4];
                scheme.Store(schemes);

                const std::array<ui8, 4u> levels{{
                    Levels[schemes[0]],
                    Levels[schemes[1]],
                    Levels[schemes[2]],
                    Levels[schemes[3]],
                }};

                const ui32 sum = levels[0] + levels[1] + levels[2] + levels[3];

                if (sum > 32) {
                    result += Load(stream, schemes[0], levels[0], r0);
                    result += Load(stream, schemes[1], levels[1], r1);
                    result += Load(stream, schemes[2], levels[2], r2);
                    result += Load(stream, schemes[3], levels[3], r3);
                } else if (sum > 0) {
                    TVec4u tmp;
                    result += stream->Read(&tmp, sum);

                    *r0 = (tmp & Msk[schemes[0]]) + Add[schemes[0]];
                    tmp = tmp >> levels[0];

                    *r1 = (tmp & Msk[schemes[1]]) + Add[schemes[1]];
                    tmp = tmp >> levels[1];

                    *r2 = (tmp & Msk[schemes[2]]) + Add[schemes[2]];
                    tmp = tmp >> levels[2];

                    *r3 = (tmp & Msk[schemes[3]]) + Add[schemes[3]];
                } else {
                    *r0 = Add[schemes[0]];
                    *r1 = Add[schemes[1]];
                    *r2 = Add[schemes[2]];
                    *r3 = Add[schemes[3]];
                }
                return result;
            }

        private:
            enum {
                Size = 512
            };

            /** Numbers that are to be added to decompressed bit fields, by index. */
            std::array<TVec4u, Size> Add;

            /** Bitness by index. */
            std::array<ui8, Size> Levels;

            /** Masks for bitness above, by index. Precalculated for speed from Levels. */
            std::array<TVec4u, Size> Msk;
        };

    }
}
