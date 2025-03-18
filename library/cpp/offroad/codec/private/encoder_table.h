#pragma once

#include <util/system/yassert.h>

#include "prefix_group.h"
#include "utility.h"

namespace NOffroad {
    namespace NPrivate {
        class TEncoderTable {
        public:
            TEncoderTable() {
            }

            void Reset(const TPrefixGroupList& groups) {
                Y_ENSURE(groups.size() * 2 <= Size);

                if (groups.empty()) {
                    /* We're constructing encoder table from an empty model, and this
                     * should still work. So we do the most sensible default. */
                    Reset(TWeightedPrefixGroupSet::DefaultSortedGroups(32));
                    return;
                }

                Levels_.fill(Max<ui16>());
                MaskByLevel_.fill(ui64(-1));
                Initialized_ = true;

                for (ui16 scheme = 0; scheme < groups.size(); ++scheme) {
                    const TPrefixGroup& group = groups[scheme];
                    Insert(ToUI64(group.Prefix()), group.Level(), scheme);
                }
            }

            void CalculateParams(const TVec4u& data, ui32* level, ui32* scheme) const {
                Y_ASSERT(Initialized_); /* Got here? You probably didn't initialize your table with a proper model! See e.g. TTable64::TTable64(const TModel64&). */

                ui16 currentLevel = 0;
                TVec4u tmp = data;
                while (!CanFold(tmp)) {
                    ++currentLevel;
                    tmp = (tmp >> 1);
                }

                ui64 key = ToUI64(tmp);
                while ((MaskByLevel_[currentLevel] & key)) {
                    ++currentLevel;
                    key = (key >> 1) & NPrivate::Mask7fff;
                }

                while (true) {
                    ui16 currentScheme = Find(key, currentLevel);
                    if (currentScheme != Max<ui16>()) {
                        *level = currentLevel;
                        *scheme = currentScheme;
                        return;
                    }
                    ++currentLevel;
                    Y_ENSURE(currentLevel < 42, "Something went COMPLETELY WRONG with your dict");
                    key = (key >> 1) & NPrivate::Mask7fff;
                }
            }

        private:
            enum {
                Size = 1 << 16
            };

            static constexpr ui64 Multiplier = 42424243ULL;
            static constexpr ui64 Modulo = 1000000007ULL;

            Y_FORCE_INLINE ui16 StartPos(ui64 prefix, ui16 level) const {
                return static_cast<ui16>((prefix * Multiplier + level) % Modulo);
            }

            void Insert(ui64 prefix, ui16 level, ui16 scheme) {
                Y_ASSERT(level < 64);
                MaskByLevel_[level] -= (MaskByLevel_[level] & prefix);
                ui16 pos = StartPos(prefix, level);
                while (true) {
                    if (Levels_[pos] == Max<ui16>()) {
                        Prefixes_[pos] = prefix;
                        Levels_[pos] = level;
                        Schemes_[pos] = scheme;
                        return;
                    }
                    Y_ENSURE(Prefixes_[pos] != prefix || Levels_[pos] != level);
                    ++pos;
                }
            }

            Y_FORCE_INLINE ui16 Find(ui64 prefix, ui16 level) const {
                ui16 pos = StartPos(prefix, level);
                while (Levels_[pos] != Max<ui16>()) {
                    if (Levels_[pos] == level && Prefixes_[pos] == prefix) {
                        return Schemes_[pos];
                    }
                    ++pos;
                }
                return Max<ui16>();
            }

            bool Initialized_ = false;
            alignas(16) std::array<ui64, Size> Prefixes_;
            alignas(16) std::array<ui16, Size> Levels_;
            alignas(16) std::array<ui16, Size> Schemes_;
            alignas(16) std::array<ui64, 64> MaskByLevel_;
        };

    }
}
