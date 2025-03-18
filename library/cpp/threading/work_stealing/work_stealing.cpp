#include "work_stealing.h"

TPriorityMtpJobs::TPriorityMtpJobs(TMtpJobsPtr majorJobs)
    : Major(majorJobs)
{
    if (!Major)
        Major.Reset(new TSimpleMtpJobs);
}

TMtpJobsPtr TPriorityMtpJobs::MinorJobQueue() {
    using TMinorJobs = TFollowedMtpJobs<TSimpleMtpJobs>;

    TMtpJobsPtr ret(new TMinorJobs(this));
    Minors.Enqueue(ret);
    AtomicIncrement(MinorsCount);

    return ret;
}

bool TPriorityMtpJobs::DoPush(IObjectInQueue* obj) {
    return Major->Push(obj);
}

IObjectInQueue* TPriorityMtpJobs::DoPop() {
    if (IObjectInQueue* job = Major->Pop())
        return job;

    // only if no major jobs, check minors
    for (size_t i = 0, count = AtomicGet(MinorsCount); i < count; ++i)
        if (IObjectInQueue* job = PopMinor())
            return job;

    return nullptr;
}

IObjectInQueue* TPriorityMtpJobs::PopMinor() {
    TMtpJobsPtr minor;
    if (Minors.Dequeue(&minor)) {
        bool sharedBeforePop = minor.RefCount() > 1;

        if (IObjectInQueue* job = minor->Pop()) {
            Minors.Enqueue(minor);
            return job;
        }

        if (sharedBeforePop)
            Minors.Enqueue(minor); // can be pushed later, restore
        else
            AtomicDecrement(MinorsCount); // cannot be pushed, remove completely
    }

    return nullptr;
}
