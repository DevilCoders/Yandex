#pragma once

#include "mtp_jobs.h"
#include "holder.h"
#include "waiter.h"
#include "function.h"

#include <util/generic/ptr.h>
#include <util/generic/vector.h>

class TMtpMultiTask: private TMtpJobWaiter {
public:
    void AddJob(IObjectInQueue* job);

    // adds and takes ownership
    void TakeJob(TAutoPtr<IObjectInQueue> job) {
        Owned.push_back(job);
        AddJob(Owned.back().Get());
    }

    template <class TFunc>
    void AddFunc(TFunc&& f) {
        TakeJob(MakeFunctionMtpJob(std::forward<TFunc>(f)));
    }

    // Begins jobs processing in other threads using specified @processor queue, without blocking.
    // Jobs added after Start() begin their processing immediately.
    // @processor could be nullptr, in this case all jobs will be done in current thread.
    void Start(TMtpJobsPtr processor) {
        DoStart(processor, 1);
    }

    bool Started() const {
        return Processor.Get();
    }

    // Blocks caller until all jobs are processed
    void Wait();

    void Process(TMtpJobsPtr processor) {
        Start(processor);
        Wait();
    }

    // Process all jobs using standard util IThreadPool interface.
    // At least one job is guaranteed to be executed in current thread,
    // so this method is mostly the same as processing all jobs
    // using TMtpTask from library/cpp/threading/mtp_tasks.
    void Process(IThreadPool& processor);

    void ProcessLocally() {
        Process(nullptr); // fake mpt
    }

    // Process using current thread + additional @threads-1 temporarily started threads
    // @threads = 0 or 1 is same as ProcessLocally()
    void Process(size_t threads);

private:
    // @minLocal specifies minimal number of jobs to be processed
    // in current thread
    void DoStart(TMtpJobsPtr processor, size_t minLocal = 0);

    inline void Detach(IObjectInQueue* job);
    inline void Delay(IObjectInQueue* job);
    inline void ProcessInCurrentThread(IObjectInQueue* job);
    inline void ProcessDelayed();
    void CheckErrors() const;

private:
    TVector<TAutoPtr<IObjectInQueue>> Owned; // jobs taken with ownership
    TMtpJobHolder Jobs;                      // all jobs wrapped as TWaitedMtpJob

    TVector<IObjectInQueue*> Delayed; // delayed jobs are processed locally in current thread during Wait()
    size_t Done = 0;

    TMtpJobsPtr Processor;
};
