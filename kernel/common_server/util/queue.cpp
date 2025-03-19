#include "queue.h"

class TCountedMtpQueue::TObjectInQueue: public IObjectInQueue {
public:
    TObjectInQueue(TCountedMtpQueue* parent, IObjectInQueue* obj)
        : Parent(parent)
        , Object(obj)
    {
    }

    void Process(void* threadSpecificResource) override {
        Y_ASSERT(Parent);
        auto busy = Guard(Parent->BusyThreadsCount);
        if (Object) {
            Object->Process(threadSpecificResource);
        }
    }

private:
    TCountedMtpQueue* Parent;
    IObjectInQueue* Object;
};

TCountedMtpQueue::TCountedMtpQueue()
    : Slave(MakeHolder<TThreadPool>())
    , ThreadsCount(0)
{
}

TCountedMtpQueue::TCountedMtpQueue(THolder<IThreadPool>&& slave)
    : Slave(std::move(slave))
    , ThreadsCount(0)
{
}

bool TCountedMtpQueue::Add(IObjectInQueue* obj) {
    if (!Slave) {
        return false;
    }
    return Slave->AddAndOwn(MakeHolder<TObjectInQueue>(this, obj));
}

void TCountedMtpQueue::Start(size_t threadCount, size_t queueSizeLimit /*= 0*/) {
    if (Slave) {
        Slave->Start(threadCount, queueSizeLimit);
        ThreadsCount = threadCount;
    }
}

void TCountedMtpQueue::Stop() noexcept {
    if (Slave) {
        Slave->Stop();
        ThreadsCount = 0;
    }
}

size_t TCountedMtpQueue::Size() const noexcept {
    return Slave ? Slave->Size() : 0;
}

ui64 TCountedMtpQueue::GetBusyThreadsCount() const {
    return BusyThreadsCount.Val();
}

ui64 TCountedMtpQueue::GetFreeThreadsCount() const {
    ui64 busy = GetBusyThreadsCount();
    ui64 count = ThreadsCount;
    if (count) {
        Y_ASSERT(busy <= count);
        return count - busy;
    } else {
        return 0;
    }
}

TOptionalRTYMtpQueue::TOptionalRTYMtpQueue(bool smart) {
    if (smart) {
        Slave = MakeHolder<TThreadPoolBinder<TRTYMtpQueue, TOptionalRTYMtpQueue>>(this);
    } else {
        Slave = MakeHolder<TThreadPoolBinder<TSimpleThreadPool, TOptionalRTYMtpQueue>>(this);
    }
}

bool TOptionalRTYMtpQueue::Add(IObjectInQueue* obj) {
    return Slave->Add(obj);
}

void TOptionalRTYMtpQueue::Start(size_t threadCount, size_t queueSizeLimit /*= 0*/) {
    Slave->Start(threadCount, queueSizeLimit);
}

void TOptionalRTYMtpQueue::Stop() noexcept {
    Slave->Stop();
}

size_t TOptionalRTYMtpQueue::Size() const noexcept {
    return Slave->Size();
}
