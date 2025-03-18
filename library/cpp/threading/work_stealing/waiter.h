#pragma once

#include "mtp_jobs.h"

#include <util/thread/pool.h>

#include <util/generic/refcount.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>
#include <util/system/condvar.h>

#include <exception>

class IWaitableObjectInQueue;

// Not used currently because TSimpleMtpJobs shows
// silghtly better performance on wizard tests
// TODO: try again
class TZeroWaiter {
public:
    void Inc() {
        Counter.Inc();
    }

    void Dec() {
        // Dec() should be called strictly after corresponding Inc()

        TGuard<TAtomicCounter> guard(DecLock);

        if (Counter.Dec() == 0) {
            with_lock (Mutex) {
                guard.Release();
                Cond.BroadCast();
            }
        }
    }

    // Stop incrementing and wait until decremented to zero.
    // Should be called after all Inc().
    void WaitZero() {
        with_lock (Mutex) {
            while (Counter.Val() || DecLock.Val())
                Cond.Wait(Mutex);
        }
    }

private:
    TMutex Mutex;
    TCondVar Cond;

    TAtomicCounter Counter;
    TAtomicCounter DecLock; // number of Dec() operations currently running
};

class TMtpJobWaiter {
public:
    void Register(IWaitableObjectInQueue* job) {
        Y_UNUSED(job);
        //Impl.Inc();

        ++Total_; // not supposed to run concurently
    }

    // Total number of registered jobs
    size_t Total() const {
        return Total_;
    }

    // Mark job as processed (usually after job->Process())
    void SetDone(IWaitableObjectInQueue* job);

    // Wait all registered jobs to be marked as processed
    void WaitAll() {
        //Impl.WaitZero();

        for (size_t i = 0; i < Total_; ++i)
            Done.WaitPop();
    }

private:
    //TZeroWaiter Impl;
    TSimpleMtpJobs Done;
    size_t Total_ = 0;
};

class IWaitableObjectInQueue: public IObjectInQueue {
public:
    IWaitableObjectInQueue(TMtpJobWaiter& waiter)
        : Waiter(&waiter)
    {
        Waiter->Register(this);
    }

    // should be called in the end of IObjectInQueue::Process(),
    // see TWaitedMtpJob below
    void OnWaitDone() {
        Waiter->SetDone(this);
    }

private:
    TMtpJobWaiter* Waiter;
};

inline void TMtpJobWaiter::SetDone(IWaitableObjectInQueue* job) {
    //Y_UNUSED(job);
    //Impl.Dec();

    Done.SafePush(job);
}

// Simple wrapper for arbitrary IObjectInQueue
class TWaitedMtpJob: public IWaitableObjectInQueue {
public:
    TWaitedMtpJob(IObjectInQueue* job, TMtpJobWaiter& waiter)
        : IWaitableObjectInQueue(waiter)
        , Job(job)
    {
    }

    void Process(void* threadSpecificResource) override {
        try {
            Job->Process(threadSpecificResource);
        } catch (...) {
            Exception = std::current_exception();
        }
        IWaitableObjectInQueue::OnWaitDone();
    }

    void CheckError() const {
        if (Exception) {
            std::rethrow_exception(Exception);
        }
    }

private:
    IObjectInQueue* Job;
    std::exception_ptr Exception;
};
