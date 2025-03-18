#pragma once

#include <util/datetime/base.h>
#include <util/system/spinlock.h>
#include <util/thread/pool.h>

namespace NAntiRobot {

class TMtpQueueDecorator : public IThreadPool {
public:
    TMtpQueueDecorator(TAtomicSharedPtr<IThreadPool> slave);
    [[nodiscard]] bool Add(IObjectInQueue* obj) override;
    void Start(size_t threadCount, size_t queueSizeLimit = 0) override;
    void Stop() noexcept override;
    size_t Size() const noexcept override;

private:
    TAtomicSharedPtr<IThreadPool> Slave;
};

class TFailCountingQueue : public TMtpQueueDecorator {
public:
    TFailCountingQueue(TAtomicSharedPtr<IThreadPool> slave);
    bool Add(IObjectInQueue* obj) override;

    int GetFailedAdditionsCount() const;

private:
    TAtomicCounter FailCounter;
};

class TRpsCountingQueue : public TMtpQueueDecorator {
public:
    struct TRpsStat {
        TInstant Time;
        TInstant TimePrev;
        size_t Processed;
        size_t ProcessedPrev;
    };

    TRpsCountingQueue(TAtomicSharedPtr<IThreadPool> slave);
    ~TRpsCountingQueue() override;

    [[nodiscard]] bool Add(IObjectInQueue* obj) override;

    TRpsStat GetRps() const;

    void UpdatePrevData();

private:
    struct TTask;
    struct TImpl;
    TAtomicSharedPtr<TImpl> Impl;
};



}
