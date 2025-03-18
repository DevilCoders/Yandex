#include <library/cpp/testing/benchmark/bench.h>

#include <library/cpp/pop_count/popcount.h>
#include <library/cpp/select_in_word/select_in_word.h>

#include <util/generic/vector.h>
#include <util/generic/xrange.h>
#include <util/random/fast.h>
#include <util/system/cpu_id.h>

#include <immintrin.h>

struct TWork {
    int BitNumber;
    ui64 Word;
};

constexpr size_t WORK_SIZE = 1ull << 20;
const auto WORK = [] {
    TVector<TWork> work(WORK_SIZE);

    TFastRng32 rng(11, 0);

    for (const size_t i: xrange(WORK_SIZE)) {
        do {
            work[i].Word = rng.GenRand64();
        } while (work[i].Word == 0);
        work[i].BitNumber = rng.Uniform(PopCount(work[i].Word));
    }

    return work;
}();


Y_CPU_BENCHMARK(SelectInWordX86, iface) {
    for (const size_t i: xrange(iface.Iterations())) {
        const auto& work = WORK[i & (WORK_SIZE - 1)];
        NBench::DoNotOptimize(SelectInWordX86(work.Word, work.BitNumber));
    }
}

Y_CPU_BENCHMARK(SelectInWordBmi2, iface) {
    for (const size_t i: xrange(iface.Iterations())) {
        const auto& work = WORK[i & (WORK_SIZE - 1)];
        NBench::DoNotOptimize(SelectInWordBmi2(work.Word, work.BitNumber));
    }
}

Y_CPU_BENCHMARK(SelectInWordDispatch, iface) {
    for (const size_t i: xrange(iface.Iterations())) {
        const auto& work = WORK[i & (WORK_SIZE - 1)];
        NBench::DoNotOptimize(SelectInWord(work.Word, work.BitNumber));
    }
}
