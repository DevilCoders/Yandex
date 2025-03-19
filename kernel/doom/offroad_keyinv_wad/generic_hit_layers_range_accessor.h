#pragma once

namespace NDoom {

template <class Range>
struct TGenericHitLayersRangeAccessor {
    using TData = Range;
    using TPosition = typename Range::TPosition;

    enum {
        Layers = Range::Layers
    };

    Y_FORCE_INLINE static void AddHit(size_t, TData*) {

    }

    Y_FORCE_INLINE static const TPosition& Position(const TData& data, size_t layer) {
        return data[layer];
    }

    Y_FORCE_INLINE static void SetPosition(size_t layer, const TPosition& position, TData* data) {
        (*data)[layer] = position;
    }
};

} // namespace NDoom
