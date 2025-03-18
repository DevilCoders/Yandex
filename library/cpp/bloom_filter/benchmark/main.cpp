#include <library/cpp/bloom_filter/bloomfilter.h>
#include <library/cpp/testing/benchmark/bench.h>

constexpr size_t ELEM_COUNT = 1000;
constexpr size_t HASH_COUNT = 3;
constexpr float ERROR = 0.001;

template <typename TFilter>
struct TFixture {
    TFilter Filter;

    template <typename... Args>
    explicit TFixture(Args&&... args)
        : Filter(std::forward<Args>(args)...)
    {
        for (size_t i = 0; i < ELEM_COUNT; ++i) {
            size_t j = i * 1000;
            Filter.Add(&j, sizeof(j));
        }
    }
};

Y_CPU_BENCHMARK(BloomDefault, iface) {
    static TFixture<TBloomFilter> fixture(ELEM_COUNT, ERROR);
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        for (size_t i = 0; i < ELEM_COUNT; ++i) {
            size_t j = i * 1000;
            fixture.Filter.Has(&j, sizeof(j));
        }
    }
}

Y_CPU_BENCHMARK(BloomFaster, iface) {
    static TFixture<TBloomFilterFaster> fixture(ELEM_COUNT, ERROR);
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        for (size_t i = 0; i < ELEM_COUNT; ++i) {
            size_t j = i * 1000;
            fixture.Filter.Has(&j, sizeof(j));
        }
    }
}

Y_CPU_BENCHMARK(BloomFixedHashCount, iface) {
    static TFixture<TBloomFilter> fixture(ELEM_COUNT, HASH_COUNT, ERROR);
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        for (size_t i = 0; i < ELEM_COUNT; ++i) {
            size_t j = i * 1000;
            fixture.Filter.Has(&j, sizeof(j));
        }
    }
}
