#pragma once

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/thread/pool.h>

#include <functional>

class TAsyncJobs {
public:
    struct IJob: IObjectInQueue {};

    TAsyncJobs(size_t threads);
    ~TAsyncJobs() noexcept;

    void Start();
    void Stop() noexcept;

    void Add(THolder<IJob> job);
    void Add(THolder<IJob> job, TInstant time);

    void WaitForCompletion();

private:
    class TImpl;
    THolder<TImpl> Impl;
};

class TSomeJob final : public TAsyncJobs::IJob {
private:
    std::function<void()> Function;

public:
    TSomeJob(std::function<void()> function)
        : Function(std::move(function))
    {
    }

    void Process(void*) override {
        Function();
    }
};

/**
 * Two queues is quick workaround for problem with hanging sendmail processes (because
 * of - for example - some network problems). Hanging sendmail leads to all threads in queue
 * processing mail async and not scheduling cron tasks. Thus cron targets are not launched
 * and this is very bad.
 *
 * Then I decided to add third queue as another quick workaround. I need third queue to
 * process cron tasks that are already scheduled. Processing of cron entries could also be
 * long and thus could also prevent new cron tasks from scheduling.
 */
class TAsyncJobQueues {
public:
    TAsyncJobQueues()
        : Jobs(4)
        , MailJobs(1)
        , SmsJobs(1)
        , JugglerJobs(1)
        , TelegramJobs(1)
        , JNSChannelJobs(1)
        , CronProcessingJobs(1)
        , JobsPerSecond(1)
    {
    }

    TAsyncJobs Jobs;
    TAsyncJobs MailJobs;
    TAsyncJobs SmsJobs;
    TAsyncJobs JugglerJobs;
    TAsyncJobs TelegramJobs;
    TAsyncJobs JNSChannelJobs;
    TAsyncJobs CronProcessingJobs;
    TAsyncJobs JobsPerSecond;
};

inline TAsyncJobs& AsyncJobs() {
    return Singleton<TAsyncJobQueues>()->Jobs;
}

inline TAsyncJobs& AsyncMailJobs() {
    return Singleton<TAsyncJobQueues>()->MailJobs;
}

inline TAsyncJobs& AsyncSmsJobs() {
    return Singleton<TAsyncJobQueues>()->SmsJobs;
}

inline TAsyncJobs& AsyncJugglerJobs() {
    return Singleton<TAsyncJobQueues>()->JugglerJobs;
}

inline TAsyncJobs& AsyncTelegramJobs() {
    return Singleton<TAsyncJobQueues>()->TelegramJobs;
}

inline TAsyncJobs& AsyncJNSChannelJobs() {
    return Singleton<TAsyncJobQueues>()->JNSChannelJobs;
}

inline TAsyncJobs& AsyncCronProcessingJobs() {
    return Singleton<TAsyncJobQueues>()->CronProcessingJobs;
}

inline TAsyncJobs& AsyncJobsPerSecond() {
    return Singleton<TAsyncJobQueues>()->JobsPerSecond;
}
