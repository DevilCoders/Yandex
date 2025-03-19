#include "fast_distance.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/random/fast.h>
#include <util/random/common_ops.h>
#include <util/generic/ymath.h>

#include <cmath>

Y_UNIT_TEST_SUITE(FastDistanceTest) {
    struct TItem {
        TVector<float> FloatRep;
        TVector<ui8> IntegerRep;

        const float* GetFloatItem() const {
            return FloatRep.data();
        }

        const TArrayRef<const ui8> GetIntegerItem() const {
            return TArrayRef<const ui8>(IntegerRep.data(), IntegerRep.size());
        }
    };

    template <class TGen>
    TItem GenerateRandomItem(const NDssmApplier::EDssmModelType dssmModelType, const size_t dimension, TGen&& gen) {
        TItem item;

        item.FloatRep.reserve(dimension);

        float norm = 0;

        for (size_t i = 0; i < dimension; ++i) {
            const float x = gen.GenRandReal1();
            item.FloatRep.push_back(x);
            norm += x * x;
        }

        norm = std::sqrt(norm);
        for (float& comp : item.FloatRep) {
            comp /= norm;
        }

        const std::pair<float, float> bounds = NDssmApplier::GetBounds(dssmModelType);

        item.IntegerRep = NDssmApplier::NUtils::Compress(item.FloatRep, bounds.first, bounds.second);
        item.FloatRep = NDssmApplier::NUtils::Decompress(item.GetIntegerItem(),
                                                         NDssmApplier::GetDecompression(dssmModelType));

        UNIT_ASSERT_EQUAL(item.IntegerRep, NDssmApplier::NUtils::Compress(item.FloatRep, bounds.first, bounds.second));

        return item;
    }

    class TSlowDistance {
    public:
        using TResult = float;
        using TLess = std::greater<TResult>;

        TSlowDistance(const size_t dimension)
            : Dimension(dimension)
        {
        }

        TResult operator()(const float* a, const float* b) const {
            TResult res = 0;
            TResult normA = 0;
            TResult normB = 0;

            for (size_t i = 0; i < Dimension; ++i) {
                res += a[i] * b[i];
                normA += a[i] * a[i];
                normB += b[i] * b[i];
            }

            // The normalization is not done intentionally:
            // The error here actually changes the order of close distances
            // res /= std::sqrt(normA * normB);

            return res;
        }

    private:
        size_t Dimension = 0;
    };

    template <class TFastDistance>
    void TestFastDistanceIsEquivalentToSlow(const NDssmApplier::EDssmModelType dssmModelType,
                                            const size_t dimension, const size_t numIterations = 2000)
    {
        TFastRng32 gen(17, 0);

        const TFastDistance fastDistance(dssmModelType);
        const TSlowDistance slowDistance(dimension);

        using TResultSlow = TSlowDistance::TResult;
        using TResultFast = typename TFastDistance::TResult;

        for (size_t iteration = 0; iteration < numIterations; ++iteration) {
            const TItem a = GenerateRandomItem(dssmModelType, dimension, gen);
            const TItem b = GenerateRandomItem(dssmModelType, dimension, gen);
            const TItem c = GenerateRandomItem(dssmModelType, dimension, gen);

            const TResultSlow slowAc = slowDistance(a.GetFloatItem(), c.GetFloatItem());
            const TResultSlow slowBc = slowDistance(b.GetFloatItem(), c.GetFloatItem());

            const TResultFast fastAc = fastDistance(a.GetIntegerItem(), c.GetIntegerItem());
            const TResultFast fastBc = fastDistance(b.GetIntegerItem(), c.GetIntegerItem());

            // some tolerance to floating point rounding errors
            if (Abs(fastAc - fastBc) <= 2) {
                continue;
            }

            if ((TSlowDistance::TLess()(slowAc, slowBc) != typename TFastDistance::TLess()(fastAc, fastBc))) {
                Cerr << "Slow dists: " << slowAc << " " << slowBc << ", fast dists: " << fastAc << " " << fastBc << Endl;
            }

            UNIT_ASSERT_EQUAL(TSlowDistance::TLess()(slowAc, slowBc), typename TFastDistance::TLess()(fastAc, fastBc));
        }
    };

    void TestFastDistanceSpecializationsEquivalentToSlow(const NDssmApplier::EDssmModelType dssmModelType,
                                                         const size_t dimension) {
        TestFastDistanceIsEquivalentToSlow<NDssmApplier::NPrivate::TFastDistancePlain>(dssmModelType, dimension);

#if defined(__SSE2__)
        TestFastDistanceIsEquivalentToSlow<NDssmApplier::NPrivate::TFastDistanceSse2>(dssmModelType, dimension);
#endif
    }

    Y_UNIT_TEST(FastDistanceIsEquivalentToSlow) {
        TestFastDistanceSpecializationsEquivalentToSlow(NDssmApplier::EDssmModelType::LogDwellTimeBigrams, 50);
        TestFastDistanceSpecializationsEquivalentToSlow(NDssmApplier::EDssmModelType::AnnRegStats, 50);
        TestFastDistanceSpecializationsEquivalentToSlow(NDssmApplier::EDssmModelType::DssmBoostingEmbeddings, 50);
        TestFastDistanceSpecializationsEquivalentToSlow(NDssmApplier::EDssmModelType::MainContentKeywords, 50);
        TestFastDistanceSpecializationsEquivalentToSlow(NDssmApplier::EDssmModelType::MarketMiddleClick, 50);
        TestFastDistanceSpecializationsEquivalentToSlow(NDssmApplier::EDssmModelType::MarketHard, 50);
    }
}
