#include "async_task_batch.h"

#include <util/generic/queue.h>
#include <util/thread/pool.h>
#include <util/system/mutex.h>
#include <util/system/condvar.h>
#include <util/system/guard.h>
#include <library/cpp/deprecated/atomic/atomic.h>

TAsyncTaskBatch::ITask::~ITask() {}

class TAsyncTaskBatch::TImpl {
public:
    explicit TImpl(IThreadPool* queue)
        : Queue_(queue)
    {
    }

    ~TImpl() noexcept {
        WaitAllAndProcessNotStarted();
    }

    void OnFinished(ui64 cycleCount = 0, bool isSelf = true, std::exception_ptr exceptionPtr = {}) {
        with_lock (Mutex_) {
            TotalCycleCount_ += cycleCount;
            if (isSelf) {
                SelfCycleCount_ += cycleCount;
            }
            if (exceptionPtr) {
                ExceptionPtr_ = exceptionPtr;
            }
            if (0 == --NotFinished_) {
                CondVar_.Signal();
            }
        }
    }

    void Add(ITaskRef& task) {
        TTaskWrapperRef wrapper = new TTaskWrapper(task, *this);
        with_lock (Mutex_) {
            Tasks_.push(wrapper);

            NotFinished_++;
            CondVar_.Signal();
        }
        if (Queue_) {
            THolder<TTaskExecutor> executor = MakeHolder<TTaskExecutor>(wrapper);
            if (Queue_->Add(executor.Get())) {
                Y_UNUSED(executor.Release());
            }
        }
    }

    TWaitInfo WaitAllAndProcessNotStarted() {
        TWaitInfo info;

        while (true) {
            TTaskWrapperRef taskWrapper;
            with_lock (Mutex_) {
                if (Tasks_) {
                    taskWrapper = std::move(Tasks_.front());
                    Tasks_.pop();
                } else if (NotFinished_ > 0) {
                    CondVar_.Wait(Mutex_);
                    continue;
                } else {
                    break;
                }
            }
            info.TotalTasks++;
            info.SelfProcessedTasks += taskWrapper->TryProcess(true /* isSelf */);
        }
        info.TotalCycleCount = TotalCycleCount_;
        info.SelfCycleCount = SelfCycleCount_;
        info.ExceptionPtr = ExceptionPtr_;
        TotalCycleCount_ = 0;
        SelfCycleCount_ = 0;
        ExceptionPtr_ = {};
        return info;
    }

private:
    class TTaskWrapper
        : public TAtomicRefCount<TTaskWrapper>
    {
    public:
        TTaskWrapper(ITaskRef task, TAsyncTaskBatch::TImpl& impl)
            : Task_(task)
            , Impl_(impl)
        {
        }

        ~TTaskWrapper() {
            Y_VERIFY(AtomicGet(OnceGuard_) == 1);
        }

        bool TryProcess(bool isSelf) {
            if (AtomicCas(&OnceGuard_, 1, 0)) {
                std::exception_ptr exceptionPtr;
                TPrecisionTimer timer;
                try {
                    Task_->Process();
                } catch (...) {
                    Cdbg << CurrentExceptionMessage();
                    exceptionPtr = std::current_exception();
                }
                ui64 cycleCount = timer.GetCycleCount();

                Task_ = nullptr;
                Impl_.OnFinished(cycleCount, isSelf, exceptionPtr);

                return true;
            }

            return false;
        }
    private:
        ITaskRef Task_;
        TAsyncTaskBatch::TImpl& Impl_;
        TAtomic OnceGuard_ = 0;
    };

    using TTaskWrapperRef = TIntrusivePtr<TTaskWrapper>;

    class TTaskExecutor
        : public IObjectInQueue
    {
    public:
        explicit TTaskExecutor(TTaskWrapperRef& wrapper)
            : Wrapper_(wrapper)
        {
        }

        void Process(void*) final {
            THolder<TTaskExecutor> deleter(this);
            Wrapper_->TryProcess(false /* isSelf */);
        }
    private:
        TTaskWrapperRef Wrapper_;
    };
private:
    IThreadPool* Queue_;

    TMutex Mutex_;
    TCondVar CondVar_;

    size_t NotFinished_ = 0;
    ui64 TotalCycleCount_ = 0;
    ui64 SelfCycleCount_ = 0;
    std::exception_ptr ExceptionPtr_;

    TQueue<TTaskWrapperRef> Tasks_;
};

TAsyncTaskBatch::TAsyncTaskBatch(IThreadPool* queue)
    : Impl_(new TImpl(queue))
{
}

TAsyncTaskBatch::~TAsyncTaskBatch() {}

/// Add task to IThreadPool.
void TAsyncTaskBatch::Add(ITaskRef task) {
    Impl_->Add(task);
}

/// Add arbitrary void function to IThreadPool
void TAsyncTaskBatch::Add(std::function<void()> task) {
    class TBatchJob : public TAsyncTaskBatch::ITask {
    public:
        TBatchJob(std::function<void()> job)
            : Job(std::move(job))
        {
        }

        void Process() override {
            Job();
        }

    private:
        std::function<void()> Job;
    };

    Add(new TBatchJob(std::move(task)));
}

/// Wait for all tasks and process not started
TAsyncTaskBatch::TAsyncTaskBatch::TWaitInfo TAsyncTaskBatch::WaitAllAndProcessNotStarted() {
    return Impl_->WaitAllAndProcessNotStarted();
}
