#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/thread/pool.h>

// Queue that processes items in multithreaded queue (IThreadPool) and then postprocesses them serially in order they were added
class TSerialPostProcessQueue: public TNonCopyable {
public:
    struct IProcessObject {
        virtual ~IProcessObject() {
        }
        virtual void ParallelProcess(void* threadSpecificResource) = 0; // Process that can be executed in parallel threads
        virtual void SerialProcess() = 0;                               // Process that will be executed serially
    };

public:
    // If there is nonzero serialQueueSizeLimit and queue is full
    // blocking flag will block Add() function untill queue size will be OK.
    // Otherwise Add() will return false
    TSerialPostProcessQueue(IThreadPool* parallelQueue, bool blocking = false);

    ~TSerialPostProcessQueue();

    //
    // Methods are similar to IThreadPool
    //

    void SafeAdd(TAutoPtr<IProcessObject> obj);
    bool Add(TAutoPtr<IProcessObject> obj);
    void Start(size_t threadCount, size_t queueSizeLimit = 0, size_t serialQueueSizeLimit = 0);
    // Wait for completion of all scheduled objects, and then exit
    void Stop() noexcept;
    // Number of tasks currently in queue
    size_t Size() const noexcept;

private:
    IThreadPool* ParallelQueue;
    THolder<IThreadPool> SerialQueue;
};
