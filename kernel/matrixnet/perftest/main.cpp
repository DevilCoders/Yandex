#include <benchmark/benchmark.h>
#include <library/cpp/archive/yarchive.h>

#include <kernel/matrixnet/mn_dynamic.h>
#include <util/generic/buffer.h>
#include <util/memory/blob.h>
#include <util/stream/mem.h>
#include <util/generic/xrange.h>
#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/random/fast.h>

namespace {
extern "C" {
    extern const unsigned char TestModels[];
    extern const ui32 TestModelsSize;
};
static const TString RuFast("/ru_fast.info");
static const TString RuHast("/ru_hast.info");

TBlob GetBlob(const TString& modelName) {
    TArchiveReader archive(TBlob::NoCopy(TestModels, TestModelsSize));
    return archive.ObjectBlobByKey(modelName);
}
}
struct TModelAndDataHolder {
    TModelAndDataHolder(const TString& modelName, size_t docCount) {
        TBlob blob = GetBlob(modelName);
        TMemoryInput in(blob.Data(), blob.Length());
        Model.Load(&in);

        TFastRng64 rng(1234);
        const auto fCount = Model.GetNumFeats();

        Features.resize(docCount);
        for (auto i : xrange<size_t>(0, docCount)) {
            Features[i].resize(fCount);
            for (auto j : xrange<size_t>(0, fCount)) {
                Features[i][j] = rng.GenRandReal1();
            }
        }
        ResultRelev.resize(docCount);
    }
    NMatrixnet::TMnSseDynamic Model;
    TVector<TVector<float>> Features;
    TVector<double> ResultRelev;
};

void DoAll(const TString& mname, int docCount, benchmark::State& state) {
    TModelAndDataHolder modelAndData(mname, docCount);
    for (auto _ : state) {
        modelAndData.Model.CalcRelevs(modelAndData.Features, modelAndData.ResultRelev);
        benchmark::DoNotOptimize(modelAndData.ResultRelev);
    }
}

void DoBinarization(const TString& mname, int docCount, benchmark::State& state) {
    TModelAndDataHolder modelAndData(mname, docCount);
    for (auto _ : state) {
        benchmark::DoNotOptimize(modelAndData.Model.CalcBinarization(modelAndData.Features));
    }
}

void DoApply(const TString& mname, int docCount, benchmark::State& state) {
    TModelAndDataHolder modelAndData(mname, docCount);
    auto binarizaztion = modelAndData.Model.CalcBinarization(modelAndData.Features);
    for (auto _ : state) {
        modelAndData.Model.CalcRelevs(*binarizaztion, modelAndData.ResultRelev);
        benchmark::DoNotOptimize(modelAndData.ResultRelev);
    }
}

void TestFastModelAll(benchmark::State& state) {
    DoAll(RuFast, state.range(0), state);
}

void TestHastModelAll(benchmark::State& state) {
    DoAll(RuHast, state.range(0), state);
}

void TestFastModelBinarization(benchmark::State& state) {
    DoBinarization(RuFast, state.range(0), state);
}

void TestHastModelBinarization(benchmark::State& state) {
    DoBinarization(RuHast, state.range(0), state);
}

void TestFastModelApply(benchmark::State& state) {
    DoApply(RuFast, state.range(0), state);
}

void TestHastModelApply(benchmark::State& state) {
    DoApply(RuHast, state.range(0), state);
}

BENCHMARK(TestFastModelAll)->Arg(1)->Arg(128)->Arg(1024);
BENCHMARK(TestHastModelAll)->Arg(1)->Arg(128)->Arg(1024);
BENCHMARK(TestFastModelBinarization)->Arg(1)->Arg(128)->Arg(1024);
BENCHMARK(TestHastModelBinarization)->Arg(1)->Arg(128)->Arg(1024);
BENCHMARK(TestFastModelApply)->Arg(1)->Arg(128)->Arg(1024);
BENCHMARK(TestHastModelApply)->Arg(1)->Arg(128)->Arg(1024);
