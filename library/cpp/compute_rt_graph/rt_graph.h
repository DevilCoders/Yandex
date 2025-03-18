#pragma once

#include <library/cpp/threading/future/future.h>
#include <util/thread/pool.h>
#include <util/generic/ptr.h>

namespace NComputeRTGraph {
    using TFunc = std::function<void()>;
    class IComputable: public TMoveOnly {
    public:
        virtual void Do() = 0;
        virtual ~IComputable();
    };

    class ITask: public TThrRefBase {
    public:
        virtual bool IsFinished() const = 0;
        virtual bool HasException() const = 0;
        virtual NThreading::TFuture<void> GetFuture() const = 0;
        virtual void Wait() const = 0;
        virtual ~ITask() = default;
    };

    using ITaskPtr = TIntrusivePtr<ITask>;

    class TRTGraph {
    private:
        class TImpl;
        THolder<TImpl> Impl;
    public:
        explicit TRTGraph(ui32 threads = 0, IThreadPool* queue = nullptr);
        ~TRTGraph();
        ITaskPtr AddFunc(TFunc&& func, const TVector<ITaskPtr>& deps);
        ITaskPtr AddFunc(TFunc&& func);
        ITaskPtr Add(THolder<IComputable>&& computable, const TVector<ITaskPtr>& deps);
        ITaskPtr Add(THolder<IComputable>&& computable);
        void Stop();
    };
}
