#include <library/cpp/knn_index/searcher.h>
#include <library/cpp/knn_index/writer.h>

#include <library/cpp/testing/benchmark/bench.h>

#include <util/generic/buffer.h>
#include <util/generic/ptr.h>
#include <util/random/mersenne.h>
#include <util/stream/buffer.h>

template <class TFeatureType>
TVector<TFeatureType> GenerateRandomPoint(size_t dimension) {
    static TMersenne<ui32> rng;
    TVector<TFeatureType> point(dimension);
    for (size_t i = 0; i < dimension; ++i) {
        point[i] = (TFeatureType)rng();
    }
    return point;
}

template <class TFeatureType, size_t clusterCount, size_t pointCount, size_t dimension>
class TBenchmark {
public:
    using TWriter = NKNNIndex::TWriter<TFeatureType>;
    using TSearcher = NKNNIndex::TSearcher<TFeatureType>;

    TBenchmark() {
        TBufferOutput output(Buffer_);
        TWriter writer(dimension, &output);
        size_t clusterSize = pointCount / clusterCount;
        for (size_t clId = 0; clId < clusterCount; ++clId) {
            for (size_t docId = 0; docId < clusterSize; ++docId) {
                TVector<TFeatureType> point = GenerateRandomPoint<TFeatureType>(dimension);
                writer.WriteItem(clId * clusterSize + docId, 0, point.data());
            }
            TVector<TFeatureType> centroid = GenerateRandomPoint<TFeatureType>(dimension);
            writer.WriteCluster(centroid.data());
        }
        writer.Finish();
        Searcher_.Reset(new TSearcher(TBlob::NoCopy(Buffer_.data(), Buffer_.size())));
    }

    template <NKNNIndex::EDistanceType distanceType>
    void Do(size_t topClusterCount, size_t topPointCount, const NBench::NCpu::TParams& iface) const {
        for (size_t iteration = 0; iteration < iface.Iterations(); ++iteration) {
            Y_UNUSED(iteration);
            TVector<TFeatureType> query = GenerateRandomPoint<TFeatureType>(dimension);
            Searcher_->template FindNearestItems<distanceType>(query.data(), topClusterCount, topPointCount);
        }
    }

private:
    TBuffer Buffer_;
    THolder<TSearcher> Searcher_;
};

static const TBenchmark<i8, 10000, 10000000, 100> bench_i8_10K_10M_100;
static const TBenchmark<i8, 10000, 10000000, 112> bench_i8_10K_10M_112;
static const TBenchmark<ui8, 10000, 10000000, 100> bench_ui8_10K_10M_100;
static const TBenchmark<float, 10000, 10000000, 100> bench_float_10K_10M_100;

Y_CPU_BENCHMARK(i8_10K_10M_100_dot_product, iface) {
    bench_i8_10K_10M_100.Do<NKNNIndex::EDistanceType::DotProduct>(10, 50, iface);
}

Y_CPU_BENCHMARK(i8_10K_10M_112_dot_product, iface) {
    bench_i8_10K_10M_112.Do<NKNNIndex::EDistanceType::DotProduct>(10, 50, iface);
}

Y_CPU_BENCHMARK(i8_100K_10M_112_100k_dot_product, iface) {
    bench_i8_10K_10M_112.Do<NKNNIndex::EDistanceType::DotProduct>(100, 50, iface);
}

Y_CPU_BENCHMARK(float_10K_10M_100_dot_product, iface) {
    bench_float_10K_10M_100.Do<NKNNIndex::EDistanceType::DotProduct>(10, 50, iface);
}

Y_CPU_BENCHMARK(ui8_10K_10M_100_l2, iface) {
    bench_ui8_10K_10M_100.Do<NKNNIndex::EDistanceType::L2Distance>(10, 50, iface);
}

Y_CPU_BENCHMARK(ui8_10K_10M_100_l2Sqr, iface) {
    bench_ui8_10K_10M_100.Do<NKNNIndex::EDistanceType::L2SqrDistance>(10, 50, iface);
}

Y_CPU_BENCHMARK(ui8_10K_10M_100_l1, iface) {
    bench_ui8_10K_10M_100.Do<NKNNIndex::EDistanceType::L1Distance>(10, 50, iface);
}
