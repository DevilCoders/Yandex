#pragma once

#include "mtp_jobs.h"

class IMtpWorkStealing {
public:
    virtual ~IMtpWorkStealing() {
    }

    virtual TMtpJobsPtr MinorJobQueue() = 0;
};

// Work-stealing IMptJobs: holds an internal "major" (main) sub-queue
// which is pushed to by default. Also allows additional "minor" sub-queues
// which are popped from only if there is no jobs in the major sub-queue.

// Note that closing this queue does not automatically close all its minor sub-queues.
// So getting nullptr from WaitPop() does not guarantee this queue cannot hold
// any more jobs.
class TPriorityMtpJobs: public IMtpJobs, public IMtpWorkStealing {
public:
    TPriorityMtpJobs(TMtpJobsPtr majorJobs = nullptr);

    // Implements IMtpWorkStealing
    TMtpJobsPtr MinorJobQueue() override;

private:
    // Implements IMtpJobs
    bool DoPush(IObjectInQueue* obj) override;
    IObjectInQueue* DoPop() override;

    IObjectInQueue* PopMinor();

private:
    TMtpJobsPtr Major;
    TLockFreeQueue<TMtpJobsPtr> Minors;
    TAtomic MinorsCount = 0;
};
