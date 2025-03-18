#include "queue.h"
#include "mtp_jobs.h"
#include "work_stealing.h"
#include "multitask.h"
#include "waiter.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/output.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/event.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>

#include <util/datetime/base.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/scope.h>

#include <util/random/random.h>

namespace {
    class TSimpleJob: public IObjectInQueue {
    public:
        TSimpleJob(TAutoPtr<IObjectInQueue> payload = nullptr)
            : Payload(payload)
        {
        }

        void Process(void*) override {
            ThreadId = TThread::CurrentThreadId();

            if (Payload)
                Payload->Process(nullptr);

            Sleep(TDuration::MilliSeconds(1));
            SetDone();
        }

        void SetDone() {
            AtomicSet(Done, 1);
        }

        bool IsDone() const {
            return AtomicGet(Done) == 1;
        }

        bool IsDoneInCurrentThread() const {
            return IsDone() && ThreadId == TThread::CurrentThreadId();
        }

    private:
        THolder<IObjectInQueue> Payload;
        TAtomic Done = 0;
        size_t ThreadId = 0;
    };
}

Y_UNIT_TEST_SUITE(TMtpJobsTest) {
    using TSimpleJob = TSimpleJob;

    Y_UNIT_TEST(TestSimpleMtpJobs) {
        TSimpleMtpJobs jobs;

        UNIT_ASSERT(!jobs.Closed());
        UNIT_ASSERT(jobs.Pop() == nullptr);

        for (size_t i = 0; i < 100; ++i) {
            THolder<TSimpleJob> job(new TSimpleJob);
            UNIT_ASSERT(jobs.Push(job.Get()));
            UNIT_ASSERT((i % 2 == 0 ? jobs.Pop() : jobs.WaitPop()) == job.Get()); // should not block
            UNIT_ASSERT(jobs.Pop() == nullptr);
        }

        TVector<TSimpleJob> many(10000);
        TSet<IObjectInQueue*> ptrs;
        for (size_t i = 0; i < many.size(); ++i) {
            UNIT_ASSERT(jobs.Push(&many[i]));
            ptrs.insert(&many[i]);
        }

        jobs.Close();
        UNIT_ASSERT(jobs.Closed());

        TSimpleJob someUnusedJob;
        UNIT_ASSERT(!jobs.Push(&someUnusedJob)); // cannot push in closed jobs

        for (size_t i = 0; i < many.size(); ++i) {
            IObjectInQueue* j = jobs.Pop();
            UNIT_ASSERT(j != nullptr);
            UNIT_ASSERT(ptrs.erase(j) == 1);
        }

        UNIT_ASSERT(ptrs.empty());
        UNIT_ASSERT(jobs.Pop() == nullptr);
        UNIT_ASSERT(jobs.WaitPop() == nullptr); // does not block
        UNIT_ASSERT(!jobs.Push(&someUnusedJob));
    }

    Y_UNIT_TEST(TestLimitedMtpJobs) {
        const size_t limit = 10;
        TLimitedMtpJobs jobs(limit);

        UNIT_ASSERT(!jobs.Closed());
        UNIT_ASSERT(jobs.Pop() == nullptr);

        TVector<TSimpleJob> many(100);
        for (size_t i = 0; i < many.size(); ++i) {
            UNIT_ASSERT_EQUAL(i < 10, jobs.Push(&many[i]));

            IObjectInQueue* j = jobs.WaitPop();
            UNIT_ASSERT(j);
            UNIT_ASSERT(jobs.Push(j));
        }
    }

    Y_UNIT_TEST(TestFixedMtpQueue) {
        for (size_t threads = 1; threads <= 10; ++threads) {
            TFixedMtpQueue q;
            q.Start(threads);

            TSimpleJob job1, job2, job3;

            UNIT_ASSERT(q.Add(&job1));
            UNIT_ASSERT(q.Add(&job2));
            UNIT_ASSERT(q.JobQueue()->Push(&job3));

            q.Stop();

            // make sure the queue processes all accepted jobs anyway (even for threads=0)
            for (auto j : {&job1, &job2, &job3}) {
                UNIT_ASSERT(j->IsDone());
            }
        }
    }

    Y_UNIT_TEST(TestFixedMtpQueueWithoutThreads) {
        TFixedMtpQueue q;
        q.Start(0); // no threads

        TSimpleJob job1, job2;

        UNIT_ASSERT(!q.Add(&job1));
        UNIT_ASSERT(q.JobQueue()->Push(&job2)); // still accepted!
        UNIT_ASSERT(!job2.IsDone());            // will be done after Stop()

        q.Stop();

        UNIT_ASSERT(!job1.IsDone());
        UNIT_ASSERT(job2.IsDoneInCurrentThread());
    }

    Y_UNIT_TEST(TestPriorityMtpJobs) {
        TPriorityMtpJobs majors;
        TMtpJobsPtr minors = majors.MinorJobQueue();
        UNIT_ASSERT(minors.Get() && minors->Pop() == nullptr && majors.Pop() == nullptr);

        TSimpleJob j1, j2, j3;

        UNIT_ASSERT(majors.Push(&j1));
        UNIT_ASSERT(minors->Pop() == nullptr && majors.Pop() == &j1); // minor jobs does not see major jobs
        UNIT_ASSERT(minors->Pop() == nullptr && majors.Pop() == nullptr);

        UNIT_ASSERT(minors->Push(&j1));
        UNIT_ASSERT(minors->WaitPop() == &j1 && majors.Pop() == nullptr);
        UNIT_ASSERT(minors->Pop() == nullptr && majors.Pop() == nullptr);

        UNIT_ASSERT(minors->Push(&j1));
        UNIT_ASSERT(majors.WaitPop() == &j1 && minors->Pop() == nullptr); // major jobs does see minor jobs
        UNIT_ASSERT(minors->Pop() == nullptr && majors.Pop() == nullptr);

        TMtpJobsPtr minors2 = majors.MinorJobQueue();
        UNIT_ASSERT(minors->Push(&j1) && minors2->Push(&j2) && majors.Push(&j3));
        UNIT_ASSERT(majors.WaitPop() == &j3); // priority
        UNIT_ASSERT(minors2->WaitPop() == &j2);
        UNIT_ASSERT(minors2->Pop() == nullptr);
        UNIT_ASSERT(majors.WaitPop() == &j1);
        UNIT_ASSERT(minors->Pop() == nullptr && majors.Pop() == nullptr);

        minors->Close();
        UNIT_ASSERT(!minors->Push(&j1) && minors2->Push(&j2) && majors.Push(&j3));
        minors2->Close();
        UNIT_ASSERT(!minors2->Push(&j1) && majors.Push(&j1));
        UNIT_ASSERT(!minors->WaitPop());
        UNIT_ASSERT(majors.WaitPop() && majors.WaitPop() && majors.WaitPop());
        UNIT_ASSERT(!minors2->WaitPop() && !majors.Pop());

        UNIT_ASSERT(majors.Push(&j1));
        TVector<TSimpleJob> many(100);
        for (size_t i = 0; i < many.size(); ++i)
            majors.MinorJobQueue()->Push(&many[i]);
        UNIT_ASSERT(majors.Push(&j2));

        majors.Close();
        UNIT_ASSERT(!majors.Push(&j3));

        UNIT_ASSERT(EqualToOneOf(majors.Pop(), &j1, &j2));
        UNIT_ASSERT(EqualToOneOf(majors.WaitPop(), &j1, &j2));

        for (size_t i = 0; i < many.size(); ++i)
            UNIT_ASSERT(majors.Pop());

        UNIT_ASSERT(!majors.Pop());
    }

    Y_UNIT_TEST(TestMtpWaiter) {
        for (size_t threads = 2; threads <= 10; ++threads) {
            TFixedMtpQueue q;
            q.Start(threads);

            TMtpJobWaiter waiter;
            TSimpleJob waitJob(MakeFunctionMtpJob([&]() {
                waiter.WaitAll();
            }));

            TSimpleJob job1, job2, job3;
            TWaitedMtpJob wjob1(&job1, waiter);
            TWaitedMtpJob wjob2(&job2, waiter);
            TWaitedMtpJob wjob3(&job3, waiter);

            UNIT_ASSERT(q.Add(&waitJob));
            UNIT_ASSERT(!waitJob.IsDone());

            UNIT_ASSERT(q.Add(&wjob1));
            UNIT_ASSERT(!waitJob.IsDone());

            UNIT_ASSERT(q.Add(&wjob2));
            UNIT_ASSERT(!waitJob.IsDone());

            UNIT_ASSERT(q.Add(&wjob3));

            q.Stop();

            for (auto j : {&job1, &job2, &job3}) {
                UNIT_ASSERT(j->IsDone());
            }

            UNIT_ASSERT(waitJob.IsDone());
        }
    }

    template <typename T>
    static void SetVar(T * var, T val) {
        if (var)
            *var = val;
    }

    class TMultiTaskJob: public TSimpleJob {
        bool* Deleted = nullptr;

    public:
        TMultiTaskJob(bool* deleted = nullptr)
            : Deleted(deleted)
        {
            SetVar(Deleted, false);
        }

        ~TMultiTaskJob() override {
            SetVar(Deleted, true);
        }
    };

    enum class SaveOp {
        NONE,
        COPY,
        MOVE
    };

    // This class reacts on moving and copying.
    // Therefore we can check whether it was saved internally somewhere or not.
    class TSavedFunc {
        SaveOp* Op = nullptr;

    public:
        TSavedFunc(SaveOp* op = nullptr)
            : Op(op)
        {
            SetVar(Op, SaveOp::NONE);
        }

        TSavedFunc(const TSavedFunc& other)
            : Op(other.Op)
        {
            SetVar(Op, SaveOp::COPY);
        }

        TSavedFunc(TSavedFunc&& other)
            : Op(other.Op)
        {
            SetVar(Op, SaveOp::MOVE);
        }

        void operator()(void*) {
        }
    };

    static inline void Blink() {
        ::Sleep(TDuration::MilliSeconds(50));
    }

    Y_UNIT_TEST(TestMtpMultiTask) {
        bool del1 = false, del2 = false;

        TMultiTaskJob job1(&del1);
        TMultiTaskJob* job2 = new TMultiTaskJob(&del2);
        TMultiTaskJob job3;

        {
            TMtpMultiTask mtask;

            mtask.AddJob(&job1);
            UNIT_ASSERT(!job1.IsDone()); // not started yet

            mtask.TakeJob(job2);
            UNIT_ASSERT(!job2->IsDone());

            mtask.Start(new TDummyMtpJobs); // always refuses jobs

            UNIT_ASSERT(!job1.IsDone()); // delayed
            UNIT_ASSERT(!job2->IsDone());

            mtask.AddJob(&job3);         // can add after start
            UNIT_ASSERT(!job3.IsDone()); // but it is delayed anyway

            mtask.Wait();

            UNIT_ASSERT(!del1);
            UNIT_ASSERT(!del2);

            UNIT_ASSERT(job1.IsDone());
            UNIT_ASSERT(job2->IsDone());
            UNIT_ASSERT(job3.IsDone());
        }

        UNIT_ASSERT(!del1);
        UNIT_ASSERT(del2);

        {
            TMtpJobsPtr jobs = new TLimitedMtpJobs(1);
            TFixedMtpQueue q;
            q.Start(1, jobs);

            TMtpMultiTask mtask;

            TString done4 = "no";
            TMultiTaskJob job5, job6;

            mtask.AddFunc([&done4]() { done4 = "yes"; });
            mtask.AddJob(&job5); // should be refused and delayed

            UNIT_ASSERT_EQUAL(done4, "no");
            UNIT_ASSERT(!job5.IsDone());

            mtask.Start(jobs);
            Blink();

            UNIT_ASSERT_EQUAL(done4, "yes");
            UNIT_ASSERT(!job5.IsDone()); // delayed

            mtask.AddJob(&job6);
            Blink();

            UNIT_ASSERT(!job5.IsDone()); // still delayed
            UNIT_ASSERT(job6.IsDone());

            SaveOp expectCopy(SaveOp::NONE);
            SaveOp expectMove(SaveOp::NONE);
            {
                TSavedFunc f1(&expectCopy);
                TSavedFunc f2(&expectMove);

                mtask.AddFunc(f1);
                mtask.AddFunc(std::move(f2));
            }

            mtask.Wait();

            UNIT_ASSERT(expectCopy == SaveOp::COPY);
            UNIT_ASSERT(expectMove == SaveOp::MOVE);

            UNIT_ASSERT(job5.IsDone());
        }

        {
            // using IThreadPool
            TThreadPool q;
            q.Start(5);

            TMtpMultiTask mtask;
            TVector<TMultiTaskJob> jobs(10);

            for (TMultiTaskJob& j : jobs)
                mtask.AddJob(&j);

            for (const TMultiTaskJob& j : jobs)
                UNIT_ASSERT(!j.IsDone());

            mtask.Process(q);

            size_t doneInCurrentThread = 0;
            for (const TMultiTaskJob& j : jobs) {
                UNIT_ASSERT(j.IsDone());
                doneInCurrentThread += j.IsDoneInCurrentThread() ? 1 : 0;
            }
            UNIT_ASSERT(doneInCurrentThread >= 1);
            UNIT_ASSERT(doneInCurrentThread < jobs.size());
        }

        {
            // locally
            TMtpMultiTask mtask;
            TVector<TMultiTaskJob> jobs(10);

            for (TMultiTaskJob& j : jobs)
                mtask.AddJob(&j);

            mtask.ProcessLocally();

            for (const TMultiTaskJob& j : jobs)
                UNIT_ASSERT(j.IsDoneInCurrentThread());
        }

        {
            // using tmp internal queue
            for (size_t threads = 0; threads < 5; ++threads) {
                TMtpMultiTask mtask;
                TVector<TMultiTaskJob> jobs(10);

                for (TMultiTaskJob& j : jobs)
                    mtask.AddJob(&j);

                for (TMultiTaskJob& j : jobs)
                    UNIT_ASSERT(!j.IsDone());

                mtask.Process(threads);

                size_t doneInCurrentThread = 0;
                for (const TMultiTaskJob& j : jobs) {
                    UNIT_ASSERT(j.IsDone());
                    doneInCurrentThread += j.IsDoneInCurrentThread() ? 1 : 0;
                }

                if (threads <= 1) {
                    UNIT_ASSERT(doneInCurrentThread == jobs.size());
                } else {
                    //Cerr << "threads=" << threads << ", doneInCurrentThread=" << doneInCurrentThread << Endl;
                    UNIT_ASSERT(doneInCurrentThread >= 1);
                    UNIT_ASSERT(doneInCurrentThread < jobs.size());
                }
            }
        }
    }

    void TestWithExceptions(bool exceptionFromMainThread) {
        TWorkStealingMtpQueue queue;
        queue.Init(0);
        const size_t threadsCount = 10;
        queue.Start(threadsCount, 0);

        const size_t mainTestThread = TThread::CurrentThreadId();
        TSystemEvent event;
        TAtomic startedTasks = 0;
        TAtomic processedTasks = 0;

        class TTestException: public yexception {
        };

        auto job = [exceptionFromMainThread, mainTestThread,
                    &event,
                    &startedTasks, &processedTasks] {
            AtomicIncrement(startedTasks);

            Y_DEFER {
                AtomicIncrement(processedTasks);
            };

            Sleep(TDuration::MilliSeconds(RandomNumber<ui32>(100)));

            auto cid = TThread::CurrentThreadId();
            const bool throwException = exceptionFromMainThread ? cid == mainTestThread : cid != mainTestThread;

            if (TThread::CurrentThreadId() == mainTestThread) {
                event.Signal();
            } else {
                event.Wait(); // Guarantee that there will be at least one job in current test thread
            }

            if (throwException) {
                throw TTestException();
            }
        };

        TMtpMultiTask mtask;
        for (size_t i = 0; i < threadsCount + 1; ++i) {
            mtask.AddFunc(job);
        }

        // Check that exceptions in tasks are passed to caller
        UNIT_ASSERT_EXCEPTION(mtask.Process(queue), TTestException);

        // Check that engine waited all created tasks or canceled them
        UNIT_ASSERT_VALUES_EQUAL(AtomicGet(startedTasks), AtomicGet(processedTasks));
    }

    Y_UNIT_TEST(TestPassesExceptionsFromMainThread) {
        TestWithExceptions(true);
    }

    Y_UNIT_TEST(TestPassesExceptionsFromStealingThreads) {
        TestWithExceptions(false);
    }
}

