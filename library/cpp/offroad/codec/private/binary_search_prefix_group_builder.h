#pragma once

#include "histogram.h"
#include "prefix_group.h"

namespace NOffroad {
    namespace NPrivate {
        class TBinarySearchPrefixGroupBuilder {
        public:
            TBinarySearchPrefixGroupBuilder(const THistogram* histogram)
                : Histogram_(histogram)
            {
            }

            void BuildGroups(ui32 bits, ui32 groupsCount, TPrefixGroupList* groupsList) const {
                Y_ENSURE(bits <= 32);
                Y_ENSURE(bits + 1 <= groupsCount);

                TWeightedPrefixGroupSet groups;
                groups.AddZeroGroups(bits);

                double lo = 0.0;
                double hi = 1.0;

                size_t step = 0;

                TWeightedPrefixGroupSet tmp;
                while (++step < 20) {
                    double frequency = (lo + hi) * 0.5;
                    tmp.clear();
                    Histogram_->Reduce(frequency, &tmp);
                    tmp.AddZeroGroups(bits);

                    if (tmp.size() > groupsCount) {
                        lo = frequency;
                    } else {
                        hi = frequency;
                        tmp.swap(groups);
                    }
                }
                Y_ASSERT(groups.size() <= groupsCount);

                groups.BuildSortedGroups(groupsList);
            }

        private:
            const THistogram* Histogram_ = nullptr;
        };

    }
}
