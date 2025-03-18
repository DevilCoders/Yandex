#include "async_jobs_pool.h"

#include "log.h"

#include <util/generic/bitops.h>

TAsyncJobsPool::TAsyncJobsHolder::TAsyncJobsHolder(TAsyncJobsPool& pool, std::unique_ptr<TAsyncJobs> jobs, size_t threads)
    : Jobs(std::move(jobs))
    , Threads(threads)
    , Pool(&pool)
{
}

TAsyncJobsPool::TAsyncJobsHolder::~TAsyncJobsHolder() noexcept {
    Pool->Release(std::move(Jobs), Threads);
}

void TAsyncJobsPool::TAsyncJobsHolder::Add(THolder<TAsyncJobs::IJob> job) {
    Jobs->Add(std::move(job));
}

void TAsyncJobsPool::TAsyncJobsHolder::Add(THolder<TAsyncJobs::IJob> job, TInstant time) {
    Jobs->Add(std::move(job), time);
}

void TAsyncJobsPool::TAsyncJobsHolder::WaitForCompletion() {
    Jobs->WaitForCompletion();
}

TAsyncJobsPool::TAsyncJobsPool(size_t minPoolThreads)
    : Mutex()
    , MinPoolThreads(minPoolThreads > 0 ? FastClp2(minPoolThreads) : 1)
    , Pool()
{
}

TAsyncJobsPool::~TAsyncJobsPool() noexcept {
    for (auto& [_, v] : Pool) {
        for (auto& jobs : v) {
            jobs->Stop();
        }
    }
}

std::unique_ptr<TAsyncJobs> TAsyncJobsPool::TryGetJobs(size_t& threads) {
    TGuard guard(Mutex);
    auto it = Pool.lower_bound(threads);
    if (it == Pool.end()) {
        return {};
    }
    auto& v = it->second;
    threads = it->first;
    auto result = std::move(v.back());
    if (v.size() > 1) {
        v.pop_back();
    } else {
        Pool.erase(it);
    }
    return result;
}

TAsyncJobsPool::TAsyncJobsHolder TAsyncJobsPool::Get(size_t threads) {
    if (threads <= MinPoolThreads) {
        threads = MinPoolThreads;
    } else {
        threads = FastClp2(threads);
    }

    auto result = TryGetJobs(threads);
    if (result == nullptr) {
        result = std::make_unique<TAsyncJobs>(threads);
        result->Start();
        LOG("Async jobs pool: Created a new jobs scheduler with " << threads << " threads");
    } else {
        DEBUGLOG("Async jobs pool: Taken a jobs scheduler with " << threads << " threads from the pool");
    }
    return TAsyncJobsHolder(*this, std::move(result), threads);
}

void TAsyncJobsPool::Release(std::unique_ptr<TAsyncJobs> jobs, size_t threads) noexcept {
    try {
        jobs->WaitForCompletion();
        TGuard guard(Mutex);
        Pool[threads].push_back(std::move(jobs));
        DEBUGLOG("Async jobs pool: Returned a jobs scheduler with " << threads << " threads to the pool");
    } catch(std::exception const& e) {
        ERRORLOG("Async jobs pool: failed to release a jobs scheduler [" << e.what() << "]");
    }
}
