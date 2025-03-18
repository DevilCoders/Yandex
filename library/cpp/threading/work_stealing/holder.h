#pragma once

#include "mtp_jobs.h"
#include "waiter.h"

#include <util/generic/vector.h>

// Takes ownership of a collection of IObjectInQueue pointers.
class TMtpJobHolder {
public:
    TWaitedMtpJob* Hold(TAutoPtr<TWaitedMtpJob> job) {
        TWaitedMtpJob* ret = job.Get();
        Jobs.push_back(job);
        return ret;
    }

    size_t Size() const {
        return Jobs.size();
    }

    TWaitedMtpJob* operator[](size_t i) {
        return Jobs[i].Get();
    }

    const TWaitedMtpJob* operator[](size_t i) const {
        return Jobs[i].Get();
    }

    void Clear() {
        Jobs.clear();
    }

private:
    TVector<TAutoPtr<TWaitedMtpJob>> Jobs;
};
