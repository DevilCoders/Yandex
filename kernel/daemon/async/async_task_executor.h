#pragma once

#include <kernel/daemon/common/guarded_ptr.h>

#include <library/cpp/logger/global/global.h>
#include <library/cpp/json/json_value.h>

#include <util/generic/hash.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/system/rwlock.h>
#include <util/thread/pool.h>

class TAsyncTaskExecutor {
public:
    class TTask : public IObjectInQueue {
    public:
        enum TStatus {
            stsCreated  /* "CREATED" */,
            stsEnqueued /* "ENQUEUED" */,
            stsStarted  /* "STARTED" */,
            stsFinished /* "FINISHED" */,
            stsFailed   /* "FAILED" */,
            stsNotFound /* "NOT_FOUND" */,
        };

    private:
        TString Id;
        TStatus Status;

    protected:
        TRWMutex Mutex;
        NJson::TJsonValue Reply;

    public:
        typedef TAtomicSharedPtr<TTask> TPtr;

    public:
        TTask(const TString& id);

        virtual TString GetDescription() const;

        void FillInfo(NJson::TJsonValue& info) const;
        void SetStatus(TStatus status, const TString& message = TString());

        TGuardedPtr<NJson::TJsonValue, TRWMutex, TWriteGuard> GetReply() {
            return TGuardedPtr<NJson::TJsonValue, TRWMutex, TWriteGuard>(Reply, Mutex);
        }
    };

public:
    TAsyncTaskExecutor(ui32 threads)
        : Threads(threads)
    {
    }

    void AddTask(TTask::TPtr task, NJson::TJsonValue& result);
    const TTask::TPtr GetTask(const TString& taskId) const;

    void Start();
    void Stop();

protected:
    typedef THashMap<TString, typename TTask::TPtr> TTasksMap;

protected:
    const ui32 Threads;

    TTasksMap TasksMap;
    TThreadPool Tasks;
    TRWMutex RWMutex;
};
