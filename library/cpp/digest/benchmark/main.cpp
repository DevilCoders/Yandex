#include "SpookyV2.h"
#include "farmhash.h"

#include <library/cpp/digest/sfh/sfh.h>
#include <library/cpp/digest/crc32c/crc32c.h>
#include <library/cpp/digest/old_crc/crc.h>
#include <library/cpp/digest/murmur/murmur.h>
#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/testing/benchmark/bench.h>

#if defined(Y_BOBHASH)
#include <yabs/server/util/bobhash.h>
#endif

#include <util/generic/ptr.h>
#include <util/digest/city.h>
#include <util/digest/murmur.h>
#include <util/stream/output.h>

#include <contrib/libs/taocrypt/include/md5.hpp>

template <size_t N, size_t O, class F>
void RunTest(F&& f, size_t n) {
    TArrayHolder<char> a(new char[N + 100]);

    for (size_t i = 0; i < n; ++i) {
        Y_DO_NOT_OPTIMIZE_AWAY(f(a.Get() + O, N));
    }
}

Y_CPU_BENCHMARK(SuperFastHash_1000, iface) {
    RunTest<1000, 1>(SuperFastHash, iface.Iterations());
}

Y_CPU_BENCHMARK(CityHash64_1000, iface) {
    RunTest<1000, 1>([](const char* buf, size_t len) {
        return CityHash64(buf, len);
    }, iface.Iterations());
}

#if 0
Y_CPU_BENCHMARK(XXH64_1000, iface) {
    RunTest<1000, 1>([](const char* buf, size_t len) {
            return XXH64(buf, len, 0);
    }, iface.Iterations());
}
#endif

Y_CPU_BENCHMARK(MurmurHash64_1000, iface) {
    RunTest<1000, 1>([](const char* buf, size_t len) {
        return MurmurHash<ui64>(buf, len);
    }, iface.Iterations());
}

Y_CPU_BENCHMARK(OldCrc64_1000, iface) {
    RunTest<1000, 1>([](const char* buf, size_t len) {
        return Crc<ui64>(buf, len);
    }, iface.Iterations());
}

Y_CPU_BENCHMARK(Crc32c_1000, iface) {
    RunTest<1000, 1>([](const char* buf, size_t len) {
        return Crc32c(buf, len);
    }, iface.Iterations());
}

Y_CPU_BENCHMARK(SpookyHash64_1000, iface) {
    RunTest<1000, 1>([](const char* buf, size_t len) {
        return SpookyHash::Hash64(buf, len, 0);
    }, iface.Iterations());
}

Y_CPU_BENCHMARK(FarmHash32_1000, iface) {
    RunTest<1000, 1>([](const char* buf, size_t len) {
        return util::Hash32(buf, len);
    }, iface.Iterations());
}

Y_CPU_BENCHMARK(FarmHash64_1000, iface) {
    RunTest<1000, 1>([](const char* buf, size_t len) {
        return util::Hash64(buf, len);
    }, iface.Iterations());
}

Y_CPU_BENCHMARK(TaoCryptMD5_1000, iface) {
    RunTest<1000, 1>([](const char* buf, size_t len) {
        TaoCrypt::MD5().Update((const unsigned char*)buf, len);

        return 0;
    }, iface.Iterations());
}

Y_CPU_BENCHMARK(MD5_1000, iface) {
    RunTest<1000, 1>([](const char* buf, size_t len) {
        MD5().Update((const unsigned char*)buf, len);

        return 0;
    }, iface.Iterations());
}

#if defined(Y_BOBHASH)
Y_CPU_BENCHMARK(YabsBobHash_1000, iface) {
    RunTest<1000, 1>([](const char* buf, size_t len) {
        return yabs_bobhash(buf, len);
    }, iface.Iterations());
}
#endif
