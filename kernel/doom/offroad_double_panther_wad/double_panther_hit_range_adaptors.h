#pragma once

#include <kernel/doom/offroad_keyinv_wad/generic_hit_layers_range_accessor.h>

#include "double_panther_hit_range.h"

namespace NDoom {

/**
 * TDoublePantherHitRange = (start, mid, end)
 * We store only (mid, end) pair in index
 * Combiner constructs (start1, mid1, end1) (start2, mid2, end2) -> (end1, mid2, end2)
 * because end1 always equal start2
 * Vectorizer: (mid, end) -> (mid_high, mid_low, end_high, end_low)
 * Subtractor: (mid1, end1) (mid2, end2) -> (mid2 - end1, end2 - mid2)
 * it's equal to (a, b, c, d) (e, f, g, h) -> (e - c, f - d, g - e, h - f)
 */

namespace NPrivate {

Y_FORCE_INLINE NOffroad::TDataOffset FromEncoded(ui64 position) {
    return NOffroad::TDataOffset::FromEncoded(position);
}

Y_FORCE_INLINE size_t Serialize(ui64 pos, ui8* dst) {
    return NOffroad::TUi64VarintSerializer::Serialize(pos, dst);
}

Y_FORCE_INLINE size_t Deserialize(const ui8* src, ui64* pos) {
    return NOffroad::TUi64VarintSerializer::Deserialize(src, pos);
}

} // namespace NPrivate


struct TDoublePantherHitRangeVectorizer {
    enum {
        TupleSize = 4
    };

    template <class Slice>
    Y_FORCE_INLINE static void Scatter(const TDoublePantherHitRange& range, Slice&& slice) {
        const ui64 firstLayer = range.Mid().ToEncoded();
        const ui64 secondLayer = range.End().ToEncoded();
        slice[0] = (firstLayer >> 32);
        slice[1] = static_cast<ui32>(firstLayer);
        slice[2] = (secondLayer >> 32);
        slice[3] = static_cast<ui32>(secondLayer);
    }

    template <class Slice>
    Y_FORCE_INLINE static void Gather(Slice&& slice, TDoublePantherHitRange* range) {
        const ui64 firstLayer = (static_cast<ui64>(slice[0]) << 32) | slice[1];
        const ui64 secondLayer = (static_cast<ui64>(slice[2]) << 32) | slice[3];
        range->SetMid(NPrivate::FromEncoded(firstLayer));
        range->SetEnd(NPrivate::FromEncoded(secondLayer));
    }
};

struct TDoublePantherHitRangeSubtractor {
    enum {
        TupleSize = 4,
        PrefixSize = 0
    };

    template <class Storage>
    Y_FORCE_INLINE static void Integrate(Storage&&) {

    }

    template<class Value, class Delta, class Next>
    Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
        next[0] = value[2] + delta[0];
        next[1] = delta[0] ? delta[1] : value[3] + delta[1];
        next[2] = next[0] + delta[2];
        next[3] = delta[2] ? delta[3] : next[1] + delta[3];
    }

    template<class Value, class Delta, class Next>
    Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
        delta[0] = next[0] - value[2];
        delta[1] = delta[0] ? next[1] : next[1] - value[3];
        delta[2] = next[2] - next[0];
        delta[3] = delta[2] ? next[3] : next[3] - next[1];
    }
};

struct TDoublePantherHitRangeSerializer {
    enum {
        MaxSize = NOffroad::TUi64VarintSerializer::MaxSize * 2
    };

    Y_FORCE_INLINE static size_t Serialize(const TDoublePantherHitRange& range, ui8* dst) {
        const ui64 firstLayer = range.Mid().ToEncoded();
        const ui64 secondLayer = range.End().ToEncoded();
        size_t size = NPrivate::Serialize(firstLayer, dst);
        size += NPrivate::Serialize(secondLayer, dst + size);
        return size;
    }

    Y_FORCE_INLINE static size_t Deserialize(const ui8* src, TDoublePantherHitRange* range) {
        ui64 layer;
        size_t size = NPrivate::Deserialize(src, &layer);
        range->SetMid(NPrivate::FromEncoded(layer));
        size += NPrivate::Deserialize(src + size, &layer);
        range->SetEnd(NPrivate::FromEncoded(layer));
        return size;
    }
};

struct TDoublePantherHitRangeCombiner {
    static constexpr bool IsIdentity = false;

    Y_FORCE_INLINE static void Combine(
        const TDoublePantherHitRange& prev,
        const TDoublePantherHitRange& next,
        TDoublePantherHitRange* range)
    {
        range->SetStart(prev.End());
        range->SetMid(next.Mid());
        range->SetEnd(next.End());
    }
};

using TDoublePantherHitRangeAccessor = TGenericHitLayersRangeAccessor<TDoublePantherHitRange>;

} // namespace NDoom
