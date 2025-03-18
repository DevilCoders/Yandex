#pragma once

#include <util/system/mutex.h>
#include <util/generic/ptr.h>
#include <util/generic/intrlist.h>
#include <util/generic/vector.h>

class IThreadPool;

class ITaskToMultiThreadProcessing {
public:
    inline ITaskToMultiThreadProcessing() noexcept {
    }

    virtual ~ITaskToMultiThreadProcessing() {
    }

    virtual void Process(void* threadContext) = 0;
};

class TMtpTask {
public:
    TMtpTask(IThreadPool* slave = nullptr);
    ~TMtpTask();

    void Destroy() noexcept;
    void Process(ITaskToMultiThreadProcessing* task, void** taskContexts, size_t contextCount);

private:
    class TImpl;
    IThreadPool* Queue_;
    THolder<TImpl> Impl_;
};

class TMtpTasksPool {
public:
    class TMtpTaskFromPool: public TMtpTask, public TSimpleRefCount<TMtpTaskFromPool, TMtpTaskFromPool>, public TIntrusiveListItem<TMtpTaskFromPool> {
    public:
        inline TMtpTaskFromPool(TMtpTasksPool* parent)
            : TMtpTask(parent->Queue_)
            , Parent_(parent)
        {
        }

        static inline void Destroy(TMtpTaskFromPool* This) noexcept {
            This->DeActivate();
        }

    private:
        inline void DeActivate() noexcept {
            Parent_->Release(this);
        }

    private:
        TMtpTasksPool* Parent_;
    };

    using TMtpTaskFromPoolRef = TIntrusivePtr<TMtpTaskFromPool>;

    inline TMtpTasksPool(IThreadPool* slave = nullptr)
        : Queue_(slave)
    {
    }

    inline ~TMtpTasksPool() {
        while (!List_.Empty()) {
            delete List_.PopFront();
        }
    }

    inline TMtpTaskFromPoolRef Acquire() {
        return AcquireImpl();
    }

private:
    inline void Release(TMtpTaskFromPool* task) noexcept {
        with_lock (Lock_) {
            List_.PushFront(task);
        }
    }

    inline TMtpTaskFromPool* AcquireImpl() {
        with_lock (Lock_) {
            if (!List_.Empty()) {
                return List_.PopFront();
            }
        }

        return new TMtpTaskFromPool(this);
    }

private:
    IThreadPool* Queue_;
    TIntrusiveList<TMtpTaskFromPool> List_;
    TMutex Lock_;
};

using TMtpTaskFromPool = TMtpTasksPool::TMtpTaskFromPoolRef;

template <class T>
class TTaskToMultiThreadProcessing: public ITaskToMultiThreadProcessing {
public:
    inline TTaskToMultiThreadProcessing() noexcept {
    }

    ~TTaskToMultiThreadProcessing() override {
    }

    void Process(void* threadContext) override {
        ((T*)threadContext)->ProcessTask();
    }
};

template <typename T>
class TSimpleMtpTask {
public:
    inline TSimpleMtpTask(IThreadPool* queue)
        : Slave_(queue)
    {
    }

    inline ~TSimpleMtpTask() {
        Reset();
    }

    inline void Reset() {
        for (size_t i = 0; i < Tasks_.size(); ++i)
            delete Tasks_[i];
        Tasks_.clear();
    }

    template <typename TT>
    inline void Add(TT task) {
        AddAndOwn(new T(task));
    }

    inline void AddAndOwn(TAutoPtr<T> task) {
        Tasks_.push_back(task.Get());
        Y_UNUSED(task.Release());
    }

    inline void Process() {
        TTaskToMultiThreadProcessing<T> executor;
        Slave_.Process(&executor, (void**)Tasks_.data(), Tasks_.size());
        Reset();
    }

private:
    TMtpTask Slave_;
    TVector<T*> Tasks_;
};
