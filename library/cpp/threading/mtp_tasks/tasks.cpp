#include "tasks.h"

#include <util/thread/pool.h>
#include <util/system/yassert.h>
#include <util/system/datetime.h>
#include <util/stream/output.h>
#include <util/system/event.h>
#include <util/system/condvar.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

class TMtpTask::TImpl {
    class TProcessor: public IObjectInQueue, public TSimpleRefCount<TProcessor> {
    public:
        inline TProcessor()
            : Task_(nullptr)
            , Ctx_(nullptr)
        {
        }

        ~TProcessor() override {
        }

        inline void SetData(ITaskToMultiThreadProcessing* task, void* ctx) noexcept {
            Y_ASSERT(Task_ == nullptr);
            Y_ASSERT(Ctx_ == nullptr);

            Task_ = task;
            Ctx_ = ctx;
        }

        void Process(void* /*tsr*/) override {
            try {
                Task_->Process(Ctx_);
            } catch (...) {
                Cdbg << "[mtp task] " << CurrentExceptionMessage() << Endl;
            }

            Done();
        }

        inline void Wait() noexcept {
            with_lock (Mutex_) {
                while (Task_) {
                    CondVar_.Wait(Mutex_);
                }
            }
        }

    private:
        inline void Done() noexcept {
            with_lock (Mutex_) {
                Task_ = nullptr;
                Ctx_ = nullptr;

                CondVar_.Signal();
            }
        }

    private:
        ITaskToMultiThreadProcessing* Task_;
        void* Ctx_;
        TMutex Mutex_;
        TCondVar CondVar_;
    };

    using TProcessorRef = TIntrusivePtr<TProcessor>;
    using TProcessors = TVector<TProcessorRef>;

public:
    inline TImpl(IThreadPool* slave, size_t cnt)
        : Queue_(slave)
    {
        if (!Queue_) {
            MyQueue_.Reset(new TAdaptiveThreadPool());
            MyQueue_->Start(cnt);
            Queue_ = MyQueue_.Get();
        }

        Y_ASSERT(Queue_);
    }

    inline ~TImpl() {
        MyQueue_.Destroy();
    }

    inline void Process(ITaskToMultiThreadProcessing* task, void** ctx, size_t cnt) {
        Reserve(cnt);

        for (size_t i = 0; i < cnt; ++i) {
            Procs_[i]->SetData(task, ctx[i]);

            if ((/*last task - in current thread*/ i == cnt - 1) || !Queue_->Add(Procs_[i].Get())) {
                Procs_[i]->Process(nullptr);
            }
        }

        for (size_t i = 0; i < cnt; ++i) {
            Procs_[i]->Wait();
        }
    }

private:
    inline void Reserve(size_t cnt) {
        while (Procs_.size() < cnt) {
            Procs_.push_back(new TProcessor());
        }
    }

private:
    IThreadPool* Queue_;
    THolder<IThreadPool> MyQueue_;
    TProcessors Procs_;
};

TMtpTask::TMtpTask(IThreadPool* slave)
    : Queue_(slave)
{
}

TMtpTask::~TMtpTask() {
    Destroy();
}

void TMtpTask::Destroy() noexcept {
    Impl_.Destroy();
}

void TMtpTask::Process(ITaskToMultiThreadProcessing* task, void** ctx, size_t cnt) {
    if (cnt == 0) {
        return;
    }

    if (cnt == 1) {
        /*
         * megaoptimization
         */
        task->Process(*ctx);

        return;
    }

    if (!Impl_) {
        /*
         * megaoptimization, too
         */

        Impl_.Reset(new TImpl(Queue_, cnt));
    }

    Impl_->Process(task, ctx, cnt);
}