class TWorkStealingTester {
public:
    class TStat {
    public:
        struct TItem {
            size_t Major;
            size_t Minor;
            TDuration Duration;

            TString DebugName() const {
                return Minor ? TString::Join(ToString(Major), ".", ToString(Minor))
                             : TString::Join("(", ToString(Major), ")");
            }
        };

        void Register(size_t major, size_t minor, TInstant started) {
            TDuration dur = Now() - started;
            with_lock (Mutex) {
                auto ins = Threads.insert({TThread::CurrentThreadId(), {}});
                ins.first->second.push_back({major, minor, dur});
            }
        }

        struct TThreadSummary {
            size_t Majors = 0;
            size_t OwnedMinors = 0;
            size_t StolenMinors = 0;

            TDuration MinorsDuration;
            TString DebugString;

            void DebugPrint(IOutputStream& out, int i) const {
                out << "Thread #" << i << " completed " << Majors << "+" << OwnedMinors << "+" << StolenMinors
                    << " tasks in " << MinorsDuration << ":"
                    << DebugString << Endl;
            }

            bool MajorsInRange(size_t min, size_t max) const {
                bool ret = Majors >= min && Majors <= max;
                if (!ret)
                    DebugPrint(Cerr, -1); // print myself before failing assert
                return ret;
            }

