#pragma once

#include "mtp_jobs.h"
#include "work_stealing.h"

#include <util/generic/ptr.h>
#include <util/thread/pool.h>

class TMtpExecutor;

// IThreadPool with fixed number of executing threads (like TThreadPool).
// and ability to customize its internal job selection policy.
// Does not support Size(), blocking on Push(), forking.
class TFixedMtpQueue: public IThreadPool {
public:
    TFixedMtpQueue();
    TFixedMtpQueue(IMtpJobs* jobs);
    ~TFixedMtpQueue() override;

    void Start(size_t threadCount, TMtpJobsPtr customJobs);

    void Start(size_t threadCount, size_t queueSizeLimit = 0) override;
    void Stop() noexcept override;

    [[nodiscard]] bool Add(IObjectInQueue* obj) override;

    size_t Size() const noexcept override {
        return 0; // doesn't support this operation
    }

    TMtpJobsPtr JobQueue() const {
        return Jobs;
    }

private:
    TMtpJobsPtr Jobs;
    THolder<TMtpExecutor> Executor;
};

class TWorkStealingMtpQueue: public TFixedMtpQueue, public IMtpWorkStealing {
public:
    ~TWorkStealingMtpQueue() override;
    void Init(size_t queueSizeLimit);
    virtual void Start(size_t threadCount, size_t) override;
    virtual TMtpJobsPtr MinorJobQueue() override;

private:
    static TMtpJobsPtr CreateMajorJobs(size_t queueSizeLimit);

private:
    TIntrusivePtr<TPriorityMtpJobs> JobQueue;
};
