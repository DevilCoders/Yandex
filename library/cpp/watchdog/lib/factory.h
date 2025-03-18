#pragma once

#include "handle.h"
#include "interface.h"

#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/event.h>
#include <util/thread/factory.h>
#include <util/thread/lfqueue.h>

class TWatchDogFactory: public IThreadFactory::IThreadAble {
public:
    TWatchDogFactory()
        : CurrentQueue_(0)
    {
        Thread_.Reset(SystemThreadFactory()->Run(this).Release());
    }

    ~TWatchDogFactory() override {
        ShutdownEvent.Signal();
        Thread_->Join();
    }

    IWatchDog* RegisterWatchDogHandle(TWatchDogHandlePtr handle) Y_WARN_UNUSED_RESULT;

    void DoExecute() override;

private:
    typedef TLockFreeQueue<TWatchDogHandlePtr> TWatchDogQueue;
    TWatchDogQueue WatchDogQueues_[2];
    TAtomic CurrentQueue_;
    THolder<IThreadFactory::IThread> Thread_;
    TManualEvent ShutdownEvent;
};
