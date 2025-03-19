#pragma once

#include <array>

namespace NDoom {

template <class Position, size_t layers>
struct TGenericHitLayersRange: public std::array<Position, layers + 1> {
    using TPosition = Position;

    enum {
        Layers = layers
    };

    TGenericHitLayersRange() {
        this->fill(TPosition());
    }
};

} // namespace NDoom
