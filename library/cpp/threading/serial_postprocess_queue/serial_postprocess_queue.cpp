#include "serial_postprocess_queue.h"

#include <util/generic/ptr.h>
#include <util/generic/cast.h>
#include <util/system/condvar.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>
#include <util/thread/pool.h>
#include <util/thread/factory.h>

namespace {
    using IProcessObject = TSerialPostProcessQueue::IProcessObject;

    // Work item that will be processed first in thread pool
    class TAsync: public ::IObjectInQueue {
    public:
        TAsync(IProcessObject* workItem)
            : WorkItem(workItem)
            , Processed(false)
        {
        }

        void Process(void* threadSpecificResource) override {
            WorkItem->ParallelProcess(threadSpecificResource);
            Done();
        }

        void Wait() {
            TGuard<TMutex> lock(ProcessedMutex);
            while (!Processed)
                ProcessedCond.Wait(ProcessedMutex);
        }

    private:
        void Done() {
            TGuard<TMutex> lock(ProcessedMutex);
            Processed = true;
            ProcessedCond.Signal();
        }

    private:
        IProcessObject* WorkItem;
        TCondVar ProcessedCond;
        TMutex ProcessedMutex;
        bool Processed;
    };

    // Work item that will be processed second in one serial thread
    class TSync: public ::IObjectInQueue {
    public:
        TSync(TAutoPtr<IProcessObject> item)
            : Async(item.Get())
            , WorkItem(item)
        {
        }

        void Process(void*) override {
            THolder<TSync> holder(this);
            Async.Wait();
            WorkItem->SerialProcess();
        }

        IObjectInQueue* GetAsync() {
            return &Async;
        }

    private:
        TAsync Async;
        THolder<IProcessObject> WorkItem;
    };

}

TSerialPostProcessQueue::TSerialPostProcessQueue(IThreadPool* parallelQueue, bool blocking)
    : ParallelQueue(parallelQueue)
    , SerialQueue(new TThreadPool(TThreadPool::TParams().SetBlocking(blocking).SetCatching(true)))
{
}

TSerialPostProcessQueue::~TSerialPostProcessQueue() {
    Stop();
}

void TSerialPostProcessQueue::SafeAdd(TAutoPtr<IProcessObject> obj) {
    if (!Add(obj))
        ythrow yexception() << "can not add object to queue";
}

bool TSerialPostProcessQueue::Add(TAutoPtr<IProcessObject> obj) {
    THolder<TSync> objectToAdd(new TSync(obj));
    if (!SerialQueue->Add(objectToAdd.Get()))
        return false;

    // Object will be deleted in SerialQueue in TSync::Process(),
    // so now we must call Release() to prevent double deletion
    IObjectInQueue* const task = objectToAdd.Release()->GetAsync();
    if (!ParallelQueue->Add(task))
        // Process item here
        task->Process(IThreadPool::TTsr(ParallelQueue));
    return true;
}

void TSerialPostProcessQueue::Start(size_t threadCount, size_t queueSizeLimit, size_t serialQueueSizeLimit) {
    ParallelQueue->Start(threadCount, queueSizeLimit);
    SerialQueue->Start(1, serialQueueSizeLimit);
}
// Wait for completion of all scheduled objects, and then exit
void TSerialPostProcessQueue::Stop() noexcept {
    ParallelQueue->Stop();
    SerialQueue->Stop();
}

// Number of tasks currently in queue
size_t TSerialPostProcessQueue::Size() const noexcept {
    return SerialQueue->Size();
}