            bool TotalMinorsInRange(size_t min, size_t max) const {
                size_t totalMinors = OwnedMinors + StolenMinors;
                bool ret = totalMinors >= min && totalMinors <= max;
                if (!ret)
                    DebugPrint(Cerr, -1); // print myself before failing assert
                return ret;
            }

            bool OwnMinorsAtLeast(size_t expectedMinorsPerMajor) const {
                bool ret = OwnedMinors >= Majors * expectedMinorsPerMajor;
                if (!ret)
                    DebugPrint(Cerr, -1); // print myself before failing assert
                return ret;
            }
        };

        void CalcSummary(size_t threadCount) {
            Summary.resize(Max(Threads.size(), threadCount));
            size_t idx = 0;
            for (const auto& it : Threads) {
                TSet<size_t> majors;
                for (const TItem& item : it.second) {
                    if (!item.Minor)
                        majors.insert(item.Major);

                    Summary[idx].DebugString.append(" ").append(item.DebugName());
                }

                Summary[idx].Majors = majors.size();

                for (const TItem& item : it.second) {
                    if (item.Minor) {
                        Summary[idx].MinorsDuration += item.Duration;
                        if (majors.contains(item.Major))
                            Summary[idx].OwnedMinors += 1;
                        else
                            Summary[idx].StolenMinors += 1;
                    }
                }
                ++idx;
            }
        }

