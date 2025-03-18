#include <antirobot/lib/ip_map.h>
#include <antirobot/lib/ip_vec.h>
#include <antirobot/lib/ip_hash.h>

#include <library/cpp/testing/benchmark/bench.h>

#include <util/generic/xrange.h>
#include <util/random/random.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NAntiRobot {

namespace {
    TAddr GetIpAddr(float p, const TAddr& addr) {
        if (RandomNumber<float>() < p) {
            return addr;
        }
        return TAddr::FromIp(RandomNumber<unsigned int>());
    }

    template <typename T>
    std::pair<T, TAddr> GenerateImpl(const size_t num) {
        T result;
        TAddr addrToFind;
        while (result.Size() < num) {
            const auto ip = TAddr::FromIp(RandomNumber<ui32>());
            if (!addrToFind.Valid()) {
                addrToFind = ip;
            }

            if (!result.Find(ip).Defined()) {
                const TIpInterval key = TIpInterval::Parse(ip.GetSubnet(24).ToString() + "/24");
                const auto value = RandomNumber<float>();
                result.Insert({key, value});
            }
        }
        result.Finish();
        return {result, addrToFind};
    }

    template <typename T, size_t num>
    std::pair<T, TAddr> Generate() {
        static const auto res = GenerateImpl<T>(num);
        return res;
    }

    template <typename T, size_t num>
    void Benchmark(size_t findIterations, size_t hitPercent, size_t iterations) {
        const auto [ipMap, ip] = Generate<T, num>();

        for (const auto i : xrange(iterations)) {
            Y_UNUSED(i);
            for (const auto _ : xrange(findIterations)) {
                Y_UNUSED(_);
                Y_DO_NOT_OPTIMIZE_AWAY(ipMap.Find(GetIpAddr(hitPercent / 100.0, ip)).Defined());
            }
        }
    }
} // anonymous namespace

#define REGISTER_BENCHMARK(num, findIterations, hitPercent)                         \
    Y_CPU_BENCHMARK(TIpRangeMap_##num##_##findIterations##_##hitPercent, iface) { \
        Benchmark<TIpRangeMap<float>, num>(findIterations, hitPercent, iface.Iterations());   \
    }                                                                               \
                                                                                    \
    Y_CPU_BENCHMARK(TIpHashMap_##num##_##findIterations##_##hitPercent, iface) { \
        Benchmark<TIpHashMap<float, 24, 64>, num>(findIterations, hitPercent, iface.Iterations());    \
    }                                                                               \
                                                                                    \
    Y_CPU_BENCHMARK(TIpVector_##num##_##findIterations##_##hitPercent, iface) { \
        Benchmark<TIpVector<float, 24, 64>, num>(findIterations, hitPercent, iface.Iterations());   \
    }                                                                               

REGISTER_BENCHMARK(1000, 10, 0)
REGISTER_BENCHMARK(1000, 10, 1)
REGISTER_BENCHMARK(1000, 10, 5)
REGISTER_BENCHMARK(1000, 10, 10)
REGISTER_BENCHMARK(1000, 10, 50)
REGISTER_BENCHMARK(1000, 10, 75)

REGISTER_BENCHMARK(100000, 1000, 0)
REGISTER_BENCHMARK(100000, 1000, 1)
REGISTER_BENCHMARK(100000, 1000, 5)
REGISTER_BENCHMARK(100000, 1000, 10)
REGISTER_BENCHMARK(100000, 1000, 50)
REGISTER_BENCHMARK(100000, 1000, 75)

REGISTER_BENCHMARK(10000000, 100, 0)
REGISTER_BENCHMARK(10000000, 100, 1)
REGISTER_BENCHMARK(10000000, 100, 5)
REGISTER_BENCHMARK(10000000, 100, 10)
REGISTER_BENCHMARK(10000000, 100, 50)
REGISTER_BENCHMARK(10000000, 100, 75)

REGISTER_BENCHMARK(10000000, 1000, 0)
REGISTER_BENCHMARK(10000000, 1000, 1)
REGISTER_BENCHMARK(10000000, 1000, 5)
REGISTER_BENCHMARK(10000000, 1000, 10)
REGISTER_BENCHMARK(10000000, 1000, 50)
REGISTER_BENCHMARK(10000000, 1000, 75)

} // namespace NAntiRobot
