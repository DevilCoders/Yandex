#include <kernel/doom/offroad_keyinv_wad/offroad_keyinv_wad_io_ut.h>
#include <library/cpp/testing/benchmark/bench.h>
#include <util/generic/singleton.h>

Y_CPU_BENCHMARK(BuildIndex, iface) {
    for (auto i : xrange(iface.Iterations())) {
        Y_UNUSED(i);
        TIndexGenerator index;
        Y_DO_NOT_OPTIMIZE_AWAY(index);
    }
}

Y_CPU_BENCHMARK(ReadAllIndex, iface) {
    for (size_t i : xrange<size_t>(iface.Iterations())) {
        Y_UNUSED(i);
        TIndexGenerator::TReader reader = Singleton<TIndexGenerator>()->GetReader();
        TStringBuf key;
        while (reader.ReadKey(&key)) {
            while (reader.NextLayer()) {
                TPantherHit hit;
                while (reader.ReadHit(&hit)) {
                    Y_DO_NOT_OPTIMIZE_AWAY(hit);
                }
            }
        }
    }
}
