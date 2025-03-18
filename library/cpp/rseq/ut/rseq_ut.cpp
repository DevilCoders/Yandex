#include <gtest/gtest.h>

#include <array>
#include <vector>
#include <thread>

#include <library/cpp/rseq/rseq.h>

TEST(RSeq, SimpleCounter)
{
    if (!RSeq::RegisterCurrentThread()) {
        GTEST_SKIP() << "RSeq is unavailable";
    }

    constexpr int nThreads = 200, nIncrements = 10000;

    std::array<intptr_t, CPU_SETSIZE> counters{};

    std::vector<std::thread> threads;
    for (int i = 0; i < nThreads; i++) {
        threads.emplace_back([&] {
            if (!RSeq::RegisterCurrentThread()) {
                FAIL() << "thread registration failed";
                return;
            }

            for (int j = 0; j < nIncrements; j++) {
                for (;;) {
                    int cpu = rseq_cpu_start();
                    int ret = rseq_addv(&counters[cpu], 1, cpu);
                    if (!ret) {
                        break;
                    }
                }

                sched_yield();
            }
        });
    }

    for (int i = 0; i < nThreads; i++) {
        threads[i].join();
    }

    int sum = 0;
    for (auto i : counters) {
        sum += i;
    }
    EXPECT_EQ(sum, nThreads * nIncrements);
}
