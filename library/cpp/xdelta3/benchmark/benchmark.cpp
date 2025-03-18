#include <benchmark/benchmark.h>

#include <library/cpp/xdelta3/xdelta_codec/codec.h>
#include <library/cpp/xdelta3/state/data_ptr.h>
#include <library/cpp/xdelta3/ut/rand_data/generator.h>

#include <util/generic/buffer.h>
#include <util/generic/string.h>

#include <library/cpp/testing/common/env.h>

using namespace NXdeltaAggregateColumn;

constexpr auto N = 1000;
constexpr auto MaxSize = 300000;

struct TDataBuffer {
    TDataPtr Data;
    size_t Size;
};

TVector<TDataBuffer> GenerateProfiles() 
{
    TVector<TDataBuffer> result(Reserve(N));
    auto baseSize = rand() % MaxSize + 1;
    auto base = RandData(baseSize);

    for (size_t i = 0; i < N; ++i) {
        size_t size = rand() % MaxSize + 1;
        result.push_back({RandData(size), size});
    }
    return result;
}

TVector<TDataBuffer> GeneratePatches(const TVector<TDataBuffer>& profiles) 
{
    TVector<TDataBuffer> result(Reserve(profiles.size()));
    size_t patchSize = 0;
    for (size_t i = 0; i < size_t(profiles.size()) - 1; ++i) {
        auto patch = TDataPtr(ComputePatch(
            nullptr,
            profiles[i].Data.get(),
            profiles[i].Size,
            profiles[i + 1].Data.get(),
            profiles[i + 1].Size,
            &patchSize));
        result.push_back({std::move(patch), patchSize});
    }
    return result;
}

static void BM_Decode(
    benchmark::State& state,
    const TVector<TDataBuffer>& profiles,
    const TVector<TDataBuffer>& patches) 
{
    for (auto _ : state) {
        size_t patchSize = 0;
        for (size_t i = 0; i < profiles.size() - 1; ++i) {
            Y_UNUSED(TDataPtr(ApplyPatch(
                nullptr,
                0,
                profiles[i].Data.get(),
                profiles[i].Size,
                patches[i].Data.get(),
                patches[i].Size,
                profiles[i + 1].Size,
                &patchSize)));
        }
    }
}

static void BM_Encode(
    benchmark::State& state,
    const TVector<TDataBuffer>& profiles) 
{
    for (auto _ : state) {
        size_t patchSize = 0;
        for (size_t i = 0; i < size_t(profiles.size()) - 1; ++i) {
            Y_UNUSED(TDataPtr(ComputePatch(
                nullptr, profiles[i].Data.get(),
                profiles[i].Size,
                profiles[i + 1].Data.get(),
                profiles[i + 1].Size,
                &patchSize)));
        }
    }
}

static void BM_Merge(benchmark::State& state, const TVector<TDataBuffer>& patches) 
{
    for (auto _ : state) {
        size_t patchSize = 0;
        for (size_t i = 0; i < size_t(patches.size()) - 1; ++i) {
            Y_UNUSED(TDataPtr(MergePatches(
                nullptr,
                0,
                patches[i].Data.get(),
                patches[i].Size,
                patches[i + 1].Data.get(),
                patches[i + 1].Size,
                &patchSize)));
        }
    }
}

namespace
{
    TVector<TDataBuffer> Profiles = GenerateProfiles();
    TVector<TDataBuffer> Patches = GeneratePatches(Profiles);
}

BENCHMARK_CAPTURE(BM_Encode, encode, Profiles);
BENCHMARK_CAPTURE(BM_Decode, decode, Profiles, Patches);
BENCHMARK_CAPTURE(BM_Merge, merge, Patches);
