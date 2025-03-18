#include "bit_sampler_16.h"

#include <util/system/yassert.h>

#include "private/encoder_table.h"
#include "private/greedy_prefix_vec4_group_builder.h"
#include "private/histogram.h"

namespace NOffroad {
    TModel64 TBitSampler16::Finish() {
        using TBuilder = NPrivate::TGreedyPrefixVec4GroupBuilder;

        TModel64 result;
        if (IsFinished())
            return result;

        TBuilder builder1;
        for (const TArrayRef<ui32>& chunk : Chunks()) {
            for (size_t j = 0; j < 4; ++j) {
                builder1.Add(TVec4u(&chunk[j * 4]));
            }
        }

        NPrivate::TWeightedPrefixGroupSet weightedGroupSet1;

        builder1.BuildGroups<NPrivate::TPrefixGroup>(32, 512, &weightedGroupSet1);

        weightedGroupSet1.BuildSortedGroups(&result.Groups1_);

        THolder<NPrivate::TEncoderTable> context(new NPrivate::TEncoderTable());
        context->Reset(result.Groups1_);

        TBuilder builder0;
        for (const TArrayRef<ui32>& chunk : Chunks()) {
            ui32 dummy;
            ui32 indices[4];
            for (size_t j = 0; j < 4; ++j)
                context->CalculateParams(TVec4u(&chunk[j * 4]), &dummy, &indices[j]);
            builder0.Add(TVec4u(&indices[0]));
        }

        NPrivate::TWeightedPrefixGroupSet weightedGroupSet0;
        builder0.BuildGroups<NPrivate::TPrefixGroup>(9, 256, &weightedGroupSet0);

        weightedGroupSet0.BuildSortedGroups(&result.Groups0_);

        TStandardSampler::Finish();
        return result;
    }

}
