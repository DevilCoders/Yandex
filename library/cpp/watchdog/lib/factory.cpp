#include "factory.h"

#include <util/system/thread.h>

class TWatchDog: public IWatchDog {
public:
    TWatchDog(TWatchDogHandlePtr handle)
        : WatchDogHandle_(handle)
    {
    }

    ~TWatchDog() override {
        WatchDogHandle_->Cancel();
    }

private:
    TWatchDogHandlePtr WatchDogHandle_;
};

IWatchDog* TWatchDogFactory::RegisterWatchDogHandle(TWatchDogHandlePtr handle) {
    WatchDogQueues_[AtomicGet(CurrentQueue_)].Enqueue(handle);
    return new TWatchDog(handle);
}

void TWatchDogFactory::DoExecute() {
    TThread::SetCurrentThreadName("WatchDogFactory");
    while (true) {
        ui8 newQueue = AtomicXor(CurrentQueue_, 1);
        ui8 currentQueue = newQueue ^ 1;
        TWatchDogHandlePtr handlePtr;
        TInstant now(TInstant::Now());
        while (WatchDogQueues_[currentQueue].Dequeue(&handlePtr)) {
            if (!handlePtr->Canceled()) {
                handlePtr->Check(now);
                WatchDogQueues_[newQueue].Enqueue(handlePtr);
            }
        }

        bool signalled = ShutdownEvent.WaitT(TDuration::Seconds(5));
        if (signalled)
            return;
    }
}
