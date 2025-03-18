#include "preemptive_mtp_queue.h"

#include <library/cpp/deprecated/atomic/atomic.h>

#include <util/generic/fastqueue.h>
#include <util/generic/map.h>
#include <util/stream/str.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>

// some CopyPaste from util/thread/factory.h
class TPreemptiveMtpQueue::TImpl {
        typedef IThreadPool::TTsr TTsr;
        typedef TFastQueue<IObjectInQueue*> TJobQueue;
        class TThreadWrapper;
        typedef TSimpleSharedPtr<TThreadWrapper> TThreadRef;

    public:
        inline TImpl(TPreemptiveMtpQueue* parent, size_t thrnum, size_t thrmax, size_t maxqueue)
            : Parent_(parent)
            , ShouldTerminate(1)
            , MaxQueueSize(0)
            , MaxThreadCount(thrmax)
            , MaxThreadWait(TDuration::MicroSeconds(1 << 20))
            , ThreadCountExpected(0)
            , ThreadCountReal(0)
            , FreeThreadCount(0)
        {
            Start(thrnum, maxqueue);
        }

        inline ~TImpl() {
            try {
                Stop();
            } catch (...) {
            }

            Y_ASSERT(Threads.empty());
        }

        inline bool Add(IObjectInQueue* obj) {
            if (AtomicGet(ShouldTerminate)) {
                return false;
            }

            if (Threads.empty()) {
                TTsr tsr(Parent_);
                obj->Process(tsr);

                return true;
            }

            {
                TGuard<TMutex> g(QueueMutex);

                if (MaxQueueSize > 0 && Queue.Size() >= MaxQueueSize) {
                    return false;
                }

                Queue.Push(obj);
            }

            QueueCond.Signal();

            MakeNewWorkersIfNeeded();

            return true;
        }

        inline size_t Size() const noexcept {
            TGuard<TMutex> g(QueueMutex);

            return Queue.Size();
        }

        void PrintStatistics(NAntiRobot::TStatsWriter& out);
        void PrintStatistics(TStatsOutput& out);
    private:
        template <typename F>
        void DoWriteStats(F write) {
            {
                TGuard<TMutex> g(ThreadsMutex);
                write("hlmq_threads_total", ThreadCountReal);
                write("hlmq_threads_free", FreeThreadCount);
                write("active_thread_count", ThreadCountReal - FreeThreadCount);
            }
            write("hlmq_queue_size", Size());
        }

        void MakeNewWorker();

        inline size_t MakeNewWorkers(size_t num) {
            for (size_t i = 0; i < num; ++i) {
                MakeNewWorker();
            }
            return num;
        }

        inline void Start(size_t num, size_t maxque) {
            if (maxque && num > MaxThreadCount)
                ythrow yexception() << "TPreemptiveMtpQueue: initial ThreadCount(" << num << ") must be less or equal then MaxThreadCount(" << MaxThreadCount << ")";
            AtomicSet(ShouldTerminate, 0);
            MaxQueueSize = maxque;
            ThreadCountExpected = num;
            ThreadCountReal = MakeNewWorkers(num);
            Y_ASSERT(ThreadCountReal == Threads.size());
        }

        inline void Stop() {
            AtomicSet(ShouldTerminate, 1);

            WaitForComplete();

            Threads.clear();
            ThreadCountExpected = 0;
            MaxQueueSize = 0;
        }

        inline void WaitForComplete() noexcept {
            TGuard<TMutex> g(ThreadsMutex);

            while (ThreadCountReal) {
                {
                    TGuard<TMutex> g2(QueueMutex);
                    QueueCond.Signal();
                }
                StopCond.Wait(ThreadsMutex);
            }
        }

        void DoExecute() {
            THolder<TTsr> tsr(new TTsr(Parent_));

            while (true) {
                IObjectInQueue* job = nullptr;

                {
                    TGuard<TMutex> g1(QueueMutex);

                    while (Queue.Empty() && !AtomicGet(ShouldTerminate)) {
                        FreeThreadCount++;
                        const TDuration delay = CalcWaitTime();
                        if (delay == TDuration::Zero()) {
                            QueueCond.WaitI(QueueMutex);
                        } else {
                            if (!QueueCond.WaitT(QueueMutex, delay)) {
                                TGuard<TMutex> g2(ThreadsMutex);
                                if (ThreadCountReal > ThreadCountExpected) {
                                    FreeThreadCount--;
                                    return; // suicide
                                }
                            }
                        }
                        FreeThreadCount--;
                    }

                    if (AtomicGet(ShouldTerminate) && Queue.Empty()) {
                        tsr.Destroy();

                        break;
                    }

                    job = Queue.Pop();
                }

                try { // nested try for suppressing possible exception in Cdbg <<
                    try {
                        job->Process(*tsr);
                    } catch (...) {
                        Cdbg << "[mtp queue] " << CurrentExceptionMessage() << Endl;
                    }
                } catch (...) {
                }
            }
        }

