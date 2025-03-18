#pragma once

#include <util/generic/ptr.h>
#include <util/system/mutex.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/condvar.h>
#include <util/thread/lfqueue.h>

struct IObjectInQueue;

class IMtpJobs: public TAtomicRefCount<IMtpJobs> {
private:
    virtual bool DoPush(IObjectInQueue* obj) = 0;
    virtual IObjectInQueue* DoPop() = 0;

    virtual void DoNotifyPush() {
    }

public:
    virtual ~IMtpJobs() {
    }

    // Pushes @obj into the queue (without taking ownership), returns true on success.
    // The caller should always check returned value and provide fallback
    // processing of @obj in case of Push() failure.
    bool Push(IObjectInQueue* obj);

    // Safe version: either @obj is pushed or an exception is thrown.
    void SafePush(IObjectInQueue* obj) {
        if (Y_UNLIKELY(!Push(obj)))
            ythrow yexception() << "IMtpJobs::SafePush() failed";
    }

    // Pops a job and returns to caller.
    // If the queue is empty, returns nullptr without blocking.
    // Note that Pop() is not thread-safe by default, synchronization should be
    // implemented explicitly in derived class if necessary.
    IObjectInQueue* Pop() {
        return DoPop();
    }

    // Blocking version of Pop(): if there is no job at the moment of calling
    // waits until the queue is either pushed or closed. Returned nullptr
    // guarantees that the queue has been closed.
    IObjectInQueue* WaitPop();

    // After closing, Push() does not accept any more jobs.
    // Note that any remaining jobs are not processed automatically,
    // you may use Finalize() to do this explicitly.
    void Close();

    bool Closed() const {
        return AtomicGet(Closed_);
    }

    // Close and process all remaining jobs in current thread
    void Finalize();

    // Forces IMtpJobs executors waiting on WaitPop()
    // to wake up and try pop again.
    void NotifyPush() {
        DoNotifyPush();
        Cond.Signal();
    }

private:
    TAtomic Closed_ = false;

    TCondVar Cond; // push or close
    TMutex Mutex;
};

using TMtpJobsPtr = TIntrusivePtr<IMtpJobs>;

// Several useful IMtpJobs implementations

// Just basic implementation without any special functionality
class TSimpleMtpJobs: public IMtpJobs {
protected:
    bool DoPush(IObjectInQueue* obj) override {
        Jobs.Enqueue(obj);
        return true;
    }

    IObjectInQueue* DoPop() override {
        IObjectInQueue* ret = nullptr;
        return Jobs.Dequeue(&ret) ? ret : nullptr;
    }

private:
    TLockFreeQueue<IObjectInQueue*> Jobs;
};

// IThreadPool with ability to limit maximal number of jobs in it.
class TLimitedMtpJobs: public TSimpleMtpJobs {
public:
    TLimitedMtpJobs(size_t limit)
        : Limit(limit)
    {
    }

    // this method could be useful for implementing IThreadPool::Size()
    size_t Size() const {
        return AtomicGet(Counter);
    }

private:
    bool DoPush(IObjectInQueue* obj) override;
    IObjectInQueue* DoPop() override;

private:
    const size_t Limit;
    TAtomic Counter = 0;
};

// IMtpJobs with a single follower IMtpJobs watching every Push() in this one.
template <typename TMtpJobsImpl>
class TFollowedMtpJobs: public TMtpJobsImpl {
public:
    template <typename... Args>
    TFollowedMtpJobs(IMtpJobs* follower, Args&&... args)
        : TMtpJobsImpl(std::forward<Args>(args)...)
        , Follower(follower)
    {
    }

    void DoNotifyPush() override {
        if (Follower)
            Follower->NotifyPush();
    }

private:
    IMtpJobs* Follower = nullptr;
};

// Very useful job queue: always empty
class TDummyMtpJobs: public IMtpJobs {
private:
    bool DoPush(IObjectInQueue*) override {
        return false;
    }

    IObjectInQueue* DoPop() override {
        return nullptr;
    }
};
