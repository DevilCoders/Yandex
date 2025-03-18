#include <atomic>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>

#include <thread>

#include <util/generic/yexception.h>
#include <util/generic/size_literals.h>
#include <util/system/env.h>
#include <util/system/yassert.h>

#include <benchmark/benchmark.h>

constexpr size_t MaxHeapSize = 128_GB;

template <class TFn>
void RunInFork(const TFn& fn)
{
    if (GetEnv("BENCHMARK_SINGLE") != "") {
        fn();
        _exit(0);
    }

    auto pid = fork();
    if (pid == 0) {
        fn();
        _exit(0);
    } else if (pid > 0) {
        int status;
        if (wait(&status) == -1) {
            throw TSystemError() << "wait failed";
        }

        if (WEXITSTATUS(status) != 0) {
            throw yexception() << "child terminated";
        }
    } else {
        throw TSystemError() << "fork failed";
    }
}

static void DoBenchmark(benchmark::State& state)
{
    auto allocSize = state.range(0);
    auto allocCount = state.range(1);
    auto threadCount = state.range(2);

    auto workingSetSize = static_cast<size_t>(allocSize * allocCount * threadCount);
    if (workingSetSize > MaxHeapSize) {
        state.SkipWithError("test is too big");
    }

    int iterationCount = 0;
    for (auto _ : state) {
        iterationCount++;

        RunInFork([&] {
            std::vector<std::thread> threads;

            for (int i = 0; i < threadCount; i++) {
                threads.emplace_back([&] {
                    for (int j = 0; j < allocCount; j++) {
                        auto ptr = reinterpret_cast<char*>(malloc(allocSize));
                        *ptr = 'x';
                        benchmark::DoNotOptimize(*ptr);
                    }
                });
            }

            for (auto& t : threads) {
                t.join();
            }
        });
    }

    state.SetBytesProcessed(workingSetSize * iterationCount);
    state.SetItemsProcessed(allocCount * threadCount * iterationCount);
}

static void BenchmarkSmallStartupAllocs(benchmark::State& state)
{
    DoBenchmark(state);
}

BENCHMARK(BenchmarkSmallStartupAllocs)
    ->ArgNames({"alloc_size", "alloc_count", "thread_count"})
    ->UseRealTime()
    ->Args({32, 1 << 20, 16})
    ->Args({64, 1 << 20, 16})
    ->Args({128, 1 << 20, 16})
    ->Args({1024, 1 << 20, 16})
    ->Args({4_KB, 1 << 20, 16})
    ->Args({16_KB, 1 << 16, 16})
    ->Args({32_KB, 1 << 16, 16})
    ->Unit(benchmark::kMillisecond);


static void BenchmarkLargeStartupAllocs(benchmark::State& state)
{
    DoBenchmark(state);
}

BENCHMARK(BenchmarkLargeStartupAllocs)
    ->ArgNames({"alloc_size", "alloc_count", "thread_count"})
    ->UseRealTime()
    ->Args({64_KB, 1 << 16, 16})
    ->Args({128_KB, 1 << 16, 16})
    ->Args({512_KB, 1 << 14, 16})
    ->Args({1_MB, 1 << 12, 16})
    ->Unit(benchmark::kMillisecond);


enum class EPrefaultKind {
    Read,
    Write,
    None,
    MadvPopulate,
    MMapPopulate,
};

#ifndef MADV_POPULATE
    #define MADV_POPULATE 0x59410003
#endif

static void BenchmarkPrefault(benchmark::State& state, EPrefaultKind kind)
{
    constexpr size_t PrefaultBufferSize = 512_MB;
    constexpr size_t PageSize = 4_KB;

    char* buffer = nullptr;
    auto allocate = [&] {
        if (buffer) {
            Y_VERIFY(::munmap(buffer, PrefaultBufferSize) == 0);
        }

        buffer = static_cast<char*>(::mmap(
            NULL,
            PrefaultBufferSize,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | (kind == EPrefaultKind::MMapPopulate ? MAP_POPULATE : 0),
            -1,
            0));
        Y_VERIFY(buffer != MAP_FAILED);

        if (kind == EPrefaultKind::MadvPopulate) {
            int res = ::madvise(buffer, PrefaultBufferSize, MADV_POPULATE);
            if (res != 0 && errno == EINVAL) {
                return;
            }
            Y_VERIFY(res == 0);
            return;
        } else if (kind == EPrefaultKind::None) {
            return;
        } else if (kind == EPrefaultKind::MMapPopulate) {
            return;
        }

        int sum = 0;
        for (auto* current = buffer; current < buffer + PrefaultBufferSize; current += PageSize) {
            switch (kind) {
                case EPrefaultKind::Read:
                    sum += static_cast<int>(*current);
                    break;
                case EPrefaultKind::Write:
                    *current = 1;
                    break;
                default:
                    Y_VERIFY(false);
            }
        }
        benchmark::DoNotOptimize(sum);
    };

    allocate();

    int i = 0;
    for (auto _ : state) {
        if (i == PrefaultBufferSize) {
            state.PauseTiming();
            allocate();
            i = 0;
            state.ResumeTiming();
        }

        buffer[i] = 0;
        i += PageSize;
    }
}

BENCHMARK_CAPTURE(BenchmarkPrefault, Read, EPrefaultKind::Read)
    ->Iterations(100 * 1000);

BENCHMARK_CAPTURE(BenchmarkPrefault, Write, EPrefaultKind::Write)
    ->Iterations(100 * 1000);

BENCHMARK_CAPTURE(BenchmarkPrefault, None, EPrefaultKind::None)
    ->Iterations(100 * 1000);

BENCHMARK_CAPTURE(BenchmarkPrefault, MadvPopulate, EPrefaultKind::MadvPopulate)
    ->Iterations(100 * 1000);

BENCHMARK_CAPTURE(BenchmarkPrefault, MMapPopulate, EPrefaultKind::MMapPopulate)
    ->Iterations(100 * 1000);
