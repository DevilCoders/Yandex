#include "bloomfilter.h"

#include <util/stream/file.h>
#include <library/cpp/testing/unittest/registar.h>

#include <cmath>

constexpr size_t HASH_COUNT = 3;

//test that Filter2 can load file saved from Filter1
//test that error rate of Filter2 is like requested
template <class TFilter>
void TestErrorRateAndSaveLoad(size_t n, double error, bool verifyBitsCount = true) {
    TStringStream stream1, stream2;
    {
        TFilter bf(n, float(error));
        for (ui64 i = 0; i < n; ++i) {
            ui64 j = i * 1000;
            bf.Add(&j, sizeof(j));
        }
        bf.Save(&stream1);
    }

    {
        {
            TFilter bf;
            bf.Load(&stream1);
            bf.Union(bf);
            bf.Intersection(bf);
            bf.Save(&stream2);
        }
        UNIT_ASSERT_VALUES_EQUAL(stream1.Str(), stream2.Str());
        TFilter bf;
        bf.Load(&stream2);

        UNIT_ASSERT(n == 0 || double(fabs(double(bf.EstimateItemNumber() - n))) / n < (0.1 + 10.0 / n + 0.1 / (1.0 - error)));

        size_t falsePositive = 0;
        size_t total = 0;
        ui64 j = 0;
        for (ui64 i = 0; i < 100 * 1000; ++i) {
            bool has = bf.Has(&j, sizeof(j));
            if ((j % 1000) == 0 && j / 1000 < n) {
                Y_ENSURE(has, "test fail for:" << j);
            } else {
                falsePositive += has;
                total += 1;
            }
            j += 1;
        }
        { //test error rate for large n
            size_t realBitCount = bf.GetBitCount();
            if (verifyBitsCount) {
                double theoreticalBitCount = std::max(1.0, -double(n) * log(error) / pow(log(2.0), 2)); //see wikipedia
                // We need epsilon to account for rounding errors in float calculations inside TFilter.
                UNIT_ASSERT(size_t(theoreticalBitCount * (1 - 1e-6)) <= realBitCount);
                UNIT_ASSERT(bf.GetBitCount() <= size_t(theoreticalBitCount) * 2 + 64);
            }

            int k = bf.GetHashCount();
            double expectedError = pow(1.0 - pow(1 - 1.0 / double(realBitCount), double(n) * k), double(k));

            double requestedError = error;
            if (n) {
                UNIT_ASSERT(expectedError <= requestedError * (1.1 + 1.0 / n));
            }
            double e = expectedError;

            double stddev = sqrt(e * (1.0 - double(e)) * total);
            if (n > 100 && error < 0.1) { //probability estimate formulas do no work for small 'n' and error near 1.0
                UNIT_ASSERT(falsePositive <= e * total + 20 * stddev);
            }
        }
        { //test Clear
            for (ui64 i = 0; i < n; ++i) {
                ui64 j = i * 1000;
                UNIT_ASSERT(bf.Has(&j, sizeof(j)));
            }
            UNIT_ASSERT(bf.BitmapOneBitsCount() > 0 || n == 0);
            Y_ENSURE(bf.BitmapOneBitsCount() > 0 || n == 0, "empty");
            bf.Clear();
            UNIT_ASSERT(bf.BitmapOneBitsCount() == 0);
            for (ui64 i = 0; i < n; ++i) {
                ui64 j = ui64(i) * 1000;
                UNIT_ASSERT(!bf.Has(&j, sizeof(j)));
            }
        }
    }
}

class TBloomFilterFixedHashcount: public TBloomFilter {
public:
    TBloomFilterFixedHashcount(size_t elemcount, float error)
        : TBloomFilter(elemcount, HASH_COUNT, error)
    {
    }

    TBloomFilterFixedHashcount() {
    }
};

Y_UNIT_TEST_SUITE(TBloomFilter) {
    Y_UNIT_TEST(TestBloomFilter) {
        TestErrorRateAndSaveLoad<TBloomFilterFixedHashcount>(1000000, 0.0001, false);
        TestErrorRateAndSaveLoad<TBloomFilterFaster>(1000000, 0.0001);
        TestErrorRateAndSaveLoad<TBloomFilter>(1000000, 0.0001);
        TestErrorRateAndSaveLoad<TBloomFilterFixedHashcount>(0, 0.0001, false);
        TestErrorRateAndSaveLoad<TBloomFilterFaster>(0, 0.1);
        TestErrorRateAndSaveLoad<TBloomFilter>(0, 0.1);
        size_t n = 1;
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 8; ++j) {
                double error = pow(0.3, j) * 0.99999;
                TestErrorRateAndSaveLoad<TBloomFilterFixedHashcount>(n, error, false);
                TestErrorRateAndSaveLoad<TBloomFilterFaster>(n, error);
                TestErrorRateAndSaveLoad<TBloomFilter>(n, error);
            }
            n = std::max(n + 1, size_t(n * 1.8));
        }
    }
}