        void DebugPrint(IOutputStream& out, size_t threadCount, size_t majorCount, size_t minorCount) const {
            out << "=== " << threadCount << " threads, " << majorCount << " majors, " << minorCount << " minors ===\n";
            for (size_t i = 0; i < Summary.size(); ++i)
                Summary[i].DebugPrint(out, i);
        }

        bool MajorsInRange(size_t min, size_t max) const {
            return AllOf(Summary, [=](const TThreadSummary& s) { return s.MajorsInRange(min, max); });
        }

        bool TotalMinorsInRange(size_t min, size_t max) const {
            return AllOf(Summary, [=](const TThreadSummary& s) { return s.TotalMinorsInRange(min, max); });
        }

        bool OwnMinorsAtLeast(size_t expectedMinorsPerMajor) const {
            return AllOf(Summary, [=](const TThreadSummary& s) { return s.OwnMinorsAtLeast(expectedMinorsPerMajor); });
        }

    private:
        TMutex Mutex;
        THashMap<size_t, TVector<TItem>> Threads;

        TVector<TThreadSummary> Summary;
    };

    class TTaskBase: public TSimpleJob {
    public:
        TTaskBase(TStat& stat, size_t major, size_t minor = 0)
            : Stat(&stat)
            , MajorId(major)
            , MinorId(minor)
        {
        }

