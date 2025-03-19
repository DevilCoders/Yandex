#include <kernel/doom/offroad_hashed_keyinv_wad/offroad_hashed_keyinv_wad_ut.h>
#include <library/cpp/testing/benchmark/bench.h>
#include <util/generic/singleton.h>

static constexpr ui32 Ui32NumBits = 32;
static constexpr ui32 Ui64NumBits = 56;

Y_CPU_BENCHMARK(BuildIndex32, iface) {
    for (auto i : xrange(iface.Iterations())) {
        Y_UNUSED(i);
        TIndexGenerator<TUi32Io> index(500, 500, Ui32NumBits, false);
        Y_DO_NOT_OPTIMIZE_AWAY(index);
    }
}

Y_CPU_BENCHMARK(BuildIndex64, iface) {
    for (auto i : xrange(iface.Iterations())) {
        Y_UNUSED(i);
        TIndexGenerator<TUi64Io> index(500, 500, Ui64NumBits, false);
        Y_DO_NOT_OPTIMIZE_AWAY(index);
    }
}

Y_CPU_BENCHMARK(SearchIndex32, iface) {
    for (size_t i : xrange<size_t>(iface.Iterations())) {
        TIndexGenerator<TUi32Io>::TSearcher searcher = Singleton<TIndexGenerator<TUi32Io>>(500, 500, Ui32NumBits, false)->GetSearcher();
        TFastRng<ui32> rng(i);
        for (size_t j = 0; j < i; ++j) {
            typename TIndexGenerator<TUi32Io>::TIterator iterator;
            searcher.Find(rng(), &iterator);
            TPantherHit hit;
            while (iterator.ReadHit(&hit)) {
                Y_DO_NOT_OPTIMIZE_AWAY(hit);
            }
        }
    }
}

Y_CPU_BENCHMARK(SearchIndex64, iface) {
    for (size_t i : xrange<size_t>(iface.Iterations())) {
        TIndexGenerator<TUi64Io>::TSearcher searcher = Singleton<TIndexGenerator<TUi64Io>>(500, 500, Ui64NumBits, false)->GetSearcher();
        TFastRng<ui64> rng(i);
        for (size_t j = 0; j < i; ++j) {
            typename TIndexGenerator<TUi64Io>::TIterator iterator;
            searcher.Find(rng(), &iterator);
            TPantherHit hit;
            while (iterator.ReadHit(&hit)) {
                Y_DO_NOT_OPTIMIZE_AWAY(hit);
            }
        }
    }
}
