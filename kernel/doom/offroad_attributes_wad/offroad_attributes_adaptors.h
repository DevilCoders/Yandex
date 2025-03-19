#pragma once

#include <kernel/doom/offroad_keyinv_wad/default_hit_range.h>

#include <library/cpp/offroad/offset/tuple_sub_offset.h>

#include <util/system/compiler.h>
#include <util/system/types.h>

namespace NDoom {

struct TOffroadAttributesHitRangeVectorizer {
    enum {
        TupleSize = 2 * NOffroad::TTupleSubOffsetVectorizer::TupleSize
    };

    using TRange = TDefaultHitRange<NOffroad::TTupleSubOffset>;

    template <class Slice>
    Y_FORCE_INLINE static void Gather(Slice&& slice, TRange* range) {
        NOffroad::TTupleSubOffsetVectorizer::Gather(slice, &(*range)[0]);

        ui64 encoded = (static_cast<ui64>(slice[3]) << 32) | slice[4];
        range->SetEnd(NOffroad::TTupleSubOffset(NOffroad::TDataOffset::FromEncoded(encoded), slice[5]));
    }

    template <class Slice>
    Y_FORCE_INLINE static void Scatter(const TRange& range, Slice&& slice) {
        NOffroad::TTupleSubOffsetVectorizer::Scatter(range.Start(), slice);

        ui64 data = range.End().Offset().ToEncoded();
        slice[3] = data >> 32;
        slice[4] = data;
        slice[5] = range.End().SubIndex();
    }
};

} // namespace NDoom
