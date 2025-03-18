#include <antirobot/lib/ip_map.h>

#include <library/cpp/testing/benchmark/bench.h>

#include <util/generic/xrange.h>
#include <util/random/random.h>

using namespace NAntiRobot;

namespace {
    TAddr GetIpAddr(float p) {
        if (RandomNumber<float>() < p) {
            return TAddr("85.140.1.1");
        }
        return TAddr::FromIp(RandomNumber<unsigned int>());
    }
}

#define REGISTER_BENCHMARK(iterations, hitPercent)                                                \
    Y_CPU_BENCHMARK(TIpRangeMap_##iterations##_##hitPercent, iface) {                             \
        TIpRangeMap<size_t> ipRangeMap;                                                           \
        ipRangeMap.Insert({TIpInterval::Parse("176.59.0.0/16"), 17});                                   \
        ipRangeMap.Insert({TIpInterval::Parse("213.87.0.0/16"), 18});                                   \
        ipRangeMap.Insert({TIpInterval::Parse("217.118.0.0/16"), 17});                                  \
        ipRangeMap.Insert({TIpInterval::Parse("31.173.0.0/16"), 18});                                   \
        ipRangeMap.Insert({TIpInterval::Parse("85.140.0.0/16"), 17});                                   \
        ipRangeMap.Insert({TIpInterval::Parse("188.170.0.0/16"), 18});                                  \
        ipRangeMap.Insert({TIpInterval::Parse("95.108.0.0/16"), 17});                                   \
        ipRangeMap.Insert({TIpInterval::Parse("87.250.0.0/16"), 18});                                   \
        ipRangeMap.Insert({TIpInterval::Parse("109.252.0.0/16"), 17});                                  \
        ipRangeMap.Insert({TIpInterval::Parse("95.153.0.0/16"), 18});                                   \
                                                                                                  \
        for (const auto i : xrange(iface.Iterations())) {                                         \
            Y_UNUSED(i);                                                                          \
            for (const auto _ : xrange(iterations)) {                                             \
                Y_UNUSED(_);                                                                      \
                Y_DO_NOT_OPTIMIZE_AWAY(ipRangeMap.Find(GetIpAddr(hitPercent / 100.0)).Defined()); \
            }                                                                                     \
        }                                                                                         \
    }

REGISTER_BENCHMARK(10, 1)
REGISTER_BENCHMARK(10, 5)
REGISTER_BENCHMARK(10, 50)
REGISTER_BENCHMARK(10, 75)

REGISTER_BENCHMARK(100, 1)
REGISTER_BENCHMARK(100, 5)
REGISTER_BENCHMARK(100, 50)
REGISTER_BENCHMARK(100, 75)

REGISTER_BENCHMARK(1000, 1)
REGISTER_BENCHMARK(1000, 5)
REGISTER_BENCHMARK(1000, 50)
REGISTER_BENCHMARK(1000, 75)

REGISTER_BENCHMARK(10000, 1)
REGISTER_BENCHMARK(10000, 5)
REGISTER_BENCHMARK(10000, 50)
REGISTER_BENCHMARK(10000, 75)
