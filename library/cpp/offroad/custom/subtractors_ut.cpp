#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/offroad/tuple/tuple_storage.h>

#include <array>

#include "subtractors.h"

#define DEFAULT_VALUE 1
#define DEFAULT_DELTA 1

using namespace NOffroad;

Y_UNIT_TEST_SUITE(TIntegrateTest) {
    template <size_t size>
    void TestIntegrate() {
        std::array<ui32, size> diff;
        for (size_t i = 0; i < diff.size(); ++i) {
            diff[i] = i + 1;
        }
        std::array<ui32, size> copy = diff;
        for (size_t i = 1; i < copy.size(); ++i) {
            copy[i] += copy[i - 1];
        }
        Integrate1(diff);
        UNIT_ASSERT_VALUES_EQUAL(diff, copy);
    }
    Y_UNIT_TEST(SSEIntegrateTest) {
        TestIntegrate<1>();
        TestIntegrate<3>();
        TestIntegrate<4>();
        TestIntegrate<5>();
        TestIntegrate<6>();
        TestIntegrate<8>();
        TestIntegrate<1024>();
        TestIntegrate<1023>();
    }
}

Y_UNIT_TEST_SUITE(TSubtractorTest) {
    template <class Subtractor, size_t TupleSize>
    void DoDifferentiateTest(
        std::array<ui32, TupleSize> value,
        std::array<ui32, TupleSize> next,
        std::array<ui32, TupleSize> delta) {
        TTupleStorage<1, TupleSize> tmp;
        Subtractor::Differentiate(value, next, tmp.Slice(0));

        for (size_t i = 0; i < TupleSize; i++)
            UNIT_ASSERT_VALUES_EQUAL(delta[i], tmp(0, i));
    }

    template <class Subtractor, size_t TupleSize>
    void DoIntegrateTest(
        std::array<ui32, TupleSize> value,
        std::array<ui32, TupleSize> next,
        std::array<ui32, TupleSize> delta) {
        TTupleStorage<2, TupleSize> storage;
        storage.Slice(0).Assign(value);
        storage.Slice(1).Assign(delta);
        Subtractor::Integrate(storage);

        TTupleStorage<1, TupleSize> tmp;
        tmp.Slice(0).Assign(value);
        Subtractor::Integrate(value, storage.Slice(1), tmp.Slice(0));

        for (size_t i = 0; i < TupleSize; i++)
            UNIT_ASSERT_VALUES_EQUAL(next[i], tmp(0, i));
    }

    template <class Subtractor, size_t TupleSize>
    void DoSubtractorTest(size_t d = 0) {
        std::array<ui32, TupleSize> value;
        for (size_t i = 0; i < TupleSize; i++)
            value[i] = DEFAULT_DELTA;

        for (size_t x = 0; x < 1 << TupleSize; x++) {
            std::array<ui32, TupleSize> next;
            std::array<ui32, TupleSize> delta;
            bool isDiff = true;
            for (size_t i = 0; i < TupleSize; i++) {
                next[i] = x & (1 << i) ? value[i] + DEFAULT_DELTA : value[i];
                delta[i] = i < d && (isDiff &= i == 0 || !delta[i - 1]) ? next[i] - value[i] : next[i];
            }

            DoDifferentiateTest<Subtractor, TupleSize>(value, next, delta);
            DoIntegrateTest<Subtractor, TupleSize>(value, next, delta);
        }
    }

    template <class Subtractor>
    void DoSubtractorTest(size_t d = 0) {
        DoSubtractorTest<Subtractor, Subtractor::TupleSize>(d);
    }

    Y_UNIT_TEST(TINSubtractor_DoSubtractorTest) {
        DoSubtractorTest<TINSubtractor, 1>();
        DoSubtractorTest<TINSubtractor, 2>();
        DoSubtractorTest<TINSubtractor, 3>();
        DoSubtractorTest<TINSubtractor, 4>();
    }

    Y_UNIT_TEST(TI1Subtractor_DoSubtractorTest) {
        DoSubtractorTest<TI1Subtractor>();
    }

    Y_UNIT_TEST(TD1Subtractor_DoSubtractorTest) {
        DoSubtractorTest<TD1Subtractor>(1);
    }

    Y_UNIT_TEST(TD1I1Subtractor_DoSubtractorTest) {
        DoSubtractorTest<TD1I1Subtractor>(1);
    }

    Y_UNIT_TEST(TD2Subtractor_DoSubtractorTest) {
        DoSubtractorTest<TD2Subtractor>(2);
    }

    Y_UNIT_TEST(TD2I1Subtractor_DoSubtractorTest) {
        DoSubtractorTest<TD2I1Subtractor>(2);
    }

    Y_UNIT_TEST(TD3I1Subtractor_DoSubtractorTest) {
        DoSubtractorTest<TD3I1Subtractor>(3);
    }

    Y_UNIT_TEST(TD4I1Subtractor_DoSubtractorTest) {
        DoSubtractorTest<TD4I1Subtractor>(4);
    }

    Y_UNIT_TEST(TPD1I1Subtractor_DoSubtractorTest) {
        DoSubtractorTest<TPD1I1Subtractor>(1);
    }

    Y_UNIT_TEST(TPD1D2I1Subtractor_DoSubtractorTest) {
        DoSubtractorTest<TPD1D2I1Subtractor>(3);
    }

    Y_UNIT_TEST(TPD1D3I1Subtractor_DoSubtractorTest) {
        DoSubtractorTest<TPD1D3I1Subtractor>(4);
    }

    Y_UNIT_TEST(TPD2D3I1Subtractor_DoSubtractorTest) {
        DoSubtractorTest<TPD2D3I1Subtractor>(5);
    }
}