        void RegisterDone(TInstant started) {
            Stat->Register(MajorId, MinorId, started);
            TSimpleJob::SetDone();
        }

        void WaitDone() {
            while (!IsDone()) {
            }
        }

    private:
        TStat* Stat = nullptr;
        size_t MajorId = 0;
        size_t MinorId = 0;
    };

    struct TMinorTask: public TTaskBase {
        TMinorTask(TStat& stat, size_t major, size_t minor)
            : TTaskBase(stat, major, minor)
        {
        }

        void CalcPrimes() {
            TVector<size_t> primes;
            primes.push_back(2);
            for (size_t i = 3; i < 50000; ++i) {
                bool isprime = true;
                for (size_t j = 0; j < primes.size(); ++j) {
                    if (i % primes[j] == 0) {
                        isprime = false;
                        break;
                    }
                }
                if (isprime)
                    primes.push_back(i);
            }
        }

        void DoSomeWork() {
            Sleep(TDuration::MilliSeconds(50));
            //CalcPrimes();
        }

        void Process(void*) override {
            TInstant started = Now();
            if (IsDone())
                ythrow yexception() << "Already processed";

            DoSomeWork();

            RegisterDone(started);
        }
    };

    struct TMajorTask: public TTaskBase {
        TPriorityMtpJobs& ParentJobs;
        TVector<TMinorTask> Minors;

