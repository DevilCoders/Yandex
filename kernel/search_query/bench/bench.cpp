#include <kernel/search_query/cnorm.h>

#include <library/cpp/resource/resource.h>
#include <contrib/libs/benchmark/include/benchmark/benchmark.h>

#include <util/generic/vector.h>
#include <util/generic/xrange.h>
#include <util/stream/str.h>


namespace {
    struct TDataHolder
        : TVector<TString> {
    public:
        TDataHolder() {
            TString inputs = NResource::Find("/texts");
            TStringInput stream(inputs);
            do {
                emplace_back();
            } while (stream.ReadLine(back()));
        }
    } texts;
}

void CNorm(benchmark::State& state) {
    NCnorm::TConsistentNormalizer normalizer;

    size_t i = 0;
    for (auto _ : state) {
        normalizer.Normalize(texts[i % texts.size()]);
        ++i;
    }
}

void BNormOld(benchmark::State& state) {
    NCnorm::TBertNormalizer normalizer;

    size_t i = 0;
    for (auto _ : state) {
        normalizer.Normalize(texts[i % texts.size()]);
        ++i;
    }
}

void BNorm(benchmark::State& state) {
    NCnorm::TBertNormalizerOptimized normalizer;

    size_t i = 0;
    for (auto _ : state) {
        normalizer.Normalize(texts[i % texts.size()]);
        ++i;
    }
}

void ANorm(benchmark::State& state) {
    NCnorm::TAttributeNormalizer normalizer;

    size_t i = 0;
    for (auto _ : state) {
        normalizer.Normalize(texts[i % texts.size()]);
        ++i;
    }
}

BENCHMARK(CNorm)->Unit(benchmark::kMicrosecond);
BENCHMARK(BNorm)->Unit(benchmark::kMicrosecond);
BENCHMARK(BNormOld)->Unit(benchmark::kMicrosecond);
BENCHMARK(ANorm)->Unit(benchmark::kMicrosecond);
BENCHMARK_MAIN();