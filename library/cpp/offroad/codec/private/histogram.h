#pragma once

#include <util/generic/hash.h>
#include <util/ysaveload.h>
#include <util/digest/murmur.h>
#include <util/memory/pool.h>

#include <library/cpp/vec4/vec4.h>

#include "prefix_group.h"

namespace NOffroad {
    namespace NPrivate {
        class THistogram {
        public:
            THistogram(size_t capacity = 0)
                : Pool_(ExpectedMemoryUsage(capacity), TMemoryPool::TLinearGrow::Instance())
                , CountByData_(&Pool_)
            {
            }

            void Add(const TVec4u& data, ui64 count = 1) {
                CountByData_[data] += count;
                TotalCount_ += count;
            }

            bool IsEmpty() const {
                return CountByData_.empty();
            }

            /**
             * Constructs a list of prefix groups from this histogram, with each prefix
             * satisfying the provided minimal frequency requirement.
             */
            void Reduce(double frequency, TWeightedPrefixGroupSet* groups) const {
                TMemoryPool pool(ExpectedMemoryUsage(CountByData_.size()));
                TCountsMap currentCountByData(&pool);
                currentCountByData.insert(CountByData_.begin(), CountByData_.end());
                ui64 currentTotalCount = TotalCount_;
                TCountsMap nextCountByData(&pool);
                nextCountByData.reserve(currentCountByData.size());
                for (ui8 level = 0;; ++level) {
                    nextCountByData.clear();
                    ui64 nextTotalCount = 0;
                    for (const auto& entry : currentCountByData) {
                        const TVec4u& data = entry.first;
                        ui64 count = entry.second;

                        bool isZero = (data == TVec4u());
                        if ((count > frequency * currentTotalCount || isZero) && CanFold(data)) {
                            (*groups)[TPrefixGroup(data, level)] += count;
                        } else {
                            nextCountByData[data >> 1] += count;
                            nextTotalCount += count;
                        }
                    }
                    if (nextCountByData.empty()) {
                        break;
                    }
                    currentCountByData.swap(nextCountByData);
                    currentTotalCount = nextTotalCount;
                }
            }

        private:
            struct THasher {
                ui64 operator()(const TVec4u& slice) const {
                    return MurmurHash<ui64>(&slice, sizeof(slice));
                }
            };

            using TCountsMap = THashMap<TVec4u, ui64, THasher, TEqualTo<TVec4u>, TPoolAllocator>;

            static size_t ExpectedMemoryUsage(size_t capacity) {
                return capacity * 2 * (sizeof(void*)) +
                       capacity * 3 * (sizeof(TVec4u));
            }

            TMemoryPool Pool_;
            TCountsMap CountByData_;
            ui64 TotalCount_ = 0;
        };

    }
}
