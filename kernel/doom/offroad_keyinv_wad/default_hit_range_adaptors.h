#pragma once

#include "default_hit_range.h"

#include <util/system/compiler.h>

namespace NDoom {


template <class Position, class Vectorizer>
struct TDefaultHitRangeVectorizer {
    enum {
        TupleSize = Vectorizer::TupleSize
    };

    using TPosition = Position;
    using TRange = TDefaultHitRange<TPosition>;

    template <class Slice>
    Y_FORCE_INLINE static void Gather(Slice&& slice, TRange* range) {
        Vectorizer::Gather(slice, &(*range)[1]);
    }

    template <class Slice>
    Y_FORCE_INLINE static void Scatter(const TRange& range, Slice&& slice) {
        Vectorizer::Scatter(range.End(), slice);
    }
};


template <class Position, class Serializer>
struct TDefaultHitRangeSerializer {
    enum {
        MaxSize = Serializer::MaxSize
    };

    using TPosition = Position;
    using TRange = TDefaultHitRange<TPosition>;

    Y_FORCE_INLINE static size_t Serialize(const TRange& range, ui8* dst) {
        return Serializer::Serialize(range.End(), dst);
    }

    Y_FORCE_INLINE static size_t Deserialize(const ui8* src, TRange* range) {
        return Serializer::Deserialize(src, &(*range)[1]);
    }
};


template <class Position>
struct TDefaultHitRangeCombiner {
    using TPosition = Position;
    using TRange = TDefaultHitRange<TPosition>;

    static constexpr bool IsIdentity = false;

    Y_FORCE_INLINE static void Combine(const TRange& prev, const TRange& next, TRange* res) {
        res->SetStart(prev.End());
        res->SetEnd(next.End());
    }
};


} // namespace NDoom
