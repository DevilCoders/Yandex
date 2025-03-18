#include "thread_namer.h"

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/stream/format.h>
#include <util/string/builder.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <library/cpp/deprecated/atomic/atomic_ops.h>
#include <util/system/condvar.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>
#include <util/system/types.h>
#include <util/system/yassert.h>
#include <util/thread/pool.h>

#include <functional>

TIntrusivePtr<NThreading::TThreadNamer> NThreading::TThreadNamer::Make(TString name) {
    return new TThreadNamer(std::move(name));
}

NThreading::TThreadNamer::TThreadNamer(TString name)
    : Name_(std::move(name))
{
}

static size_t GetWidth(TAtomicBase value) noexcept {
    size_t width = 0;
    for (; value; value /= 10) {
        ++width;
    }
    return width ? width : 1;
}

void NThreading::TThreadNamer::SetName() noexcept {
    const auto threadIndex = AtomicGetAndIncrement(ThreadIndex_);

    with_lock (StartLock_) {
        StartCondVar_.Wait(StartLock_, [this] { return Started_; });
    }

    const auto count = AtomicGet(Count_);
    const auto width = GetWidth(count - 1); // zero-based enumeration
    const TString name = TStringBuilder() << Name_ << LeftPad(threadIndex, width, '0');
    TThread::SetCurrentThreadName(count > 1 ? name.c_str() : Name_.c_str());

    TAtomicBase setNamesCount = 0;
    with_lock (CompletionLock_) {
        setNamesCount = ++SetNamesCount_;
    }

    if (setNamesCount == count) {
        CompletionCondVar_.Signal();
    }
}

std::function<void()> NThreading::TThreadNamer::GetNamerJob() noexcept {
    AtomicIncrement(Count_);
    TIntrusivePtr<NThreading::TThreadNamer> namer(this);
    return [namer = std::move(namer)] { namer->SetName(); };
}

bool NThreading::TThreadNamer::WaitUntil(const TInstant deadline) noexcept {
    with_lock (StartLock_) {
        Started_ = true;
    }
    StartCondVar_.BroadCast();

    const auto count = AtomicGet(Count_);
    with_lock (CompletionLock_) {
        return CompletionCondVar_.WaitD(CompletionLock_, deadline, [&] {
            return SetNamesCount_ == count;
        });
    }
}

bool NThreading::TThreadNamer::WaitFor(const TDuration timeout) noexcept {
    return WaitUntil(timeout.ToDeadLine());
}

void NThreading::TThreadNamer::Wait() noexcept {
    (void)WaitUntil(TInstant::Max());
}

void NThreading::SetThreadNames(const TString& name, const size_t count, IThreadPool* const pool) {
    const auto namer = TThreadNamer::Make(name);
    for (size_t i = 0; i < count; ++i) {
        Y_VERIFY(
            pool->AddFunc(namer->GetNamerJob()),
            "failed at i=%" PRISZT " count=%" PRISZT ", name=%s",
            i, count, name.c_str());
    }

    namer->Wait();
}
