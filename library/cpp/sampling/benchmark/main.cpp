#include <library/cpp/sampling/alias_method.h>
#include <library/cpp/sampling/roulette_wheel.h>
#include <library/cpp/sampling/sampling_tree.h>

#include <library/cpp/accurate_accumulate/accurate_accumulate.h>
#include <library/cpp/testing/benchmark/bench.h>

#include <util/generic/singleton.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>
#include <util/random/fast.h>
#include <util/system/compiler.h>

#include <random>

namespace {
    template <typename TValue, size_t SIZE>
    struct TDataHolder {
        TVector<TValue> Data;
        TDataHolder()
            : Data(SIZE)
        {
            TFastRng<ui64> prng{42 * sizeof(TValue) * SIZE};
            for (auto& value : Data) {
                value = static_cast<TValue>(prng.GenRandReal3());
            }
        }
    };
}

// float

namespace {
    using TDH_F_10 = TDataHolder<float, 10>;
    using TDH_F_100 = TDataHolder<float, 100>;
    using TDH_F_1000 = TDataHolder<float, 1000>;
}

Y_CPU_BENCHMARK(RouletteWheel_Float_10, p) {
    NSampling::TRouletteWheel<float> s;
    s.PushMany(Default<TDH_F_10>().Data.begin(), Default<TDH_F_10>().Data.end());
    s.template Prepare<TKahanAccumulator<float>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(RouletteWheel_Float_100, p) {
    NSampling::TRouletteWheel<float> s;
    s.PushMany(Default<TDH_F_100>().Data.begin(), Default<TDH_F_100>().Data.end());
    s.template Prepare<TKahanAccumulator<float>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(RouletteWheel_Float_1000, p) {
    NSampling::TRouletteWheel<float> s;
    s.PushMany(Default<TDH_F_1000>().Data.begin(), Default<TDH_F_1000>().Data.end());
    s.template Prepare<TKahanAccumulator<float>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(AliasMethod_Float_10, p) {
    NSampling::TAliasMethod<float> s;
    s.PushMany(Default<TDH_F_10>().Data.begin(), Default<TDH_F_10>().Data.end());
    s.template Prepare<TKahanAccumulator<float>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(AliasMethod_Float_100, p) {
    NSampling::TAliasMethod<float> s;
    s.PushMany(Default<TDH_F_100>().Data.begin(), Default<TDH_F_100>().Data.end());
    s.template Prepare<TKahanAccumulator<float>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(AliasMethod_Float_1000, p) {
    NSampling::TAliasMethod<float> s;
    s.PushMany(Default<TDH_F_1000>().Data.begin(), Default<TDH_F_1000>().Data.end());
    s.template Prepare<TKahanAccumulator<float>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(SamplingTree_Float_10, p) {
    const auto& X = Default<TDH_F_10>().Data;
    NSampling::TSamplingTree<float> s{X.size()};
    for (const auto i : xrange(X.size())) {
        s.SetFreq(i, X[i]);
    }

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(SamplingTree_Float_100, p) {
    const auto& X = Default<TDH_F_100>().Data;
    NSampling::TSamplingTree<float> s{X.size()};
    for (const auto i : xrange(X.size())) {
        s.SetFreq(i, X[i]);
    }

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(SamplingTree_Float_1000, p) {
    const auto& X = Default<TDH_F_1000>().Data;
    NSampling::TSamplingTree<float> s{X.size()};
    for (const auto i : xrange(X.size())) {
        s.SetFreq(i, X[i]);
    }

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(STLDiscreteDistribution_Float_10, p) {
    std::discrete_distribution<> s{Default<TDH_F_10>().Data.begin(), Default<TDH_F_10>().Data.end()};

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(STLDiscreteDistribution_Float_100, p) {
    std::discrete_distribution<> s{Default<TDH_F_100>().Data.begin(), Default<TDH_F_100>().Data.end()};

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(STLDiscreteDistribution_Float_1000, p) {
    std::discrete_distribution<> s{Default<TDH_F_1000>().Data.begin(), Default<TDH_F_1000>().Data.end()};

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

// double

namespace {
    using TDH_D_10 = TDataHolder<double, 10>;
    using TDH_D_100 = TDataHolder<double, 100>;
    using TDH_D_1000 = TDataHolder<double, 1000>;
}

Y_CPU_BENCHMARK(RouletteWheel_Double_10, p) {
    NSampling::TRouletteWheel<double> s;
    s.PushMany(Default<TDH_D_10>().Data.begin(), Default<TDH_D_10>().Data.end());
    s.template Prepare<TKahanAccumulator<double>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(RouletteWheel_Double_100, p) {
    NSampling::TRouletteWheel<double> s;
    s.PushMany(Default<TDH_D_100>().Data.begin(), Default<TDH_D_100>().Data.end());
    s.template Prepare<TKahanAccumulator<double>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(RouletteWheel_Double_1000, p) {
    NSampling::TRouletteWheel<double> s;
    s.PushMany(Default<TDH_D_1000>().Data.begin(), Default<TDH_D_1000>().Data.end());
    s.template Prepare<TKahanAccumulator<double>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(AliasMethod_Double_10, p) {
    NSampling::TAliasMethod<double> s;
    s.PushMany(Default<TDH_D_10>().Data.begin(), Default<TDH_D_10>().Data.end());
    s.template Prepare<TKahanAccumulator<double>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(AliasMethod_Double_100, p) {
    NSampling::TAliasMethod<double> s;
    s.PushMany(Default<TDH_D_100>().Data.begin(), Default<TDH_D_100>().Data.end());
    s.template Prepare<TKahanAccumulator<double>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(AliasMethod_Double_1000, p) {
    NSampling::TAliasMethod<double> s;
    s.PushMany(Default<TDH_D_1000>().Data.begin(), Default<TDH_D_1000>().Data.end());
    s.template Prepare<TKahanAccumulator<double>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(SamplingTree_Double_10, p) {
    const auto& X = Default<TDH_D_10>().Data;
    NSampling::TSamplingTree<double> s{X.size()};
    for (const auto i : xrange(X.size())) {
        s.SetFreq(i, X[i]);
    }

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(SamplingTree_Double_100, p) {
    const auto& X = Default<TDH_D_100>().Data;
    NSampling::TSamplingTree<double> s{X.size()};
    for (const auto i : xrange(X.size())) {
        s.SetFreq(i, X[i]);
    }

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(SamplingTree_Double_1000, p) {
    const auto& X = Default<TDH_D_1000>().Data;
    NSampling::TSamplingTree<double> s{X.size()};
    for (const auto i : xrange(X.size())) {
        s.SetFreq(i, X[i]);
    }

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(STLDiscreteDistribution_Double_10, p) {
    std::discrete_distribution<> s{Default<TDH_D_10>().Data.begin(), Default<TDH_D_10>().Data.end()};

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(STLDiscreteDistribution_Double_100, p) {
    std::discrete_distribution<> s{Default<TDH_D_100>().Data.begin(), Default<TDH_D_100>().Data.end()};

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(STLDiscreteDistribution_Double_1000, p) {
    std::discrete_distribution<> s{Default<TDH_D_1000>().Data.begin(), Default<TDH_D_1000>().Data.end()};

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

// long double

namespace {
    using TDH_LD_10 = TDataHolder<long double, 10>;
    using TDH_LD_100 = TDataHolder<long double, 100>;
    using TDH_LD_1000 = TDataHolder<long double, 1000>;
}

Y_CPU_BENCHMARK(RouletteWheel_LongDouble_10, p) {
    NSampling::TRouletteWheel<double> s;
    s.PushMany(Default<TDH_LD_10>().Data.begin(), Default<TDH_LD_10>().Data.end());
    s.template Prepare<TKahanAccumulator<double>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(RouletteWheel_LongDouble_100, p) {
    NSampling::TRouletteWheel<double> s;
    s.PushMany(Default<TDH_LD_100>().Data.begin(), Default<TDH_LD_100>().Data.end());
    s.template Prepare<TKahanAccumulator<double>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(RouletteWheel_LongDouble_1000, p) {
    NSampling::TRouletteWheel<double> s;
    s.PushMany(Default<TDH_LD_1000>().Data.begin(), Default<TDH_LD_1000>().Data.end());
    s.template Prepare<TKahanAccumulator<double>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(AliasMethod_LongDouble_10, p) {
    NSampling::TAliasMethod<double> s;
    s.PushMany(Default<TDH_LD_10>().Data.begin(), Default<TDH_LD_10>().Data.end());
    s.template Prepare<TKahanAccumulator<double>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(AliasMethod_LongDouble_100, p) {
    NSampling::TAliasMethod<double> s;
    s.PushMany(Default<TDH_LD_100>().Data.begin(), Default<TDH_LD_100>().Data.end());
    s.template Prepare<TKahanAccumulator<double>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(AliasMethod_LongDouble_1000, p) {
    NSampling::TAliasMethod<double> s;
    s.PushMany(Default<TDH_LD_1000>().Data.begin(), Default<TDH_LD_1000>().Data.end());
    s.template Prepare<TKahanAccumulator<double>>();

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(SamplingTree_LongDouble_10, p) {
    const auto& X = Default<TDH_LD_10>().Data;
    NSampling::TSamplingTree<double> s{X.size()};
    for (const auto i : xrange(X.size())) {
        s.SetFreq(i, X[i]);
    }

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(SamplingTree_LongDouble_100, p) {
    const auto& X = Default<TDH_LD_100>().Data;
    NSampling::TSamplingTree<double> s{X.size()};
    for (const auto i : xrange(X.size())) {
        s.SetFreq(i, X[i]);
    }

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(SamplingTree_LongDouble_1000, p) {
    const auto& X = Default<TDH_LD_1000>().Data;
    NSampling::TSamplingTree<double> s{X.size()};
    for (const auto i : xrange(X.size())) {
        s.SetFreq(i, X[i]);
    }

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(STLDiscreteDistribution_LongDouble_10, p) {
    std::discrete_distribution<> s{Default<TDH_LD_10>().Data.begin(), Default<TDH_LD_10>().Data.end()};

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(STLDiscreteDistribution_LongDouble_100, p) {
    std::discrete_distribution<> s{Default<TDH_LD_100>().Data.begin(), Default<TDH_LD_100>().Data.end()};

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}

Y_CPU_BENCHMARK(STLDiscreteDistribution_LongDouble_1000, p) {
    std::discrete_distribution<> s{Default<TDH_LD_1000>().Data.begin(), Default<TDH_LD_1000>().Data.end()};

    TFastRng<ui64> prng{42};
    for (const auto i : xrange(p.Iterations())) {
        (void)i;
        Y_DO_NOT_OPTIMIZE_AWAY(s(prng));
    }
}