        inline void FinishOneThread(TThreadWrapper* tw) noexcept {
            TGuard<TMutex> g(ThreadsMutex);

            const size_t erased = Threads.erase(tw);
            Y_ASSERT(erased == 1);
            ThreadCountReal -= erased;
            StopCond.Signal();
        }

        size_t CalcWorkersDemand() const {
            if (MaxQueueSize) {
                if (Queue.Size() <= MaxQueueSize / 4)
                    return ThreadCountExpected;
                if(Queue.Size() >= 3 * MaxQueueSize / 4)
                    return MaxThreadCount;
                const size_t result =  2 * (MaxThreadCount - ThreadCountExpected) * (Queue.Size() - MaxQueueSize / 4)  / MaxQueueSize + ThreadCountExpected;
                Y_ASSERT(ThreadCountExpected <= result && result <= MaxThreadCount);
                return result;
            } else {
                return ThreadCountExpected;
            }
        }

        size_t MakeNewWorkersIfNeeded() {
            TTryGuard<TMutex> g(ThreadsMutex);
            return g.WasAcquired() ? MakeNewWorkersIfNeededNoLock() : 0;
        }

        size_t MakeNewWorkersIfNeededNoLock() {
            const ssize_t diff = ssize_t(CalcWorkersDemand()) - ssize_t(ThreadCountReal);
            if (diff > 0) {
                ThreadCountReal += MakeNewWorkers(diff);
                Y_ASSERT(ThreadCountReal == Threads.size());
                return diff;
            } else {
                return 0;
            }
        }

        TDuration CalcWaitTime() const {
            if (FreeThreadCount <= ThreadCountExpected)
                return TDuration::Zero();
            const TDuration result = MaxThreadWait / (FreeThreadCount - ThreadCountExpected);
            return result;
        }
    private:
        TPreemptiveMtpQueue* Parent_;
        mutable TMutex QueueMutex;
        mutable TMutex ThreadsMutex;
        TCondVar QueueCond;
        TCondVar StopCond;
        TJobQueue Queue;
        TMap<TThreadWrapper*, TThreadRef> Threads;
        TAtomic ShouldTerminate;
        size_t MaxQueueSize;
        size_t MaxThreadCount;
        TDuration MaxThreadWait;
        size_t ThreadCountExpected;
        size_t ThreadCountReal;
        size_t FreeThreadCount;
};

// wrapper class for assigning class IThreadFactory::IThread and class TPreemptiveMtpQueue::TImpl.
class TPreemptiveMtpQueue::TImpl::TThreadWrapper: public IThreadFactory::IThreadAble {
    typedef TSimpleSharedPtr<IThreadFactory::IThread> TIThreadRef;
public:
    TThreadWrapper(TPreemptiveMtpQueue::TImpl* parent)
    : Parent(parent)
    {
    }

    ~TThreadWrapper() override
    {
    }

    inline void Assign(const TIThreadRef& thread) {
        Thread.Reset(thread);
    }
private:
    void DoExecute() override {
        Parent->DoExecute();
        Parent->FinishOneThread(this);
    }
private:
    TPreemptiveMtpQueue::TImpl* Parent;
    TIThreadRef Thread;
};

void TPreemptiveMtpQueue::TImpl::MakeNewWorker() {
    TThreadRef wrapper(new TThreadWrapper(this));
    wrapper->Assign(Parent_->Pool()->Run(wrapper.Get()));
    Threads.insert(std::make_pair(wrapper.Get(), wrapper));
}

TPreemptiveMtpQueue::TPreemptiveMtpQueue(IThreadFactory* pool, size_t maxThreadCount)
    : TThreadFactoryHolder(pool)
    , MaxThreadCount(maxThreadCount)
{
}

TPreemptiveMtpQueue::~TPreemptiveMtpQueue() {
}

size_t TPreemptiveMtpQueue::Size() const noexcept {
    if (!Impl_.Get()) {
        return 0;
    }

    return Impl_->Size();
}

bool TPreemptiveMtpQueue::Add(IObjectInQueue* obj) {
    if (Y_UNLIKELY(!Impl_.Get())) {
        ythrow yexception() << "mtp queue not started";
    }

    return Impl_->Add(obj);
}

void TPreemptiveMtpQueue::Start(size_t thrnum, size_t maxque) {
    Impl_.Reset(new TImpl(this, thrnum, MaxThreadCount, maxque));
}

void TPreemptiveMtpQueue::Stop() noexcept {
    Impl_.Destroy();
}

void TPreemptiveMtpQueue::PrintStatistics(NAntiRobot::TStatsWriter& out) {
    Impl_->PrintStatistics(out);
}

void TPreemptiveMtpQueue::PrintStatistics(TStatsOutput& out) {
    Impl_->PrintStatistics(out);
}

void TPreemptiveMtpQueue::TImpl::PrintStatistics(NAntiRobot::TStatsWriter& out) {
    DoWriteStats([&out] (TStringBuf key, size_t value) {
        out.WriteHistogram(key, value);
    });
}

void TPreemptiveMtpQueue::TImpl::PrintStatistics(TStatsOutput& out) {
    DoWriteStats([&out] (const TString& key, size_t value) {
        out.AddHistogram(key, value);
    });
}
