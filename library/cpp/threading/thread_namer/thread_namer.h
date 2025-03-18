#pragma once

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>
#include <util/thread/fwd.h>

#include <functional>

namespace NThreading {
    // Helper class to set consecutive names for multiple threads.
    //
    // Example:
    // \code
    // const size_t threadCount = 11;
    // const auto pool = CreateThreadPool(threadCount, threadCount);
    // const auto namer = TThreadNamer::Make("Worker");
    // for (size_t i = 0; i < threadCount; ++i) {
    //     Y_VERIFY(pool->AddFunc(namer->GetNamerJob()));
    // }
    // namer->Wait(); // must be called after all invocations of `GetNamerJob`.
    // \code
    //
    // In this example thread names will be "Worker00", "Worker01", "Worker02", ..., "Worker10";
    //
    // NOTE: Each job **must** be scheduled to a thread and should not be blocked in a queue, otherwise
    // it will introduce deadlock. E.g. in case of thread pool make sure that number threads matches
    // number of `GetNamerJob` invocations.
    //
    class TThreadNamer : public TAtomicRefCount<TThreadNamer> {
    public:
        static TIntrusivePtr<TThreadNamer> Make(TString name);

        std::function<void()> GetNamerJob() noexcept;

        Y_WARN_UNUSED_RESULT bool WaitUntil(TInstant deadline) noexcept;
        Y_WARN_UNUSED_RESULT bool WaitFor(TDuration timeout) noexcept;
        void Wait() noexcept;

    public:
        TThreadNamer() = delete;
        TThreadNamer(const TThreadNamer&) = delete;
        TThreadNamer(TThreadNamer&&) = delete;
        TThreadNamer& operator=(const TThreadNamer&) = delete;
        TThreadNamer& operator=(TThreadNamer&&) = delete;

    private:
        explicit TThreadNamer(TString name);
        void SetName() noexcept;

    private:
        TString Name_;
        TAtomic Count_ = 0;

        TAtomic ThreadIndex_ = 0;
        TAtomicBase SetNamesCount_ = 0;

        TMutex StartLock_;
        TCondVar StartCondVar_;
        bool Started_ = false;

        TMutex CompletionLock_;
        TCondVar CompletionCondVar_;
    };

    // Convenience function, to name threads in util thread pool.
    //
    void SetThreadNames(const TString& name, size_t count, IThreadPool* pool);
}