        TMajorTask(TPriorityMtpJobs& parentJobs, TStat& stat, size_t major, size_t minorCount)
            : TTaskBase(stat, major)
            , ParentJobs(parentJobs)
        {
            for (size_t i = 1; i <= minorCount; ++i)
                Minors.push_back(TMinorTask(stat, major, i));
        }

        void Process(void*) override {
            TInstant started = Now();

            // for more stable results
            Sleep(TDuration::MilliSeconds(10));

            //ProcessMinorsManually();
            ProcessMinorsUsingMultiTask();

            if (!AllMinorsDone())
                ythrow yexception() << "Not all minors done";

            RegisterDone(started);
        }

        bool AllMinorsDone() const {
            for (const TMinorTask& minor : Minors)
                if (!minor.IsDone())
                    return false;
            return true;
        }

        void ProcessMinorsManually() {
            TMtpJobsPtr localJobs = ParentJobs.MinorJobQueue();

            for (TMinorTask& minor : Minors)
                if (!localJobs->Push(&minor))
                    ythrow yexception() << "Cannot add minor";

            localJobs->Close();

            while (IObjectInQueue* t = localJobs->Pop()) {
                t->Process(nullptr);
            }

            // we should wait here for tasks, stolen by other threads
            for (TMinorTask& minor : Minors)
                minor.WaitDone();
        }

        void ProcessMinorsUsingMultiTask() {
            TMtpMultiTask task;
            for (TMinorTask& minor : Minors)
                task.AddJob(&minor);

            task.Process(ParentJobs.MinorJobQueue());
        }
    };

