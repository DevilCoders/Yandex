#include "smartqueue.h"

#include <library/cpp/balloc/optional/operators.h>
#include <library/cpp/logger/global/global.h>

#include <util/generic/fastqueue.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>
#include <util/system/condvar.h>

namespace NUtil {

class TSmartMtpQueue::TImpl {
private:
    class TSmartThread : public IObjectInQueue {
    public:
        TSmartThread(TSmartMtpQueue::TImpl& parent, IObjectInQueue* firstTask, bool disableBalloc, bool fixed)
            : Parent(parent)
            , FirstTask(firstTask)
            , DisableBalloc(disableBalloc)
            , Fixed(fixed)
        {
        }

    private:
        void ProcessTask(IObjectInQueue* task, TThreadPool::TTsr& tsr) {
            try {
                task->Process(tsr);
            }
            catch (...) {
                Cerr << "[mtp queue] " << CurrentExceptionMessage() << Endl;
            }
            Parent.CompleteTask();
        }

        // IThreadFactory::IThreadAble
        void Process(void* /*tsp*/) override {
            if (DisableBalloc) {
                ThreadDisableBalloc();
            }
            IThreadPool::TTsr tsr(Parent.GetParent());

            if (FirstTask)
                ProcessTask(FirstTask, tsr);

            while (IObjectInQueue* task = Parent.GetNextTask(Fixed))
                ProcessTask(task, tsr);
        }

    private:
        TSmartMtpQueue::TImpl& Parent;
        IObjectInQueue* FirstTask;
        const bool DisableBalloc;
        const bool Fixed;
    };

public:
    TImpl(TSmartMtpQueue* parent, TDuration idleInterval, size_t minThreadCount, bool disableBalloc)
        : IsActive(false)
        , SlaveAdaptive(parent->Pool())
        , SlaveFixed(parent->Pool())
        , Parent(parent)
        , MinThreadCount(minThreadCount)
        , DisableBalloc(disableBalloc)
        , IdleInterval(idleInterval)
    {}

    ~TImpl() {
        Stop();
    }

    void Start(size_t maxThreadCount, size_t queueSizeLimit) {
        CHECK_WITH_LOG(maxThreadCount == 0 || maxThreadCount >= MinThreadCount);

        TGuard<TMutex> g(StartStopMutex);
        if (IsActive)
            return;
        IsActive = true;

        // check consistency
        CHECK_WITH_LOG(ThreadCount == 0);
        CHECK_WITH_LOG(FixedThreadCount == 0);
        CHECK_WITH_LOG(Queue.Empty());

        MaxThreadCount = maxThreadCount;
        QueueSizeLimit = queueSizeLimit;
        UnlimitedThreads = MaxThreadCount == 0;
        UnlimitedSlots = UnlimitedThreads || QueueSizeLimit == 0;

        if (!UnlimitedSlots)
            AvailableSlots = QueueSizeLimit + MaxThreadCount;

        SlaveAdaptive.Start(0);
        SlaveAdaptive.SetMaxIdleTime(IdleInterval);
        if (MinThreadCount)
            SlaveFixed.Start(MinThreadCount);
     }

    void Stop() {
        TGuard<TMutex> g(StartStopMutex);
        if (!IsActive)
            return;
        IsActive = false;

        SlaveAdaptive.Stop();
        SlaveFixed.Stop();
    }

    bool Add(IObjectInQueue* task) {
        if (!IsActive)
            return false;

        TGuard<TMutex> g(QueueMutex);

        if (!UnlimitedSlots && AvailableSlots == 0)
            return false;
        AvailableSlots--;

        VERIFY_WITH_LOG(task, "New task should not be null");
        if (!TryStartThreadUnsafe(task))
            Queue.Push(task);

        return true;
    }

    size_t GetThreadCount() {
        TGuard<TMutex> g(QueueMutex);
        return MinThreadCount + ThreadCount - FixedThreadCount;
    }

