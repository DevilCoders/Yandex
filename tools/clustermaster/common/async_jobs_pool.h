#pragma once

#include "async_jobs.h"

#include <util/system/mutex.h>

#include <map>
#include <memory>
#include <vector>

class TAsyncJobsPool {
private:
    TMutex Mutex;
    size_t MinPoolThreads = 8;
    std::map<size_t, std::vector<std::unique_ptr<TAsyncJobs>>> Pool;

private:
    class TAsyncJobsHolder {
    public:
        TAsyncJobsHolder(TAsyncJobsPool& pool, std::unique_ptr<TAsyncJobs> jobs, size_t threads);
        ~TAsyncJobsHolder() noexcept;
        void Add(THolder<TAsyncJobs::IJob> job);
        void Add(THolder<TAsyncJobs::IJob> job, TInstant time);
        void WaitForCompletion();

    private:
        std::unique_ptr<TAsyncJobs> Jobs;
        size_t Threads;
        TAsyncJobsPool* Pool;
    };

public:
    TAsyncJobsPool(size_t minPoolThreads = 8);
    ~TAsyncJobsPool() noexcept;
    TAsyncJobsHolder Get(size_t threads);

private:
    std::unique_ptr<TAsyncJobs> TryGetJobs(size_t& threads);
    void Release(std::unique_ptr<TAsyncJobs> jobs, size_t threads) noexcept;
};
