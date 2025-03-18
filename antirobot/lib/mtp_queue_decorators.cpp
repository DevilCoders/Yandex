#include "mtp_queue_decorators.h"

#include <util/generic/ptr.h>

namespace NAntiRobot {

/* TMtpQueueDecorator */

TMtpQueueDecorator::TMtpQueueDecorator(TAtomicSharedPtr<IThreadPool> slave)
    : Slave(std::move(slave))
{

}

bool TMtpQueueDecorator::Add(IObjectInQueue* obj) {
    return Slave->Add(obj);
}

void TMtpQueueDecorator::Start(size_t threadCount, size_t queueSizeLimit) {
    Slave->Start(threadCount, queueSizeLimit);
}

void TMtpQueueDecorator::Stop() noexcept {
    Slave->Stop();
}

size_t TMtpQueueDecorator::Size() const noexcept {
    return Slave->Size();
}

/* TFailCountingQueue */

TFailCountingQueue::TFailCountingQueue(TAtomicSharedPtr<IThreadPool> slave)
    : TMtpQueueDecorator(std::move(slave))
{
}

bool TFailCountingQueue::Add(IObjectInQueue* obj) {
    if (TMtpQueueDecorator::Add(obj)) {
        return true;
    } else {
        FailCounter.Inc();
        return false;
    }
}

int TFailCountingQueue::GetFailedAdditionsCount() const {
    return FailCounter.Val();
}

/* TRpsCountingQueue */

struct TRpsCountingQueue::TImpl {
    TAdaptiveLock RpsStatLock;
    TRpsCountingQueue::TRpsStat RpsStat = {TInstant(), TInstant(), 0, 0};
};

struct TRpsCountingQueue::TTask : public IObjectInQueue {
    IObjectInQueue* Slave;
    TAtomicSharedPtr<TRpsCountingQueue::TImpl> RpsData;

    TTask(IObjectInQueue* slave, TAtomicSharedPtr<TRpsCountingQueue::TImpl> rpsData)
        : Slave(slave)
        , RpsData(std::move(rpsData))
    {
    }

    void Process(void* ThreadSpecificResource) override {
        THolder<TTask> self(this);
        const TInstant start = Now();

        Slave->Process(ThreadSpecificResource);

        const TInstant end = Now();
        with_lock(RpsData->RpsStatLock) {
            ++RpsData->RpsStat.Processed;
            RpsData->RpsStat.Time += end - start;
        }
    }
};

TRpsCountingQueue::TRpsCountingQueue(TAtomicSharedPtr<IThreadPool> slave)
    : TMtpQueueDecorator(std::move(slave))
    , Impl(new TImpl)
{
}

TRpsCountingQueue::~TRpsCountingQueue() {
}

bool TRpsCountingQueue::Add(IObjectInQueue* obj) {
    auto task = MakeHolder<TTask>(obj, Impl);

    bool wasAdded = TMtpQueueDecorator::Add(task.Get());
    if (wasAdded) {
        Y_UNUSED(task.Release());
    }
    return wasAdded;
}

TRpsCountingQueue::TRpsStat TRpsCountingQueue::GetRps() const {
    auto g = Guard(Impl->RpsStatLock);
    return Impl->RpsStat;
}

void TRpsCountingQueue::UpdatePrevData() {
    with_lock(Impl->RpsStatLock) {
        Impl->RpsStat.TimePrev = Impl->RpsStat.Time;
        Impl->RpsStat.ProcessedPrev = Impl->RpsStat.Processed;
    }
}

}
