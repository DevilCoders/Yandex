#include <benchmark/benchmark.h>

#include <util/generic/size_literals.h>

constexpr size_t TestAllocSize = 16_KB;

void BenchmarkThreadCache(benchmark::State& state)
{
    for (auto _ : state) {
        auto ptr = malloc(TestAllocSize);
        benchmark::DoNotOptimize(ptr);
        free(ptr);
    }
}
