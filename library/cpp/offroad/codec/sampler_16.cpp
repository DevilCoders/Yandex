#include "sampler_16.h"

#include <util/system/yassert.h>

#include "private/greedy_prefix_vec4_group_builder.h"
#include "private/histogram.h"

namespace NOffroad {
    TModel16 TSampler16::Finish() {
        using TBuilder = NPrivate::TGreedyPrefixVec4GroupBuilder;

        TModel16 result;
        if (IsFinished())
            return result;

        TBuilder builder;
        for (const TArrayRef<ui32>& chunk : Chunks()) {
            for (size_t j = 0; j < 4; ++j) {
                builder.Add(TVec4u(&chunk[j * 4]));
            }
        }

        NPrivate::TWeightedPrefixGroupSet weightedGroupSet;

        builder.BuildGroups<NPrivate::TPrefixGroup>(32, 256, &weightedGroupSet);

        weightedGroupSet.BuildSortedGroups(&result.Groups_);

        TStandardSampler::Finish();
        return result;
    }

}
