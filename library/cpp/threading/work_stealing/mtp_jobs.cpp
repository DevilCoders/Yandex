#include "mtp_jobs.h"

#include <util/thread/pool.h>

bool IMtpJobs::Push(IObjectInQueue* obj) {
    with_lock (Mutex) {
        if (Closed() || !DoPush(obj))
            return false;
        NotifyPush();
    }

    return true;
}

IObjectInQueue* IMtpJobs::WaitPop() {
    with_lock (Mutex) {
        while (true) {
            if (IObjectInQueue* ret = Pop())
                return ret;

            if (Closed())
                break;

            Cond.Wait(Mutex);
        }
    }

    return Pop();
}

void IMtpJobs::Close() {
    if (!Closed()) {
        with_lock (Mutex) {
            AtomicSet(Closed_, true);
            Cond.BroadCast();
        }
    }
}

void IMtpJobs::Finalize() {
    Close();
    while (IObjectInQueue* job = Pop())
        job->Process(nullptr);
}

bool TLimitedMtpJobs::DoPush(IObjectInQueue* obj) {
    Y_ASSERT(AtomicGet(Counter) >= 0);
    if ((size_t)AtomicIncrement(Counter) <= Limit) {
        return TSimpleMtpJobs::DoPush(obj);
    } else {
        AtomicDecrement(Counter);
        return false;
    }
}

IObjectInQueue* TLimitedMtpJobs::DoPop() {
    IObjectInQueue* ret = TSimpleMtpJobs::DoPop();
    if (ret) {
        AtomicDecrement(Counter);
    }

    return ret;
}
