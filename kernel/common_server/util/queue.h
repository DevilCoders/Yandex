#pragma once

#include "smartqueue.h"

#include <util/generic/ptr.h>
#include <util/generic/refcount.h>

typedef NUtil::TSmartMtpQueue TRTYMtpQueue;

class TCountedMtpQueue : public IThreadPool {
public:
    TCountedMtpQueue();
    TCountedMtpQueue(THolder<IThreadPool>&& slave);

    Y_WARN_UNUSED_RESULT virtual bool Add(IObjectInQueue* obj) final;
    virtual void Start(size_t threadCount, size_t queueSizeLimit = 0) final;
    virtual void Stop() noexcept final;
    virtual size_t Size() const noexcept final;

    ui64 GetBusyThreadsCount() const;
    ui64 GetFreeThreadsCount() const;

private:
    class TObjectInQueue;

private:
    const THolder<IThreadPool> Slave;

    TAtomicCounter BusyThreadsCount;
    ui64 ThreadsCount;
};

class TOptionalRTYMtpQueue : public IThreadPool {
public:
    TOptionalRTYMtpQueue(bool smart);
    Y_WARN_UNUSED_RESULT virtual bool Add(IObjectInQueue* obj) override;
    virtual void Start(size_t threadCount, size_t queueSizeLimit = 0) override;
    virtual void Stop() noexcept override;
    virtual size_t Size() const noexcept override;
private:
    THolder<IThreadPool> Slave;
};

inline THolder<IThreadPool> CreateRTYQueue(size_t threads, size_t size = 0, const NUtil::TSmartMtpQueue::TOptions& options = Default<NUtil::TSmartMtpQueue::TOptions>()) {
    auto queue = MakeHolder<NUtil::TSmartMtpQueue>(options);
    queue->Start(threads, size);
    return queue;
}

template <class T>
inline THolder<IThreadPool> CreateRTYQueueBinder(T* bound, size_t threads, size_t size = 0, const NUtil::TSmartMtpQueue::TOptions& options = Default<NUtil::TSmartMtpQueue::TOptions>()) {
    typedef TThreadPoolBinder<NUtil::TSmartMtpQueue, T> TQueueType;
    auto queue = MakeHolder<TQueueType>(bound, options);
    queue->Start(threads, size);
    return queue;
}
