#pragma once
#include <kernel/common_server/library/storage/abstract.h>
#include <util/system/mutex.h>

class TTaskData;
class TTaskGuard;

class IQueueActor {
public:
    using TPtr = TAtomicSharedPtr<IQueueActor>;
    virtual ~IQueueActor() = default;
    virtual void RemoveTask(const TString& taskId) = 0;
    virtual NRTProc::TAbstractLock::TPtr WriteLock(const TString& key, const TDuration timeout) const = 0;
    virtual NRTProc::TAbstractLock::TPtr FakeLock(const TString& key) const final {
        return MakeAtomicShared<NRTProc::TFakeLock>(key);
    }

    virtual NRTProc::TAbstractLock::TPtr ReadLock(const TString& key, const TDuration timeout) const = 0;
};

class IDistributedQueue: public IQueueActor {
private:
    const TString Name;
    TMutex MutexLocalTasks;
    TMap<TString, TTaskData> LocalTasks;
protected:
    virtual bool SaveTask(const TTaskData& data, const bool rewrite) const = 0;

    virtual TVector<TString> GetAllTasksImpl() const = 0;
    virtual void RemoveTaskImpl(const TString& taskId) = 0;
    virtual ui32 GetSizeImpl() const = 0;
public:
    virtual TAtomicSharedPtr<TTaskData> LoadTask(const TString& taskId) const = 0;

    IDistributedQueue(const TString& name)
        : Name(name)
    {

    }

    virtual TString GetName() const {
        return Name;
    }

    using TPtr = TAtomicSharedPtr<IDistributedQueue>;

    virtual TVector<TString> GetAllTasks() const final;

    virtual ui32 GetSize() const final;
    virtual void RemoveTask(const TString& taskId) override final;

    virtual TAtomicSharedPtr<TTaskGuard> GetTask(const TString& taskId);

    virtual TAtomicSharedPtr<TTaskGuard> PutTask(const TTaskData& data, const bool rewrite);
};
