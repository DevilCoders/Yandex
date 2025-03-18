#include "roulette_wheel.h"
#include "stl.h"

#include <library/cpp/accurate_accumulate/accurate_accumulate.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/random/mersenne.h>
#include <util/stream/buffer.h>

template <typename T>
static void DoRouletteWheelTest() {
    const auto probs = TVector<T>{1., 2., 3., 4., 6.};

    auto&& sampler = NSampling::TRouletteWheel<T, size_t>{};
    sampler.Push(probs[0]).Push(probs[1]);
    sampler.PushMany(probs.begin() + 2, probs.end());
    UNIT_ASSERT_VALUES_EQUAL(probs.size(), sampler.Size());
    sampler.Prepare();
    UNIT_ASSERT_VALUES_EQUAL(probs.size(), sampler.Size());
    const auto check = [&probs](const NSampling::TRouletteWheel<T, size_t>& sampler) {
        auto&& RNG = TMersenne<ui64>{24};
        auto found = TVector<bool>(probs.size());
        for (auto index = size_t{0}; index < probs.size() * size_t{1000}; ++index) {
            const auto sample = sampler(RNG);
            UNIT_ASSERT(sample >= 0 && sample < probs.size());
            found[sample] = true;
        }

        // strictly speaking this may be not be true because we are dealing with probabilities
        UNIT_ASSERT(AllOf(found, [](const bool& x) { return x; }));
    };

    check(sampler);

    {
        auto&& buffer = TBufferStream{};
        sampler.Save(&buffer);
        auto&& otherSampler = NSampling::TRouletteWheel<T, size_t>{};
        otherSampler.Load(&buffer);
        check(otherSampler);
    }

    {
        auto&& sampler = NSampling::TRouletteWheel<T, size_t>{};
        UNIT_ASSERT_EXCEPTION(sampler.Prepare(), yexception);
    }

    {
        auto&& sampler = NSampling::TRouletteWheel<T, size_t>{};
        UNIT_ASSERT_EXCEPTION(sampler.Push(0.), yexception);
    }

    {
        auto&& sampler = NSampling::TRouletteWheel<T, size_t>{};
        UNIT_ASSERT_EXCEPTION(sampler.Push(-1.), yexception);
    }
}

template <typename T>
static void DoRouletteWheelWithIDTest() {
    const auto probs = TVector<T>{1., 2., 3., 4., 6.};
    const auto samples = TVector<TString>{"talk", "does", "not", "cook", "rise"};
    Y_ENSURE(probs.size() == samples.size(), "sizes doesn't match");

    auto&& sampler = NSampling::TRouletteWheelWithID<T, TString>{};
    sampler.Push(probs[0], samples[0]).Push(probs[1], samples[1]);
    sampler.PushMany(probs.begin() + 2, probs.end(), samples.begin() + 2);
    UNIT_ASSERT_VALUES_EQUAL(probs.size(), sampler.Size());
    sampler.Prepare();
    UNIT_ASSERT_VALUES_EQUAL(probs.size(), sampler.Size());
    const auto check = [&probs, &samples](const NSampling::TRouletteWheelWithID<T, TString>& sampler) {
        auto&& RNG = TMersenne<ui64>{24};
        auto found = TVector<bool>(probs.size());
        for (auto index = size_t{0}; index < probs.size() * size_t{1000}; ++index) {
            const auto& sample = sampler(RNG);
            const auto sampleIndex = FindIndex(samples, sample);
            UNIT_ASSERT_VALUES_UNEQUAL(NPOS, index);
            found[sampleIndex] = true;
        }

        // strictly speaking this may be not be true because we are dealing with probabilities
        UNIT_ASSERT(AllOf(found, [](const bool& x) { return x; }));
    };

    check(sampler);

    {
        auto&& buffer = TBufferStream{};
        sampler.Save(&buffer);
        auto&& otherSampler = NSampling::TRouletteWheelWithID<T, TString>{};
        otherSampler.Load(&buffer);
        check(otherSampler);
    }

    {
        auto&& sampler = NSampling::TRouletteWheelWithID<T, TString>{};
        UNIT_ASSERT_EXCEPTION(sampler.Prepare(), yexception);
    }

    {
        auto&& sampler = NSampling::TRouletteWheelWithID<T, TString>{};
        UNIT_ASSERT_EXCEPTION(sampler.Push(0., "fail"), yexception);
    }

    {
        auto&& sampler = NSampling::TRouletteWheelWithID<T, TString>{};
        UNIT_ASSERT_EXCEPTION(sampler.Push(-1., "fail"), yexception);
    }
}

Y_UNIT_TEST_SUITE(RouletteWheelTest) {
    Y_UNIT_TEST(TestFloatRouletteWheel) {
        DoRouletteWheelTest<float>();
    }

    Y_UNIT_TEST(TestDoubleRouletteWheel) {
        DoRouletteWheelTest<double>();
    }

    Y_UNIT_TEST(TestLongDoubleRouletteWheel) {
        DoRouletteWheelTest<long double>();
    }

    Y_UNIT_TEST(TestFloatRouletteWheelWithID) {
        DoRouletteWheelWithIDTest<float>();
    }

    Y_UNIT_TEST(TestDoubleRouletteWheelWithID) {
        DoRouletteWheelWithIDTest<double>();
    }

    Y_UNIT_TEST(TestLongDoubleRouletteWheelWithID) {
        DoRouletteWheelWithIDTest<long double>();
    }

    Y_UNIT_TEST(TestPushIntegerWeights) {
        const auto data = TVector<int>{4, 5, 6};
        auto&& sampler = NSampling::TRouletteWheel<double>{};
        sampler.Push(1);
        sampler.PushMany(data.cbegin(), data.cend());
        sampler.Prepare();
    }

    Y_UNIT_TEST(TestSTLWrapper) {
        {
            const auto data = TVector<double>{1., 2., 3., 4.};
            auto&& sampler = NSampling::TSTLWrapper<NSampling::TRouletteWheel>{data.cbegin(),
                                                                               data.cend()};
            auto&& RNG = TMersenne<ui64>{42};
            sampler(RNG);
        }
        {
            auto&& sampler = NSampling::TSTLWrapper<NSampling::TRouletteWheel>{{1., 2., 3., 4.}};
            auto&& RNG = TMersenne<ui64>{42};
            sampler(RNG);
        }
        {
            auto&& sampler = NSampling::TSTLWrapper<NSampling::TRouletteWheel, TKahanAccumulator<double>>{{1., 2., 3., 4.}};
            auto&& RNG = TMersenne<ui64>{42};
            sampler(RNG);
        }
    }
}
