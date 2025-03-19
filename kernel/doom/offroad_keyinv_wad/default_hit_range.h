#pragma once

#include "generic_hit_layers_range.h"

namespace NDoom {


template <class Position>
struct TDefaultHitRange: public TGenericHitLayersRange<Position, 1> {
    using TPosition = Position;

    const TPosition& Start() const {
        return (*this)[0];
    }

    void SetStart(const TPosition& position) {
        (*this)[0] = position;
    }

    const TPosition& End() const {
        return (*this)[1];
    }

    void SetEnd(const TPosition& position) {
        (*this)[1] = position;
    }
};


} // namespace NDoom
