#pragma once

#include <library/cpp/hnsw/index/dense_vector_distance.h>

#include <util/generic/yexception.h>


namespace NHnswKMeans {


class TFixedDimensionL2SqrDistance: public NHnsw::TL2SqrDistance<float> {
    using TBase = NHnsw::TL2SqrDistance<float>;
public:
    TFixedDimensionL2SqrDistance(ui32 dimension)
        : Dimension_(dimension)
    {

    }

    float operator()(const float* a, const float* b) const {
        return TBase::operator()(a, b, Dimension_);
    }

    float operator()(const float* a, const float* b, ui32 dimension) const {
        Y_ENSURE(dimension == Dimension_);
        return TBase::operator()(a, b, dimension);
    }

private:
    ui32 Dimension_ = 0;
};


}