    size_t GetQueueSize() const {
        TGuard<TMutex> g(QueueMutex);
        return Queue.Size();
    }

    void SetMaxIdleTime(TDuration interval) {
        IdleInterval = interval;
        if (IsActive)
            SlaveAdaptive.SetMaxIdleTime(interval);
    }

    // Interface for TSmartThread
    TSmartMtpQueue* GetParent() {
        return Parent;
    }

    IObjectInQueue* GetNextTask(bool fixed) {
        TGuard<TMutex> g(QueueMutex);
        if (Queue.Empty()) {
            --ThreadCount;
            if (fixed)
                --FixedThreadCount;
            return nullptr;
        }
        return Queue.Pop();
    }

    void CompleteTask() {
        if (!UnlimitedSlots) {
            TGuard<TMutex> g(QueueMutex);
            ++AvailableSlots;
        }
    }

private:
    bool TryStartThreadUnsafe(IObjectInQueue* task) {
        if (!UnlimitedThreads &&  ThreadCount >= MaxThreadCount)
            return false;
        ++ThreadCount;
        if (FixedThreadCount >= MinThreadCount) {
            SlaveAdaptive.SafeAddAndOwn(MakeHolder<TSmartThread>(*this, task, DisableBalloc, false));
        } else {
            SlaveFixed.SafeAddAndOwn(MakeHolder<TSmartThread>(*this, task, DisableBalloc, true));
            ++FixedThreadCount;
        }
        return true;
    }

private:
    TMutex StartStopMutex;
    bool IsActive;

    bool UnlimitedSlots;
    bool UnlimitedThreads;

    ui64 AvailableSlots = 0;   // max threads count + max queue size

    TFastQueue<IObjectInQueue*> Queue;
    TCondVar QueueEvent;
    TMutex QueueMutex;

    TAdaptiveThreadPool SlaveAdaptive;
    TThreadPool SlaveFixed;

    TSmartMtpQueue* Parent;
    const size_t MinThreadCount;
    const bool DisableBalloc;
    size_t MaxThreadCount;
    size_t QueueSizeLimit;

    size_t ThreadCount = 0;
    size_t FixedThreadCount = 0;
    TDuration IdleInterval;
};

TSmartMtpQueue::TSmartMtpQueue(const TOptions& options)
    : Impl(MakeHolder<TImpl>(this, options.IdleInterval, options.MinThreadCount, options.DisableBalloc))
{
}

TSmartMtpQueue::TSmartMtpQueue(IThreadFactory* pool, const TOptions& options)
    : TThreadFactoryHolder(pool)
    , Impl(MakeHolder<TImpl>(this, options.IdleInterval, options.MinThreadCount, options.DisableBalloc))
{
}

TSmartMtpQueue::~TSmartMtpQueue() {
}

void TSmartMtpQueue::SetMaxIdleTime(TDuration interval) {
    CHECK_WITH_LOG(Impl);
    Impl->SetMaxIdleTime(interval);
}

void TSmartMtpQueue::Start(size_t maxThreadCount, size_t queueSizeLimit) {
    CHECK_WITH_LOG(Impl);
    Impl->Start(maxThreadCount, queueSizeLimit);
}

void TSmartMtpQueue::Stop() noexcept {
    CHECK_WITH_LOG(Impl);
    Impl->Stop();
}

size_t TSmartMtpQueue::Size() const noexcept {
    CHECK_WITH_LOG(Impl);
    return Impl->GetQueueSize();
}

bool TSmartMtpQueue::Add(IObjectInQueue* obj) {
    CHECK_WITH_LOG(Impl);
    return Impl->Add(obj);
}

size_t TSmartMtpQueue::ThreadCount() {
    CHECK_WITH_LOG(Impl);
    return Impl->GetThreadCount();
}

NUtil::TSmartMtpQueue::TOptions NUtil::TSmartMtpQueue::TOptions::NoBalloc(0, TDuration::Seconds(1), true);

} // namespace NUtil
