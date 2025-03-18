#include "blocking_counter.h"

#include <util/system/guard.h>
#include <util/system/yassert.h>

NThreading::TBlockingCounter::TBlockingCounter(const i64 count) noexcept
    : Count_(count)
{
    Y_VERIFY(Count_ >= 0, "Count_ = %" PRIi64, Count_);
}

i64 NThreading::TBlockingCounter::Decrement() noexcept {
    i64 count = 0;
    with_lock (Lock_) {
        count = --Count_;
    }

    if (count == 0) {
        CondVar_.Signal();
    }

    return count == 0;
}

bool NThreading::TBlockingCounter::WaitUntil(const TInstant deadline) noexcept {
    with_lock (Lock_) {
        return CondVar_.WaitD(Lock_, deadline, [&] { return Count_ == 0; });
    }
}

bool NThreading::TBlockingCounter::WaitFor(const TDuration timeout) noexcept {
    return WaitUntil(timeout.ToDeadLine());
}

void NThreading::TBlockingCounter::Wait() noexcept {
    (void)WaitUntil(TInstant::Max());
}
