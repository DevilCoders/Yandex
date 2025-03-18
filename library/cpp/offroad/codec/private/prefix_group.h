#pragma once

#include <util/digest/multi.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/ylimits.h>
#include <util/system/yassert.h>
#include <util/ysaveload.h>

#include <library/cpp/vec4/vec4.h>

#include "utility.h"

namespace NOffroad {
    namespace NPrivate {
        /**
         * Encodes a prefix group.
         */
        class TPrefixGroup {
        public:
            TPrefixGroup() {
            }

            TPrefixGroup(const TVec4u& prefix, ui8 level)
                : Prefix_(prefix)
                , Level_(level)
            {
                Y_ASSERT(CanFold(prefix));
            }

            const TVec4u& Prefix() const {
                return Prefix_;
            }

            ui8 Level() const {
                return Level_;
            }

            void Save(IOutputStream* stream) const {
                ::Save(stream, Level_);
                ::Save(stream, ToUI64(Prefix_));
            }

            void Load(IInputStream* stream) {
                ::Load(stream, Level_);
                ui64 prefix;
                ::Load(stream, prefix);
                Prefix_ = FromUI64(prefix);
            }

            friend bool operator==(const TPrefixGroup& l, const TPrefixGroup& r) {
                return l.Level_ == r.Level_ && l.Prefix_ == r.Prefix_;
            }

        private:
            TVec4u Prefix_;
            ui8 Level_ = 0;
        };

        struct TPrefixGroupHasher {
            ui64 operator()(const TPrefixGroup& group) const {
                return MultiHash(
                    static_cast<ui64>(group.Level()),
                    static_cast<ui64>(group.Prefix().Value<0>()),
                    static_cast<ui64>(group.Prefix().Value<1>()),
                    static_cast<ui64>(group.Prefix().Value<2>()),
                    static_cast<ui64>(group.Prefix().Value<3>()));
            }
        };

        class TWeightedPrefixGroup: public TPrefixGroup {
        public:
            TWeightedPrefixGroup() {
            }

            TWeightedPrefixGroup(const TVec4u& prefix, ui8 level, ui64 weight)
                : TPrefixGroup(prefix, level)
                , Weight_(weight)
            {
            }

            ui64 Weight() const {
                return Weight_;
            }

            bool CanMerge(const TWeightedPrefixGroup& other) const {
                return Level() == other.Level() && Prefix() == other.Prefix();
            }

            void Merge(const TWeightedPrefixGroup& other) {
                Y_ASSERT(CanMerge(other));

                Weight_ += other.Weight_;
            }

            friend bool operator<(const TWeightedPrefixGroup& l, const TWeightedPrefixGroup& r) {
                if (l.Level() != r.Level())
                    return l.Level() < r.Level();
                if (l.Weight() != r.Weight())
                    return l.Weight() > r.Weight();
                return ToUI64(l.Prefix()) < ToUI64(r.Prefix());
            }

        private:
            ui64 Weight_ = 0;
        };

        using TPrefixGroupList = TVector<TPrefixGroup>;

        class TWeightedPrefixGroupSet: public THashMap<TPrefixGroup, ui64, TPrefixGroupHasher> {
        public:
            void AddZeroGroups(size_t bits) {
                /* Note that we're assigning max weight to zero groups so that they go
                 * first in list, so that group at index [0] is always an all-zeros group.
                 * Tuple readers/writers rely on that.
                 *
                 * Division by 2 is added so that the counters don't wrap around. */

                for (size_t i = 0; i <= bits; ++i)
                    (*this)[TPrefixGroup(TVec4u(), i)] += Max<ui64>() / 2;
            }

            void BuildSortedGroups(TPrefixGroupList* result) {
                TVector<TWeightedPrefixGroup> weightedGroups;
                weightedGroups.reserve(size());
                for (const auto& entry : *this) {
                    weightedGroups.emplace_back(entry.first.Prefix(), entry.first.Level(), entry.second);
                }
                Sort(weightedGroups);
                result->assign(weightedGroups.begin(), weightedGroups.end());
            }

            static TPrefixGroupList DefaultSortedGroups(size_t bits) {
                TPrefixGroupList result;
                result.reserve(bits + 1);

                for (size_t i = 0; i <= bits; ++i)
                    result.push_back(TPrefixGroup(TVec4u(), i));

                return result;
            }
        };

    }
}
