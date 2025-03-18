#include "thread_namer.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/thread/pool.h>
#include <util/system/types.h>

#include <thread>

using NThreading::SetThreadNames;
using NThreading::TThreadNamer;

Y_UNIT_TEST_SUITE(ThreadNamerTests) {
    Y_UNIT_TEST(TestWithUtilThreadPool) {
        const size_t threadCount = 11;
        const auto pool = CreateThreadPool(threadCount, threadCount);
        const auto namer = TThreadNamer::Make("Util");
        for (size_t i = 0; i < threadCount; ++i) {
            Y_VERIFY(pool->AddFunc(namer->GetNamerJob()), "at i=%" PRISZT, i);
        }
        namer->Wait();

        // TODO(yazevnul): should check if name is actually matches promised patter, but
        // CurrentThread(G|S)etName acts strangely on Windows.
        pool->Stop();
    }

    Y_UNIT_TEST(TestWithUtilityFunction) {
        const size_t threadCount = 11;
        const auto pool = CreateThreadPool(threadCount, threadCount);
        SetThreadNames("Util", threadCount, pool.Get());
    }

    Y_UNIT_TEST(TestWithStlThreads) {
        std::thread threads[11];
        const auto namer = TThreadNamer::Make("Stl");
        for (auto& thread : threads) {
            thread = std::thread(namer->GetNamerJob());
        }
        namer->Wait();

        for (auto& thread : threads) {
            thread.join();
        }
    }
}