    TAutoPtr<TStat> Run(size_t threadCount, size_t majorCount, size_t minorCount, bool waitBeforeStop = true) {
        TPriorityMtpJobs* jobs = new TPriorityMtpJobs();
        TMtpJobsPtr ptr(jobs);

        TFixedMtpQueue q;
        q.Start(threadCount, ptr);

        TAutoPtr<TStat> stat(new TStat);
        TVector<TMajorTask> majors;
        for (size_t i = 1; i <= majorCount; ++i)
            majors.push_back(TMajorTask(*jobs, *stat, i, minorCount));

        for (TMajorTask& t : majors) {
            UNIT_ASSERT(q.Add(&t));
        }

        if (waitBeforeStop) {
            for (TMajorTask& t : majors)
                t.WaitDone();
        }

        q.Stop();

        for (TMajorTask& t : majors) {
            UNIT_ASSERT(t.IsDone());
        }

        stat->CalcSummary(threadCount);
        //stat->DebugPrint(Cerr, threadCount, majorCount, minorCount);

        return stat;
    }
};
/*
this test is unstable and should be improved

Y_UNIT_TEST_SUITE(TWorkStealingTest) {
    Y_UNIT_TEST(TestWorkStealing) {
        TWorkStealingTester t;
        TAutoPtr<TWorkStealingTester::TStat> stat;

        stat = t.Run(1, 1, 1);
        UNIT_ASSERT(stat->MajorsInRange(1, 1));
        UNIT_ASSERT(stat->TotalMinorsInRange(1, 1));
        UNIT_ASSERT(stat->OwnMinorsAtLeast(1));

        stat = t.Run(1, 1, 10);
        UNIT_ASSERT(stat->MajorsInRange(1, 1));
        UNIT_ASSERT(stat->TotalMinorsInRange(10, 10));
        UNIT_ASSERT(stat->OwnMinorsAtLeast(10));

        stat = t.Run(5, 1, 5, true);
        UNIT_ASSERT(stat->MajorsInRange(0, 1));
        UNIT_ASSERT(stat->TotalMinorsInRange(0, 2));
        UNIT_ASSERT(stat->OwnMinorsAtLeast(1));

        stat = t.Run(5, 1, 5, false);
        UNIT_ASSERT(stat->MajorsInRange(0, 1));
        UNIT_ASSERT(stat->TotalMinorsInRange(0, 5));    // unstable due to early non-waiting closing,
        UNIT_ASSERT(stat->OwnMinorsAtLeast(1));         // just to make sure all done

        stat = t.Run(5, 1, 10);
        UNIT_ASSERT(stat->MajorsInRange(0, 1));
        UNIT_ASSERT(stat->TotalMinorsInRange(0, 5));
        UNIT_ASSERT(stat->OwnMinorsAtLeast(2));

        stat = t.Run(5, 2, 10);
        UNIT_ASSERT(stat->MajorsInRange(0, 1));
        UNIT_ASSERT(stat->TotalMinorsInRange(2, 6));
        UNIT_ASSERT(stat->OwnMinorsAtLeast(2));

        stat = t.Run(5, 5, 10);
        UNIT_ASSERT(stat->MajorsInRange(1, 1));
        UNIT_ASSERT(stat->TotalMinorsInRange(8, 12));
        UNIT_ASSERT(stat->OwnMinorsAtLeast(8));

        stat = t.Run(5, 7, 10);
        UNIT_ASSERT(stat->MajorsInRange(1, 2));
        UNIT_ASSERT(stat->TotalMinorsInRange(12, 16));
        UNIT_ASSERT(stat->OwnMinorsAtLeast(5));

        stat = t.Run(5, 10, 10);
        UNIT_ASSERT(stat->MajorsInRange(1, 3));
        UNIT_ASSERT(stat->TotalMinorsInRange(15, 25));
        UNIT_ASSERT(stat->OwnMinorsAtLeast(8));

        stat = t.Run(100, 100, 5);
        UNIT_ASSERT(stat->MajorsInRange(1, 1));
        UNIT_ASSERT(stat->TotalMinorsInRange(3, 7));
        UNIT_ASSERT(stat->OwnMinorsAtLeast(2));

        stat = t.Run(100, 200, 5);
        UNIT_ASSERT(stat->MajorsInRange(1, 3));
        UNIT_ASSERT(stat->TotalMinorsInRange(5, 15));
        UNIT_ASSERT(stat->OwnMinorsAtLeast(2));
    }
}
*/
