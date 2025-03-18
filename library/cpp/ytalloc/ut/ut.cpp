#include <library/cpp/ytalloc/api/ytalloc.h>

#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/testing/benchmark/bench.h>

#include <thread>
#include <atomic>
#include <unordered_set>

Y_UNIT_TEST_SUITE(YTAlloc) {

using namespace NYT;
using namespace NYT::NYTAlloc;

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST(ZeroSizeAllocation)
{
    Free(Allocate(0));
}

Y_UNIT_TEST(SmallAllocateFree)
{
    constexpr int N = 100000;
    constexpr size_t S = 256;
    std::unordered_set<void*> ptrs;
    for (int i = 0; i <  N; ++i) {
        UNIT_ASSERT(ptrs.insert(Allocate(S)).second);
    }
    for (auto* ptr : ptrs) {
        Free(ptr);
    }
}

Y_UNIT_TEST(GetAllocationSize1)
{
    UNIT_ASSERT_EQUAL(GetAllocationSize(nullptr), 0);

    auto run = [] (size_t size, size_t allocationSize) {
        void* ptr = Allocate(size);
        UNIT_ASSERT_EQUAL(GetAllocationSize(size), allocationSize);
        UNIT_ASSERT_EQUAL(GetAllocationSize(ptr), allocationSize);
        Free(ptr);
    };

    for (auto size : SmallRankToSize) {
        if (size <  2) {
            continue;
        }
        run(size, size);
        run(size - 1, size);
    }
}

Y_UNIT_TEST(GetAllocationSize2)
{
    UNIT_ASSERT_EQUAL(GetAllocationSize(static_cast<size_t>(0)), 16);
    UNIT_ASSERT_EQUAL(GetAllocationSize(1), 16);
    UNIT_ASSERT_EQUAL(GetAllocationSize(15), 16);
    UNIT_ASSERT_EQUAL(GetAllocationSize(16), 16);
    UNIT_ASSERT_EQUAL(GetAllocationSize(769), 1024);
    UNIT_ASSERT_EQUAL(GetAllocationSize(1023), 1024);
    UNIT_ASSERT_EQUAL(GetAllocationSize(1024), 1024);
    UNIT_ASSERT_EQUAL(GetAllocationSize(24'577), 32'768);
    UNIT_ASSERT_EQUAL(GetAllocationSize(32'700), 32'768);
    UNIT_ASSERT_EQUAL(GetAllocationSize(32'767), 32'768);
    UNIT_ASSERT_EQUAL(GetAllocationSize(32'768), 32'768);
    UNIT_ASSERT_EQUAL(GetAllocationSize(32'769), 36'784);
    UNIT_ASSERT_EQUAL(GetAllocationSize(1'000'000'000), 1'000'001'488);
}

Y_UNIT_TEST(ZeroUsageMemoryTag)
{
    SetCurrentMemoryTag(42);
    UNIT_ASSERT_EQUAL(GetMemoryUsageForTag(42), 0);
    SetCurrentMemoryTag(NullMemoryTag);
}

// Allocate vector that results in exactly `size` memory usage considering the 16-byte header.
std::unique_ptr<char[]> MakeAllocation(size_t size)
{
    Y_VERIFY(IsPowerOf2(size));

    auto result = std::unique_ptr<char[]>(new char[size]);

    // We make fake side effect to prevent any compiler optimizations here.
    // (Clever compilers love to throw away our unused allocations.)
    NBench::Escape(result.get());

    return result;
}

Y_UNIT_TEST(MemoryTags)
{
    for (size_t allocationSize = 1 << 5; allocationSize <= (1 << 20); allocationSize <<= 1) {
        {
            SetCurrentMemoryTag(42);
            auto allocation = MakeAllocation(allocationSize);
            SetCurrentMemoryTag(NullMemoryTag);
            UNIT_ASSERT_EQUAL(GetMemoryUsageForTag(42), allocationSize);
        }
        UNIT_ASSERT_EQUAL(GetMemoryUsageForTag(42), 0);
    }
}

Y_UNIT_TEST(AlignedAllocation)
{
    auto check = [] (size_t size) {
        void* ptr = AllocatePageAligned(size);
        UNIT_ASSERT_GE(GetAllocationSize(ptr), size);
        UNIT_ASSERT_EQUAL(reinterpret_cast<uintptr_t>(ptr) % PageSize, 0);
        Free(ptr);
    };
    check(0);
    check(1);
    check(17);
    check(4095);
    check(4096);
    check(65536);
    check(100000);
    check(LargeAllocationSizeThreshold);
    check(LargeAllocationSizeThreshold - 1);
    check(LargeAllocationSizeThreshold + 1);
}

template <class T, size_t N>
TEnumIndexedVector<T, ssize_t> AggregateArenaCounters(const std::array<TEnumIndexedVector<T, ssize_t>, N>& counters)
{
    TEnumIndexedVector<T, ssize_t> result;
    for (size_t index = 0; index < counters.size(); ++index) {
        for (auto counter : TEnumTraits<T>::GetDomainValues()) {
            result[counter] += counters[index][counter];
        }
    }
    return result;
}

template <class F>
void TestWithMemoryTags(const F& func)
{
    for (TMemoryTag tag : std::vector<TMemoryTag>{0, 1}) {
        SetCurrentMemoryTag(tag);
        func();
    }
    SetCurrentMemoryTag(NullMemoryTag);
}

void DontOptimizeAway(char* ptr, size_t size)
{
    static std::atomic<char> sum;
    ::memset(ptr, 0, size);
    for (size_t k = 0; k < size; ++k) {
        sum += ptr[k];
    }
}

Y_UNIT_TEST(LargeCounters)
{
    auto run = [] {
        constexpr auto N = 100_MBs;
        constexpr auto Eps = 1_MBs;
        auto total1 = GetTotalAllocationCounters()[ETotalCounter::BytesUsed];
        auto largeTotal1 = AggregateArenaCounters(GetLargeArenaAllocationCounters())[ELargeArenaCounter::BytesUsed];
        auto* ptr = Allocate(N);
        auto total2 = GetTotalAllocationCounters()[ETotalCounter::BytesUsed];
        auto largeTotal2 = AggregateArenaCounters(GetLargeArenaAllocationCounters())[ELargeArenaCounter::BytesUsed];
        UNIT_ASSERT_LE(std::abs(total2 - total1 - N), Eps);
        UNIT_ASSERT_LE(std::abs(largeTotal2 - largeTotal1 - N), Eps);
        Free(ptr);
        auto total3 = GetTotalAllocationCounters()[ETotalCounter::BytesUsed];
        auto largeTotal3 = AggregateArenaCounters(GetLargeArenaAllocationCounters())[ELargeArenaCounter::BytesUsed];
        UNIT_ASSERT_LE(std::abs(total3 - total1), Eps);
        UNIT_ASSERT_LE(std::abs(largeTotal3 - largeTotal1), Eps);
        SetCurrentMemoryTag(NullMemoryTag);
    };
    TestWithMemoryTags(run);
}

Y_UNIT_TEST(HugeCounters)
{
    auto run = [] {
        constexpr auto N = 10_GBs;
        constexpr auto Eps = 1_MBs;
        auto total1 = GetTotalAllocationCounters()[ETotalCounter::BytesUsed];
        auto hugeTotal1 = GetHugeAllocationCounters()[EHugeCounter::BytesUsed];
        auto* ptr = Allocate(N);
        auto total2 = GetTotalAllocationCounters()[ETotalCounter::BytesUsed];
        auto hugeTotal2 = GetHugeAllocationCounters()[EHugeCounter::BytesUsed];
        UNIT_ASSERT_LE(std::abs(total2 - total1 - N), Eps);
        UNIT_ASSERT_LE(std::abs(hugeTotal2 - hugeTotal1 - N), Eps);
        Free(ptr);
        auto total3 = GetTotalAllocationCounters()[ETotalCounter::BytesUsed];
        auto hugeTotal3 = GetHugeAllocationCounters()[EHugeCounter::BytesUsed];
        UNIT_ASSERT_LE(std::abs(total3 - total1), Eps);
        UNIT_ASSERT_LE(std::abs(hugeTotal3 - hugeTotal1), Eps);
    };
    TestWithMemoryTags(run);
}

Y_UNIT_TEST(AroundLargeBlobThreshold)
{
    constexpr size_t HugeAllocationSizeThreshold = 1ULL << (LargeRankCount - 1);
    for (int i = -10; i <= 10; ++i) {
        size_t size = HugeAllocationSizeThreshold + i * 10;
        void* ptr = Allocate(size);
        Free(ptr);
    }
}

Y_UNIT_TEST(PerThreadCacheReclaim)
{
    const int N = 1000;
    const int M = 200;
    const size_t S = 16_KB;

    auto getBytesCommitted = [] {
        static_assert(SmallRankCount >= 23, "SmallRankCount is too small");
        return GetSmallArenaAllocationCounters()[22][ESmallArenaCounter::BytesCommitted];
    };

    auto bytesBefore = getBytesCommitted();
    fprintf(stderr, "bytesBefore = %" PRISZT"\n", bytesBefore);

    for (int i = 0; i < N; i++) {
        std::thread t([&] {
            std::vector<char*> ptrs;
            for (int j = 0; j < M; ++j) {
                auto* ptr = new char[S];
                ptrs.push_back(ptr);
                DontOptimizeAway(ptr, S);
            }
            for (int j = 0; j < M; ++j) {
                delete[] ptrs[j];
            }
        });
        t.join();
    }


    auto bytesAfter = getBytesCommitted();
    fprintf(stderr, "bytesAfter = %" PRISZT"\n", bytesAfter);

    UNIT_ASSERT_GE(bytesAfter, bytesBefore);
    UNIT_ASSERT_LE(bytesAfter, bytesBefore + 5_MBs);
}

Y_UNIT_TEST(ForkStressTest)
{
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([] {
            const size_t S = 3000;
            auto* ptr = new char[S];
            DontOptimizeAway(ptr, S);
            for (int i = 0; i < 10; ++i) {
                Sleep(TDuration::MilliSeconds(100));
                if (::fork() == 0) {
                    _exit(0);
                }
            }
            delete[] ptr;
        });
        Cerr << "[" << TInstant::Now() << "] Starting thread" << Endl;
        Sleep(TDuration::MilliSeconds(100));
    }

    for (auto& thread : threads) {
        Cerr << "[" << TInstant::Now() << "] Joining thread" << Endl;
        thread.join();
    }
}

Y_UNIT_TEST(MemoryZones)
{
    auto check = [] (size_t size, EMemoryZone currentZone, EMemoryZone allocZone) {
        SetCurrentMemoryZone(currentZone);
        auto* ptr = Allocate(size);
        UNIT_ASSERT_EQUAL(GetAllocationMemoryZone(ptr), allocZone);
        Free(ptr);
        SetCurrentMemoryZone(EMemoryZone::Normal);
    };

    check(                         100,     EMemoryZone::Normal,     EMemoryZone::Normal);
    check(                         100, EMemoryZone::Undumpable,     EMemoryZone::Normal);
    check(LargeAllocationSizeThreshold,     EMemoryZone::Normal,     EMemoryZone::Normal);
    check(LargeAllocationSizeThreshold, EMemoryZone::Undumpable, EMemoryZone::Undumpable);

    UNIT_ASSERT_EQUAL(GetAllocationMemoryZone(nullptr), EMemoryZone::Unknown);
}

static std::atomic_flag Locked = {false};
static pthread_key_t key;

void MakeDeadlock(void* )
{
    Cerr << "Background thread stopping" << Endl;

    while (Locked.test_and_set()) {}
    Locked.clear();

    Cerr << "Background thread stoped" << Endl;
}

Y_UNIT_TEST(BackgroundThreadDeadlock)
{
    Cerr << "Test started" << Endl;
    pthread_key_create(&key, MakeDeadlock);

    EnableLogging([] (const TLogEvent& event) {
        Cerr << event.Message << Endl;

        if (void* ptr = pthread_getspecific(key); !ptr) {
            pthread_setspecific(key, (void*) 1);
        }
    });

    std::vector<TString> blobs(16, TString(1_GB, 'f'));
    blobs.clear();
    Sleep(TDuration::Seconds(2));

    Locked.test_and_set();

    std::thread forker([] {
        Cerr << "Forker started" << Endl;
        auto pid = fork();
        UNIT_ASSERT(pid >= 0);

        if (pid == 0) {
            exit(0);
        }
        Cerr << "Forker stopped" << Endl;
    });

    Sleep(TDuration::Seconds(1));

    // At this point background thread is stuck in TLS desctructor.
    // But allocations from another threads should proceed without blocking.

    std::thread locker([] {
        Cerr << "Locker started" << Endl;
        std::vector<TString> blobs(128, TString(256_KB, 'f'));
        Cerr << "Locker stopped" << Endl;
    });

    locker.join();
    Locked.clear();
    forker.join();
}

////////////////////////////////////////////////////////////////////////////////

} // Y_UNIT_TEST_SUITE
