#include "rt_graph.h"

#include <library/cpp/threading/atomic/bool.h>
#include <util/generic/deque.h>
#include <util/generic/hash_set.h>
#include <util/system/spinlock.h>
#include <util/thread/pool.h>
#include <exception>

namespace NComputeRTGraph {
    IComputable::~IComputable() {
    }

    class TComputableFunc: public IComputable {
    public:
        TComputableFunc(std::exception_ptr e)
            : Func([e]()
        {
                std::rethrow_exception(e);
            }) {
        }
        TComputableFunc(TFunc&& func)
            : Func(std::move(func))
        {
        }

        void Do() override {
            Func();
        }

        ~TComputableFunc() override {
        }

    private:
        TFunc Func;
    };

    class TRTGraph::TImpl {
    private:
        class TTask;
        using TTaskPtr = TIntrusivePtr<TTask>;

        class TTask: public ITask, public IObjectInQueue {
        public:
            explicit TTask(THolder<IComputable>&& computable, TAtomicBase deps, TImpl* graph);
            bool TryAddBackDep(TTaskPtr task);
            TAtomicBase DecreaseDeps();
            void FailTask(std::exception_ptr e);
            void FinishTask();
            void MakeException(std::exception_ptr e);

            bool IsFinished() const override;
            bool HasException() const override;
            NThreading::TFuture<void> GetFuture() const override;
            void Wait() const override;
            ~TTask() override = default;

            void Process(void*) override;

        private:
            TAdaptiveLock ComputableLock_;
            THolder<IComputable> Computable_;

            TAdaptiveLock TaskLock_;
            NAtomic::TBool Finished_;
            TAtomic NotReadyDeps_;
            TDeque<TTaskPtr> BackDeps_;
            TImpl* Graph_;
            NThreading::TPromise<void> Promise_;
        };

    private:
        struct TTaskOps {
            ui64 operator()(const TTaskPtr& ptr) const {
                return THash<TTask*>()(ptr.Get());
            }
            bool operator()(const TTaskPtr& l, const TTaskPtr& r) const {
                return l.Get() == r.Get();
            }
        };

        TAdaptiveLock Lock_;
        THashSet<TTaskPtr, TTaskOps, TTaskOps> Tasks_;
        TAutoPtr<IThreadPool> Queue_;

    public:
        TImpl(IThreadPool* queue);

        void DecreaseDeps(TTaskPtr task);
        void FailTask(TTaskPtr task, std::exception_ptr e);
        void StartTask(TTaskPtr task);
        ITaskPtr Add(THolder<IComputable>&& computable, const TVector<ITaskPtr>& deps);

        void Stop();
    };

    TRTGraph::TImpl::TImpl(IThreadPool* queue)
        : Lock_()
        , Tasks_()
        , Queue_(queue)
    {
    }

    void TRTGraph::TImpl::DecreaseDeps(TTaskPtr task) {
        if (task->DecreaseDeps() == 0) {
            StartTask(task);
        }
    }

    void TRTGraph::TImpl::StartTask(TTaskPtr task) {
        with_lock (Lock_) {
            // task Process will make Unref
            task->Ref();
            Queue_->SafeAdd(task.Get());
            Tasks_.erase(task);
        }
    }

    ITaskPtr TRTGraph::TImpl::Add(THolder<IComputable>&& computable, const TVector<ITaskPtr>& deps) {
        TAtomicBase s = deps.size();
        TTaskPtr task = MakeIntrusive<TTask>(std::move(computable), s, this);
        with_lock (Lock_) {
            Tasks_.insert(task);
        }
        if (s == 0) {
            StartTask(task);
            return task;
        }
        for (ITaskPtr depRaw : deps) {
            if (!depRaw) {
                DecreaseDeps(task);
                continue;
            }
            TTaskPtr dep = static_cast<TTask*>(depRaw.Get());
            if (!dep->TryAddBackDep(task)) {
                auto f = dep->GetFuture();
                f.Wait();
                try {
                    f.GetValue();
                } catch (...) {
                    task->MakeException(std::current_exception());
                }
                DecreaseDeps(task);
            }
        }
        return task;
    }

