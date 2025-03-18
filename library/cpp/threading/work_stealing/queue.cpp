#include "queue.h"

#include <util/generic/vector.h>
#include <util/system/mutex.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/condvar.h>
#include <util/system/defaults.h>
#include <util/thread/factory.h>

class TMtpExecutor: private IThreadFactory::IThreadAble {
public:
    TMtpExecutor(IMtpJobs* jobs, IThreadPool* tsrAlloc)
        : Jobs(jobs)
        , TsrAlloc(tsrAlloc)
    {
    }

    ~TMtpExecutor() override {
        try {
            Stop();
        } catch (...) {
        }
    }

    void Start(IThreadFactory* threadPool, size_t threadCount) {
        ThreadRefs.resize(threadCount);
        for (size_t i = 0; i < threadCount; ++i)
            ThreadRefs[i] = threadPool->Run(this);

        // wait until all these threads have started
        with_lock (StartMutex) {
            while (RunningThreads < threadCount)
                StartCond.Wait(StartMutex);
        }
    }

    void Stop() {
        Jobs->Close();

        with_lock (StopMutex) {
            while (RunningThreads > 0)
                StopCond.Wait(StopMutex);

            ThreadRefs.clear();
        }
    }

private:
    // Implements IThreadFactory::IThreadAble
    void DoExecute() override {
        with_lock (StartMutex) {
            ++RunningThreads;
            StartCond.Signal();
        }

        {
            using TTsr = IThreadPool::TTsr;
            THolder<TTsr> tsr(new TTsr(TsrAlloc));

            while (IObjectInQueue* job = Jobs->WaitPop())
                ProcessWithoutExceptions(job, *tsr);
            // TTsr::~TTsr must be called here and not in the end of DoExecute();
            // it references this->TsrAlloc, and "this" can become invalid immediately after StopCond.BroadCast()
        }

        with_lock (StopMutex) {
            --RunningThreads;
            StopCond.BroadCast();
        }
    }

    void ProcessWithoutExceptions(IObjectInQueue* job, void* tsr) noexcept {
        try {
            try {
                job->Process(tsr);
            } catch (...) {
                Cdbg << "[mtp queue] " << CurrentExceptionMessage() << Endl;
            }
        } catch (...) {
        }
    }

private:
    IMtpJobs* Jobs = nullptr;
    IThreadPool* TsrAlloc = nullptr;

    using TThreadRef = TAutoPtr<IThreadFactory::IThread>;
    TVector<TThreadRef> ThreadRefs;
    size_t RunningThreads = 0;

    TCondVar StartCond, StopCond;
    TMutex StartMutex, StopMutex;
};

TFixedMtpQueue::TFixedMtpQueue() {
}

TFixedMtpQueue::~TFixedMtpQueue() {
    try {
        Stop();
    } catch (...) {
    }
}

void TFixedMtpQueue::Start(size_t threadCount, TMtpJobsPtr jobs) {
    if (Jobs)
        ythrow yexception() << "Already started";

    Jobs = jobs;

    // do not start executor without threads
    if (threadCount) {
        Executor.Reset(new TMtpExecutor(Jobs.Get(), this));
        Executor->Start(/*this*/ SystemThreadFactory(), threadCount); // TODO: SystemThreadFactory() or TThreadFactoryHolder?
    }
}

void TFixedMtpQueue::Start(size_t threadCount, size_t queueSizeLimit) {
    TMtpJobsPtr jobs;
    if (queueSizeLimit)
        jobs.Reset(new TLimitedMtpJobs(queueSizeLimit));
    else
        jobs.Reset(new TSimpleMtpJobs());

    Start(threadCount, jobs);
}

bool TFixedMtpQueue::Add(IObjectInQueue* obj) {
    if (Y_UNLIKELY(!Jobs))
        ythrow yexception() << "Not started";

    // In order to use TFixedMtpQueue in TMtpTask safely:
    // refuse all jobs if there is not worker threads
    if (!Executor)
        return false;

    return Jobs->Push(obj);
}

void TFixedMtpQueue::Stop() noexcept {
    // IThreadPool guarantees it processes all accepted jobs
    if (Jobs)
        Jobs->Finalize();

    Executor.Destroy();
    Jobs.Reset(nullptr);
}

TWorkStealingMtpQueue::~TWorkStealingMtpQueue() = default;

void TWorkStealingMtpQueue::Init(size_t queueSizeLimit) {
    if (JobQueue)
        ythrow yexception() << "TWorkStealingMtpQueue has been already initialized";

    JobQueue = new TPriorityMtpJobs(CreateMajorJobs(queueSizeLimit));
}

void TWorkStealingMtpQueue::Start(size_t threadCount, size_t) {
    if (!JobQueue)
        ythrow yexception() << "TWorkStealingMtpQueue is not initialized";

    TFixedMtpQueue::Start(threadCount, JobQueue);
}

TMtpJobsPtr TWorkStealingMtpQueue::MinorJobQueue() {
    if (JobQueue)
        return JobQueue->MinorJobQueue();
    else
        return new TDummyMtpJobs(); // faked mtp
}

TMtpJobsPtr TWorkStealingMtpQueue::CreateMajorJobs(size_t queueSizeLimit) {
    return queueSizeLimit > 0 ? new TLimitedMtpJobs(queueSizeLimit)
                              : new TSimpleMtpJobs();
}
