#pragma once

#include <library/cpp/dot_product/dot_product.h>

#include <library/cpp/hnsw/index_builder/dense_vector_storage.h>

#include <util/generic/buffer.h>
#include <util/generic/yexception.h>
#include <util/random/normal.h>
#include <util/stream/output.h>

#include <cmath>

namespace NKmeansHnsw {
    template <class TVectorComponent, class TGen>
    void GenerateRandomUniformSpherePoint(const size_t dimension, TGen&& gen, TVectorComponent* const vector) {
        for (size_t i = 0; i < dimension; ++i) {
            vector[i] = StdNormalDistribution<TVectorComponent>(gen);
        }

        const TVectorComponent norm = std::sqrt(DotProduct(vector, vector, dimension));

        for (size_t i = 0; i < dimension; ++i) {
            vector[i] /= norm;
        }
    }

    template <class TVectorComponent, class TGen>
    NHnsw::TDenseVectorStorage<TVectorComponent> GenerateRandomUniformSpherePoints(const size_t numPoints, const size_t dimension, TGen&& gen) {
        const size_t length = numPoints * dimension * sizeof(TVectorComponent);
        TBuffer buffer(length);
        buffer.Proceed(length);
        TVectorComponent* const data = reinterpret_cast<TVectorComponent*>(buffer.Data());

        for (size_t i = 0; i < numPoints; ++i) {
            TVectorComponent* const vector = data + dimension * i;
            GenerateRandomUniformSpherePoint(dimension, gen, vector);
        }

        NHnsw::TDenseVectorStorage<TVectorComponent> storage(TBlob::FromBuffer(buffer), dimension);
        return storage;
    }

}
