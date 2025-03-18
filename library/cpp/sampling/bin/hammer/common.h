#pragma once

#include <library/cpp/accurate_accumulate/accurate_accumulate.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/random/entropy.h>
#include <util/random/fast.h>

template <template <typename, typename> class S>
static bool DoTest(const TVector<double>& probs, const size_t size, const size_t repeat) {
    Y_ENSURE(size <= probs.size(), "wrong size");
    auto&& sampler = S<double, size_t>{};
    sampler.PushMany(probs.cbegin(), probs.cbegin() + size);
    sampler.template Prepare<TKahanAccumulator<double>>();

    auto&& RNG = TFastRng<ui64>{Seed()};
    auto found = TVector<bool>(size);
    for (auto index = size_t{}, indexEnd = size * repeat; index < indexEnd; ++index) {
        const auto sample = sampler(RNG);
        found[sample] = true;
    }

    return AllOf(found, [](const bool& x) { return x; });
}
