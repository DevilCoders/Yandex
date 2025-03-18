#pragma once

#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/datetime/cputimer.h>
#include <functional>

struct IObjectInQueue;
class IThreadPool;

struct TAsyncTaskStats {
    TDuration TotalTime;
    TDuration SelfTime;
};

/**
 * `TAsyncTaskBatch` will try to add tasks to IThreadPool and wait them to complete or execute itself if queue is busy.
 *
 * Usage example:
 * \code
 * class TSomeJob: public TAsyncTaskBatch::ITask {
 *     ...
 * };
 *
 * void DoSomething() {
 *     ...
 * }
 *
 * TAsyncTaskBatch batch;
 * TSomeJob job;
 *
 * batch.Add(new TSomeJob(...));
 * batch.Add(DoSomething);
 *
 * batch.WaitAllAndProcessNotStarted();
 * \endcode
 */
class TAsyncTaskBatch {
public:
    class ITask
        : public TAtomicRefCount<ITask>
    {
    public:
        virtual ~ITask();
        virtual void Process() = 0;
    };
    using ITaskRef = TIntrusivePtr<ITask>;

    struct TWaitInfo {
        size_t TotalTasks = 0;
        size_t SelfProcessedTasks = 0;
        ui64 TotalCycleCount = 0;
        ui64 SelfCycleCount = 0;
        std::exception_ptr ExceptionPtr;

        TAsyncTaskStats GetStats() const {
            return {
                GetTotalDuration(),
                GetSelfDuration()
            };
        }

        TDuration GetTotalDuration() const {
            return CyclesToDuration(TotalCycleCount);
        }

        TDuration GetSelfDuration() const {
            return CyclesToDuration(SelfCycleCount);
        }
    };
public:
    explicit TAsyncTaskBatch(IThreadPool* queue = nullptr);
    virtual ~TAsyncTaskBatch();

    void Add(ITaskRef task);
    void Add(std::function<void()> task);

    virtual TWaitInfo WaitAllAndProcessNotStarted();
private:
    class TImpl;
    THolder<TImpl> Impl_;
};
