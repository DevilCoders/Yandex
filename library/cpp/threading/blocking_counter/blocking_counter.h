#pragma once

#include <util/datetime/base.h>
#include <util/system/compiler.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>
#include <util/system/types.h>

namespace NThreading {
    // TBlockingCounter
    //
    // Synchronization primitive to help split work among multiple worker threads. Blocking counter
    // maintains a single counter with initial value of `initialCount`. Main thread may call `Wait`
    // on the blocking counter to block until `count` events will occur, worker threads call
    // `Decrement` on the counter apon completion of their work, once counter internal count reaches
    // zero the main thread will unblock.
    //
    // Example:
    // \code
    // const auto pool = CreateThreadPool(...);
    // TBlockingCounter bc(5);
    // for (int i = 0; i < 5; ++i) {
    //     Y_ENSURE(pool->AddFunc([&] {
    //         Y_DEFER { bc.Decrement(); };
    //         DoWork();
    //     });
    //  }
    //
    // bc.Wait();
    // \code
    //
    class TBlockingCounter {
    public:
        explicit TBlockingCounter(i64 initialCount) noexcept;

        TBlockingCounter(const TBlockingCounter&) = delete;
        TBlockingCounter& operator=(const TBlockingCounter&) = delete;

        // Decrement internal counter by one and return it's value.
        //
        // Should be called at most `initialCount` times. Call to this function with internal
        // counter value being less or equal to zero will result in undefined behavior.
        //
        i64 Decrement() noexcept;

        // Blocks until internal counter values reaches zero. This functions must be called at most
        // once.
        //
        Y_WARN_UNUSED_RESULT bool WaitUntil(TInstant deadline) noexcept;
        Y_WARN_UNUSED_RESULT bool WaitFor(TDuration timeout) noexcept;
        void Wait() noexcept;

    private:
        TMutex Lock_;
        TCondVar CondVar_;
        i64 Count_ = 0;
    };
}
