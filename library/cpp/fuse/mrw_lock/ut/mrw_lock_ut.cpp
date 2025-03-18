#include <library/cpp/fuse/mrw_lock/mrw_lock.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/system/rwlock.h>
#include <util/system/thread.h>

using namespace NThreading;

Y_UNIT_TEST_SUITE(TMRWLockTests) {

    constexpr int CYCLES = 100;

    void Pause() {
        Sleep(TDuration::MilliSeconds(50));
    }

    Y_UNIT_TEST(TestAquireReadTwice) {
        TMRWLock lock;
        lock.AcquireRead();
        lock.AcquireRead();
        UNIT_ASSERT_EQUAL(lock.HoldsWriteLock(), false);
        lock.ReleaseRead();
        lock.ReleaseRead();
    }

    Y_UNIT_TEST(TestAquireWriteTwice) {
        TMRWLock lock;
        lock.AcquireWrite();
        UNIT_ASSERT_EQUAL(lock.HoldsWriteLock(), true);
        lock.ReleaseWrite();
        lock.AcquireWrite();
        lock.ReleaseWrite();
    }

    Y_UNIT_TEST(TestReadsAreParallel) {
        TMRWLock lock;
        int state = 0;
        lock.AcquireRead();

        TThread thread([&](){
            lock.AcquireRead();
            UNIT_ASSERT_EQUAL(state, 0);
            lock.ReleaseRead();
        });
        thread.Start();
        Pause();
        state = 1;
        lock.ReleaseRead();
        thread.Join();
    }

    Y_UNIT_TEST(TestWritesAreConcurrent) {
        int state = 0;
        TMRWLock lock;
        lock.AcquireWrite();

        TThread thread([&](){
            lock.AcquireWrite();
            UNIT_ASSERT_EQUAL(lock.HoldsWriteLock(), true);
            UNIT_ASSERT_EQUAL(state, 1);
            lock.ReleaseWrite();
        });

        thread.Start();
        Pause();
        state = 1;
        lock.ReleaseWrite();
        thread.Join();
    }

    Y_UNIT_TEST(TestReadsWaitWrites) {
        int state = 0;
        TMRWLock lock;
        lock.AcquireWrite();

        TThread thread([&](){
            lock.AcquireRead();
            UNIT_ASSERT_EQUAL(state, 1);
            lock.ReleaseRead();
        });

        thread.Start();
        Pause();
        state = 1;
        lock.ReleaseWrite();
        thread.Join();
    }

    Y_UNIT_TEST(TestAllReadsStartTogether) {
        int state = 0;
        TMRWLock lock;
        lock.AcquireWrite();

        TThread thread1([&](){
            lock.AcquireRead();
            UNIT_ASSERT_EQUAL(state, 1);
            Pause();
            state = 2;
            lock.ReleaseRead();
        });
        TThread thread2([&](){
            lock.AcquireRead();
            UNIT_ASSERT_EQUAL(state, 1);
            Pause();
            state = 2;
            lock.ReleaseRead();
        });

        thread1.Start();
        thread2.Start();
        Pause();
        state = 1;
        lock.ReleaseWrite();
        thread1.Join();
        thread2.Join();
    }

    Y_UNIT_TEST(TestWritesWaitReads) {
        int state = 0;
        TMRWLock lock;
        lock.AcquireRead();
        lock.AcquireRead();

        TThread thread([&](){
            lock.AcquireWrite();
            UNIT_ASSERT_EQUAL(state, 2);
            lock.ReleaseWrite();
        });

        thread.Start();
        Pause();
        state = 1;
        lock.ReleaseRead();
        state = 2;
        lock.ReleaseRead();
        thread.Join();
    }

    Y_UNIT_TEST(TestWritersBeforeReaders) {
        int state = 0;
        TMRWLock lock;
        lock.AcquireWrite();

        TThread threadRead([&](){
            lock.AcquireRead();
            UNIT_ASSERT_EQUAL(state, 2);
            lock.ReleaseRead();
        });
        threadRead.Start();
        TThread threadWrite([&](){
            lock.AcquireWrite();
            UNIT_ASSERT_EQUAL(state, 1);
            state = 2;
            lock.ReleaseWrite();
        });
        threadWrite.Start();

        Pause();
        state = 1;
        lock.ReleaseWrite();
        threadRead.Join();
        threadWrite.Join();
    }

    Y_UNIT_TEST(TestReleaseReadInDifferentThread) {
        TMRWLock lock;
        lock.AcquireRead();

        TThread thread([&](){
            Pause();
            lock.ReleaseRead();
        });

        thread.Start();
        thread.Join();
        lock.AcquireWrite();
        lock.ReleaseWrite();
    }

    // this test crashes if uses TRWMutex
    Y_UNIT_TEST(TestReleaseWriteInDifferentThread) {
        TMRWLock lock;
        lock.AcquireWrite();

        TThread thread([&](){
            Pause();
            lock.ReleaseWrite();
        });

        thread.Start();
        thread.Join();
        lock.AcquireWrite();
        lock.ReleaseWrite();
    }

    Y_UNIT_TEST(TestReadWriteGuard) {
        TMRWLock lock;
        {
            auto readGuard1 = lock.LockRead();
            Y_UNUSED(readGuard1);
            {
                auto readGuard2 = lock.LockRead();
                UNIT_ASSERT_EQUAL(readGuard2.HoldsWriteLock(), false);
                Y_UNUSED(readGuard2);
            }
        }
        {
            auto writeGuard = lock.LockWrite();
            UNIT_ASSERT_EQUAL(writeGuard.HoldsWriteLock(), true);
            Y_UNUSED(writeGuard);
        }
        {
            auto writeGuard = lock.LockWrite();
            Y_UNUSED(writeGuard);
        }
    }

    Y_UNIT_TEST(TestPassWriteGuardToAnotherThread) {
        TMRWLock lock;
        int state = 0;
        TThread thread([guard = lock.LockWrite(), &state]() {
            Pause();
            state = 1;
            // guard implicitly released
        });
        thread.Start();
        thread.Join();
        lock.LockWrite();
        UNIT_ASSERT_EQUAL(state, 1);
    }

    Y_UNIT_TEST(TestPassReadGuardToAnotherThread) {
        TMRWLock lock;
        int state = 0;

        TThread thread([guard = lock.LockRead(), &state]() {
            Pause();
            state = 1;
            // guard implicitly released
        });
        thread.Start();
        thread.Join();
        {
            lock.LockRead();
            UNIT_ASSERT_EQUAL(state, 1);
        }

        {
            lock.LockWrite();
            UNIT_ASSERT_EQUAL(state, 1);
        }
    }


    Y_UNIT_TEST(TestReadContention) {
        TMRWLock lock;
        TInstant start = TInstant::Now();
        int state = 0;
        lock.AcquireWrite();

        TVector<THolder<TThread>> threads;
        for(int i = 0; i < CYCLES; i++) {
            THolder<TThread> threadPtr = MakeHolder<TThread>([&]() {
                lock.AcquireRead();
                UNIT_ASSERT_EQUAL(state, 1);
                lock.ReleaseRead();
            });
            threadPtr->Start();
            threads.push_back(std::move(threadPtr));
        }
        state = 1;
        lock.ReleaseWrite();
        for (auto& threadPtr: threads) {
            threadPtr->Join();
        }
        Cerr << "Read Contention test run in " << (TInstant::Now() - start) << Endl;
    }

    Y_UNIT_TEST(TestReadWriteContention) {
        TMRWLock lock;
        lock.AcquireWrite();
        TInstant start = TInstant::Now();
        int state = 0;
        TVector<THolder<TThread>> threads;
        for(int i = 0; i < CYCLES; i++) {
            THolder<TThread> threadPtr = MakeHolder<TThread>([i, &lock, &state]() {
                if (i % 10 == 0) {
                    lock.AcquireWrite();
                    state++;
                    lock.ReleaseWrite();
                } else {
                    lock.AcquireRead();
                    lock.ReleaseRead();
                }
            });
            threadPtr->Start();
            threads.push_back(std::move(threadPtr));
        }
        lock.ReleaseWrite();
        for (auto& threadPtr: threads) {
            threadPtr->Join();
        }
        lock.AcquireRead();
        lock.ReleaseRead();
        UNIT_ASSERT_EQUAL(state, CYCLES / 10);
        Cerr << "Reads&Write contention test run in " << (TInstant::Now() - start) << Endl;
    }

}
