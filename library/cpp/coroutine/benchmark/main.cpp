#include <library/cpp/testing/benchmark/bench.h>
#include <library/cpp/coroutine/engine/impl.h>

#include <util/thread/singleton.h>

namespace {
    struct TMyExecutor: public TContExecutor {
        inline TMyExecutor()
            : TContExecutor(20000)
        {
        }
    };
}

template <size_t NN, class II>
static void XRun(II&& iface) {
    auto& e = *FastTlsSingleton<TMyExecutor>();

    i64 n = iface.Iterations();

    auto f = [&](TCont* c) {
        while (n >= 0) {
            c->Yield();
            --n;
        }
    };

    for (size_t x = 1; x < NN; ++x) {
        e.Create(f, "slave");
    }

    e.Execute(f);
}

Y_CPU_BENCHMARK(ContextSwitch_1, iface) {
    XRun<1>(iface);
}

Y_CPU_BENCHMARK(ContextSwitch_10, iface) {
    XRun<10>(iface);
}

Y_CPU_BENCHMARK(ContextSwitch_100, iface) {
    XRun<100>(iface);
}

Y_CPU_BENCHMARK(ContextSwitch_1000, iface) {
    XRun<1000>(iface);
}
