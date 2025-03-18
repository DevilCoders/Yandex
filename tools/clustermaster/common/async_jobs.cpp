#include "async_jobs.h"

#include "log.h"
#include "thread_util.h"

#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/system/condvar.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>

#include <atomic>

class TAsyncJobs::TImpl {
private:
    class TJobWrapper final : public IObjectInQueue {
    private:
        THolder<TAsyncJobs::IJob> Job;
        TAsyncJobs::TImpl& Parent;

    public:
        TJobWrapper(THolder<TAsyncJobs::IJob> job, TAsyncJobs::TImpl& parent)
            : Job(std::move(job))
            , Parent(parent)
        {
            TGuard guard(Parent.ActiveJobsMutex);
            ++Parent.ActiveJobs;
        }
        ~TJobWrapper() noexcept {
            TGuard guard(Parent.ActiveJobsMutex);
            if (--Parent.ActiveJobs == 0) {
                Parent.JobsDone.Signal();
            }
        }
        void Process(void* param) override {
            Job->Process(param);
        }
    };

private:
    size_t PoolThreads;
    TThreadPool Pool;
    TMultiMap<TInstant, THolder<TJobWrapper>> ScheduledJobs;
    TMutex ActiveJobsMutex;
    TCondVar JobsDone;
    size_t ActiveJobs = 0;
    TMutex QueueMutex;
    TCondVar QueueNotEmpty;
    bool Terminate = false;
    TThread Thread;

private:
    void ThreadProc() noexcept {
        LOG("Started job scheduler thread");
        SetCurrentThreadName("job scheduler");

        TGuard guard(QueueMutex);
        while (!Terminate) {
            const auto deadline = ActivateJobs();
            QueueNotEmpty.WaitD(QueueMutex, deadline);
        }

        LOG("Finished job scheduler thread");
    }

    TInstant ActivateJobs() {
        for (auto it = begin(ScheduledJobs)
                ; it != end(ScheduledJobs) && it->first <= TInstant::Now()
                ; it = ScheduledJobs.erase(it))
        {
            Y_VERIFY(Pool.AddAndOwn(std::move(it->second)));
        }

        if (!ScheduledJobs.empty()) {
            return begin(ScheduledJobs)->first;
        }
        return TInstant::Max();
    };

public:
    TImpl(size_t threads)
        : PoolThreads(threads)
        , Pool()
        , ScheduledJobs()
        , ActiveJobsMutex()
        , JobsDone()
        , ActiveJobs(0)
        , QueueMutex()
        , QueueNotEmpty()
        , Terminate(false)
        , Thread([this] { this->ThreadProc(); })
    {
        LOG("Created job scheduler with " << threads << " threads");
    }

    ~TImpl() noexcept {
        Stop();
        LOG("Destroyed job scheduler");
    }

    void Start() {
        Pool.Start(PoolThreads);
        Terminate = false;
        Thread.Start();
        LOG("Started job scheduler");
    }

    void Stop() noexcept {
        bool previous = false;
        {
            TGuard guard(QueueMutex);
            previous = Terminate;
            Terminate = true;
            ScheduledJobs.clear(); // drop unstarted jobs to prevent possible dead wait on WaitForCompletion
            QueueNotEmpty.Signal();
        }
        Thread.Join();
        Pool.Stop();
        if (!previous) {
            LOG("Stopped job scheduler");
        }
    }

    void Add(THolder<TAsyncJobs::IJob> job) {
        auto wrapper = MakeHolder<TJobWrapper>(std::move(job), *this);
        TGuard guard(QueueMutex);
        Y_ENSURE(!Terminate, "The scheduler has been stopped already");
        Pool.SafeAddAndOwn(std::move(wrapper));
    }

    void Add(THolder<TAsyncJobs::IJob> job, TInstant time) {
        auto wrapper = MakeHolder<TJobWrapper>(std::move(job), *this);
        TGuard guard(QueueMutex);
        Y_ENSURE(!Terminate, "The scheduler has been stopped already");
        Y_VERIFY(ScheduledJobs.emplace(time, std::move(wrapper))->second);
        QueueNotEmpty.Signal();
    }

    void WaitForCompletion() {
        TGuard guard(ActiveJobsMutex);
        if (ActiveJobs != 0) {
            DEBUGLOG("Job scheduler: waiting for jobs completion. Active jobs = " << ActiveJobs);
            do {
                JobsDone.WaitI(ActiveJobsMutex);
            } while (ActiveJobs != 0);
        }
        DEBUGLOG("Job scheduler: all jobs done");
    }
};

TAsyncJobs::TAsyncJobs(size_t threads)
    : Impl(new TImpl(threads))
{
}

TAsyncJobs::~TAsyncJobs() noexcept {
    Stop();
}

void TAsyncJobs::Start() {
    Impl->Start();
}

void TAsyncJobs::Stop() noexcept {
    Impl->Stop();
}

void TAsyncJobs::Add(THolder<IJob> job) {
    Impl->Add(std::move(job));
}

void TAsyncJobs::Add(THolder<IJob> job, TInstant time) {
    Impl->Add(std::move(job), time);
}

void TAsyncJobs::WaitForCompletion() {
    Impl->WaitForCompletion();
}