    void TRTGraph::TImpl::Stop() {
        for (;;) {
            with_lock (Lock_) {
                if (Tasks_.size() == 0) {
                    break;
                }
            }
        }
        Queue_->Stop();
    }

    TRTGraph::TImpl::TTask::TTask(THolder<IComputable>&& computable, TAtomicBase deps, TImpl* graph)
        : ComputableLock_()
        , Computable_(std::move(computable))
        , TaskLock_()
        , Finished_()
        , NotReadyDeps_(deps)
        , BackDeps_()
        , Graph_(graph)
        , Promise_(NThreading::NewPromise())
    {
    }

    void TRTGraph::TImpl::TTask::Process(void*) {
        try {
            // lock just in case
            with_lock (ComputableLock_) {
                Y_VERIFY(Computable_ != nullptr);
                Computable_->Do();
            }
            FinishTask();
        } catch (...) {
            FailTask(std::current_exception());
        }
        UnRef();
    }

    void TRTGraph::TImpl::TTask::FailTask(std::exception_ptr e) {
        with_lock (TaskLock_) {
            with_lock (ComputableLock_) {
                Computable_.Reset();
            }
            Finished_ = true;
            for (auto task : BackDeps_) {
                task->MakeException(e);
                Graph_->DecreaseDeps(task);
            }
            Promise_.SetException(e);
        }
    }

    void TRTGraph::TImpl::TTask::FinishTask() {
        with_lock (TaskLock_) {
            with_lock (ComputableLock_) {
                Computable_.Reset();
            }
            Finished_ = true;
            for (auto task : BackDeps_) {
                Graph_->DecreaseDeps(task);
            }
            Promise_.SetValue();
        }
    }

    bool TRTGraph::TImpl::TTask::IsFinished() const {
        return Finished_;
    }

    bool TRTGraph::TImpl::TTask::HasException() const {
        return Promise_.HasException();
    }

    bool TRTGraph::TImpl::TTask::TryAddBackDep(TTaskPtr task) {
        with_lock (TaskLock_) {
            if (!Finished_) {
                BackDeps_.push_back(task);
                return true;
            }
        }
        return false;
    }

    TAtomicBase TRTGraph::TImpl::TTask::DecreaseDeps() {
        return AtomicSub(NotReadyDeps_, 1);
    }

    void TRTGraph::TImpl::TTask::MakeException(std::exception_ptr e) {
        with_lock (ComputableLock_) {
            Computable_.Reset(new TComputableFunc(e));
        }
    }

    NThreading::TFuture<void> TRTGraph::TImpl::TTask::GetFuture() const {
        return Promise_.GetFuture();
    }

    void TRTGraph::TImpl::TTask::Wait() const {
        GetFuture().Wait();
    }

    TRTGraph::TRTGraph(ui32 threads, IThreadPool* queue)
        : Impl(new TImpl(queue != nullptr ? queue : CreateThreadPool(threads).Release()))
    {
    }

    ITaskPtr TRTGraph::AddFunc(TFunc&& func, const TVector<ITaskPtr>& deps) {
        return Add(MakeHolder<TComputableFunc>(std::move(func)), deps);
    }

    ITaskPtr TRTGraph::AddFunc(TFunc&& func) {
        return Add(MakeHolder<TComputableFunc>(std::move(func)), {});
    }

    ITaskPtr TRTGraph::Add(THolder<IComputable>&& computable, const TVector<ITaskPtr>& deps) {
        return Impl->Add(std::move(computable), deps);
    }

    ITaskPtr TRTGraph::Add(THolder<IComputable>&& computable) {
        return Add(std::move(computable), {});
    }

    void TRTGraph::Stop() {
        Impl->Stop();
    }

    TRTGraph::~TRTGraph() = default;
}
