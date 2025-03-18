#include "eliasfano.h"
#include <util/random/random.h>
#include <util/stream/buffer.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NSuccinctArrays;

Y_UNIT_TEST_SUITE(TEliasFanoArrayTest) {
    Y_UNIT_TEST(Test1) {
        TVector<ui64> data(1, 0);
        TEliasFanoArray<ui64> ef(data.begin(), data.end());
        for (size_t i = 0; i < data.size(); ++i)
            UNIT_ASSERT_EQUAL(data[i], ef[i]);
    }

    Y_UNIT_TEST(Test2) {
        TVector<ui64> data(1000, 0);
        TEliasFanoArray<ui64> ef(data.begin(), data.end());
        for (size_t i = 0; i < data.size(); ++i)
            UNIT_ASSERT_EQUAL(data[i], ef[i]);
    }

    Y_UNIT_TEST(Test3) {
        TVector<ui64> data(1000, 5);
        TEliasFanoArray<ui64> ef(data.begin(), data.end());
        for (size_t i = 1; i < data.size(); ++i)
            UNIT_ASSERT_EQUAL(data[i], ef[i]);
    }

    Y_UNIT_TEST(Test4) {
        TVector<ui64> data(10000, 0);
        for (size_t i = 1; i < data.size(); ++i)
            data[i] = RandomNumber<ui64>(1024);
        TEliasFanoArray<ui64> ef(data.begin(), data.end()); //, data.front());
        for (size_t i = 0; i < data.size(); ++i)
            UNIT_ASSERT_EQUAL(data[i], ef[i]);
    }

    Y_UNIT_TEST(Test5) {
        TVector<ui64> data(10000, 0);
        for (size_t i = 1; i < data.size(); ++i)
            data[i] = RandomNumber<ui64>(1 << 16);
        TEliasFanoArray<ui64> ef(data.begin(), data.end());
        for (size_t i = 0; i < data.size(); ++i)
            UNIT_ASSERT_EQUAL(data[i], ef[i]);
    }

    Y_UNIT_TEST(Test6) {
        TVector<ui64> data(10000, 23);
        for (size_t i = 1; i < data.size(); ++i)
            data[i] = RandomNumber<ui64>(i) + data[i - 1];
        TEliasFanoArray<ui64> ef(data.begin(), data.end());
        for (size_t i = 0; i < data.size(); ++i)
            UNIT_ASSERT_EQUAL(data[i], ef[i]);
    }

    Y_UNIT_TEST(Test7) {
        ui64 data[] = {1ULL, 10ULL, 100ULL, 1000ULL, (ui64)-100LL, (ui64)-10LL, (ui64)-1LL};
        TEliasFanoArray<ui64> ef(data, data + Y_ARRAY_SIZE(data));
        for (size_t i = 0; i < Y_ARRAY_SIZE(data); ++i)
            UNIT_ASSERT_EQUAL(data[i], ef[i]);
    }

    Y_UNIT_TEST(Test8) {
        ui64 data[] = {1ULL, 10ULL, 100ULL, 1000ULL, (ui64)-100LL, (ui64)-10LL, (ui64)-2LL};
        TEliasFanoMonotoneArray<ui64> ef(data, data + Y_ARRAY_SIZE(data));
        for (size_t i = 0; i < Y_ARRAY_SIZE(data); ++i)
            UNIT_ASSERT_EQUAL(data[i], ef[i]);
    }

    Y_UNIT_TEST(Test9) {
        TVector<ui64> data(10000, 0);
        for (size_t i = 0; i < data.size(); ++i)
            data[i] = RandomNumber<ui64>(1024);
        TEliasFanoArray<ui64> ef(data.begin(), data.end());
        for (size_t i = 0; i < data.size(); ++i)
            UNIT_ASSERT_EQUAL(data[i], ef[i]);
    }

    /*Y_UNIT_TEST(Test10) {
        TVector<ui64> data(10000, 0);
        for (size_t i = 1; i < data.size(); ++i)
            data[i] = RandomNumber<ui64>(1024);
        TEliasFanoArray<ui64> ef;
        ef.Learn(data.begin(), data.end());
        for (size_t i = 0; i < data.size(); ++i)
            ef.Add(data[i]);
        ef.Finish();
        for (size_t i = 0; i < data.size(); ++i)
            UNIT_ASSERT_EQUAL(data[i], ef[i]);
    }*/

    Y_UNIT_TEST(Test11) {
        TVector<ui64> data(1000, 0);
        for (size_t i = 1; i < data.size(); ++i)
            data[i] = RandomNumber<ui64>(i) + data[i - 1];

        TEliasFanoArray<ui64> ef(data.begin(), data.end());
        TBufferStream ss;
        ::Save(&ss, ef);

        TEliasFanoArray<ui64> efSaved;
        ::Load(&ss, efSaved);

        for (size_t i = 0; i < data.size(); ++i)
            UNIT_ASSERT_EQUAL(efSaved[i], ef[i]);
    }
}
