#pragma once

#include <kernel/doom/offroad_keyinv_wad/generic_hit_layers_range.h>

#include <library/cpp/offroad/offset/data_offset.h>

namespace NDoom {


struct TDoublePantherHitRange: public TGenericHitLayersRange<NOffroad::TDataOffset, 2> {
    using TPosition = NOffroad::TDataOffset;

    Y_FORCE_INLINE const TPosition& Start() const {
        return (*this)[0];
    }

    Y_FORCE_INLINE void SetStart(const TPosition& position) {
        (*this)[0] = position;
    }

    Y_FORCE_INLINE const TPosition& Mid() const {
        return (*this)[1];
    }

    Y_FORCE_INLINE void SetMid(const TPosition& position) {
        (*this)[1] = position;
    }

    Y_FORCE_INLINE const TPosition& End() const {
        return (*this)[2];
    }

    Y_FORCE_INLINE void SetEnd(const TPosition& position) {
        (*this)[2] = position;
    }
};


} // namespace NDoom
