#pragma once

#include <kernel/common_server/library/executor/abstract/queue.h>
#include <kernel/common_server/library/executor/abstract/storage.h>
#include <kernel/common_server/library/executor/abstract/task.h>
#include <kernel/common_server/library/unistat/signals.h>

#include <library/cpp/messagebus/scheduler/scheduler.h>

#include <util/generic/object_counter.h>
#include <util/thread/pool.h>
#include <library/cpp/mediator/messenger.h>
#include <kernel/common_server/util/accessor.h>

class TTaskExecutorConfig {
    RTLINE_READONLY_ACCEPTOR(ClearingRegularity, TDuration, TDuration::Hours(2));
    RTLINE_READONLY_ACCEPTOR(ThreadsCountDequeue, ui32, 16);
    RTLINE_READONLY_ACCEPTOR(ThreadsCountExecute, ui32, 16);
    RTLINE_READONLY_ACCEPTOR(ThreadsCountView, ui32, 6);
    RTLINE_READONLY_ACCEPTOR(UnlockDataWaitingDuration, TDuration, TDuration::Seconds(1));
    RTLINE_READONLY_ACCEPTOR(WaitingDurationClearTasks, TDuration, TDuration::Minutes(15));
    RTLINE_READONLY_ACCEPTOR(WaitingDurationClearData, TDuration, TDuration::Minutes(15));

private:
    bool EventLogUsageFlag = false;
    IDistributedTasksQueueConfig::TPtr QueueConfig;
    IDTasksStorageConfig::TPtr StorageConfig;
    TString Name = "global";
public:

    bool EventLogUsage() const {
        return EventLogUsageFlag;
    }

    bool IsValid() const {
        return QueueConfig && StorageConfig;
    }

    TString GetName() const {
        return Name;
    }

    void SetQueueConfig(IDistributedTasksQueueConfig::TPtr queueConfig) {
        QueueConfig = queueConfig;
    }

    const IDistributedTasksQueueConfig& GetQueueConfig() const {
        return *QueueConfig;
    }

    const IDTasksStorageConfig& GetStorageConfig() const {
        return *StorageConfig;
    }

    bool Init(const TYandexConfig::Section* section) {
        if (!section)
            return false;
        ThreadsCountDequeue = section->GetDirectives().Value("ThreadsCountDequeue", ThreadsCountDequeue);
        ThreadsCountExecute = section->GetDirectives().Value("ThreadsCountExecute", ThreadsCountExecute);
        ThreadsCountView = section->GetDirectives().Value("ThreadsCountView", ThreadsCountView);
        EventLogUsageFlag = section->GetDirectives().Value("EventLogUsage", EventLogUsageFlag);
        Name = section->GetDirectives().Value("Name", Name);
        UnlockDataWaitingDuration = section->GetDirectives().Value("UnlockDataWaitingDuration", UnlockDataWaitingDuration);
        WaitingDurationClearTasks = section->GetDirectives().Value("WaitingDurationClearTasks", WaitingDurationClearTasks);
        WaitingDurationClearData = section->GetDirectives().Value("WaitingDurationClearData", WaitingDurationClearData);
        ClearingRegularity = section->GetDirectives().Value("ClearingRegularity", ClearingRegularity);
        auto child = section->GetAllChildren();
        {
            auto it = child.find("Queue");
            AssertCorrectConfig(it != child.end(), "No 'Queue' section in configuration");
            QueueConfig = IDistributedTasksQueueConfig::ConstructConfig(it->second);
        }

        {
            auto it = child.find("Storage");
            AssertCorrectConfig(it != child.end(), "No 'Storage' section in configuration");
            StorageConfig = IDTasksStorageConfig::ConstructConfig(it->second);
        }
        return true;
    }

    void ToString(IOutputStream& os) const {
        os << "ThreadsCountDequeue: " << ThreadsCountDequeue << Endl;
        os << "ThreadsCountExecute: " << ThreadsCountExecute << Endl;
        os << "ThreadsCountView: " << ThreadsCountView << Endl;
        os << "UnlockDataWaitingDuration: " << UnlockDataWaitingDuration << Endl;
        os << "EventLogUsage: " << EventLogUsageFlag << Endl;

        os << "WaitingDurationClearTasks: " << WaitingDurationClearTasks << Endl;
        os << "WaitingDurationClearData: " << WaitingDurationClearData << Endl;
        os << "ClearingRegularity: " << ClearingRegularity << Endl;

        os << "<Queue>" << Endl;
        QueueConfig->ToString(os);
        os << "</Queue>" << Endl;

        os << "<Storage>" << Endl;
        StorageConfig->ToString(os);
        os << "</Storage>" << Endl;

    }
};

class TNewTaskSignal: public TUnistatSignal<double> {
private:
    using TBase = TUnistatSignal<double>;
public:
    TNewTaskSignal(const TString& executorName)
        : TBase({ "frontend-executor-" + executorName + "-task-construct" }, false) {

    }
};

class TTasksCountSignal: public TUnistatSignal<double> {
private:
    using TBase = TUnistatSignal<double>;
public:
    TTasksCountSignal(const TString& executorName)
        : TBase({ "frontend-executor-" + executorName + "-tasks-count" }, true) {

    }
};

class TDataRestoreTimeSignal: public TUnistatSignal<double> {
private:
    using TBase = TUnistatSignal<double>;
public:
    TDataRestoreTimeSignal(const TString& executorName)
        : TBase({ "frontend-executor-" + executorName + "-time-restore" }, NRTProcHistogramSignals::IntervalsTasks) {

    }
};

class TDataRestoreFailSignal: public TUnistatSignal<double> {
private:
    using TBase = TUnistatSignal<double>;
public:
    TDataRestoreFailSignal(const TString& executorName)
        : TBase({ "frontend-executor-" + executorName + "-data-restore-fail" }, false) {

    }
};

class TDataLockFailSignal: public TUnistatSignal<double> {
private:
    using TBase = TUnistatSignal<double>;
public:
    TDataLockFailSignal(const TString& executorName)
        : TBase({ "frontend-executor-" + executorName + "-data-lock-fail" }, false) {

    }
};

class TTaskRestoreFailSignal: public TUnistatSignal<double> {
private:
    using TBase = TUnistatSignal<double>;
public:
    TTaskRestoreFailSignal(const TString& executorName)
        : TBase({ "frontend-executor-" + executorName + "-task-restore-fail" }, false) {

    }
};

class TTaskExecutor: private ITaskExecutor, public TNonCopyable, public IMessageProcessor {
    RTLINE_ACCEPTOR(TTaskExecutor, CleaningInterval, TDuration, TDuration::Minutes(5));
public:
    enum class EGetDataStatus {
        NotFound /* "not_found" */,
        Ok /* "ok" */,
        Timeouted /* "timeouted" */
    };

private:
    IDTasksQueue::TPtr TasksQueue;
    IDTasksStorage::TPtr TasksStorage;
    TThreadPool TasksWatcher;
    TThreadPool TasksExecutor;

    class TStoragesCleaner;
    TThreadPool StoragesCleaner;
    const TTaskExecutorConfig Config;
    IDistributedTaskContext* Context;
    TAtomic Activity = 0;
    TSystemEvent EventStop;
    mutable NBus::NPrivate::TScheduler Scheduler;

    TNewTaskSignal NewTaskSignal;
    TTasksCountSignal TasksCountSignal;
    TDataRestoreTimeSignal DataRestoreTimeSignal;
    TDataRestoreFailSignal DataRestoreFailSignal;
    TTaskRestoreFailSignal TaskRestoreFailSignal;
    TDataLockFailSignal DataLockFailSignal;
    TEnumSignal<EGetDataStatus, double> GetDataSignal;

    class TTaskItemExecutor: public IObjectInQueue, public TObjectCounter<TTaskItemExecutor> {
    private:
        TTaskExecutor& Owner;
        IDTasksQueue::TGuard::TPtr TaskQueueGuard;
    public:
        TTaskItemExecutor(TTaskExecutor& owner, IDTasksQueue::TGuard::TPtr taskInQueue)
            : Owner(owner)
            , TaskQueueGuard(taskInQueue) {
            Owner.NewTaskSignal.Signal(1);
            Owner.TasksCountSignal.Signal(TTaskItemExecutor::ObjectCount());
        }

        ~TTaskItemExecutor() {
        }

        virtual void Process(void* /*threadSpecificResource*/) override;
    };

    class TTaskSchedule: public NBus::NPrivate::IScheduleItem, public TObjectCounter<TTaskSchedule> {
    private:
        IDTasksQueue::TGuard::TPtr TaskQueueGuard;
        TTaskExecutor& Owner;
    public:

        TTaskSchedule(IDTasksQueue::TGuard::TPtr taskInQueue, TTaskExecutor& owner, const TInstant instantSchedule, const bool unlockData)
            : NBus::NPrivate::IScheduleItem(instantSchedule)
            , TaskQueueGuard(taskInQueue)
            , Owner(owner)
        {
            if (unlockData) {
                TaskQueueGuard->UnlockData();
            }
        }

        virtual void Do() override {
            DEBUG_LOG << "Task " << TaskQueueGuard->GetTaskIdentifier() << " taken from scheduler" << Endl;
            Owner.TasksExecutor.SafeAdd(new TTaskItemExecutor(Owner, TaskQueueGuard));
        }
    };

    class TTaskWatcher: public IObjectInQueue {
    private:
        TTaskExecutor& Owner;
    public:
        TTaskWatcher(TTaskExecutor& owner)
            : Owner(owner)
        {

        }

        virtual void Process(void* /*threadSpecificResource*/) override {
            while (Owner.IsActive()) {
                auto taskGuard = Owner.TasksQueue->Dequeue();
                if (!!taskGuard) {
                    INFO_LOG << "Task " << taskGuard->GetTaskIdentifier() << " taken" << Endl;
                    Owner.TasksExecutor.SafeAdd(new TTaskItemExecutor(Owner, taskGuard));
                }
            }
        }
    };

    virtual void RescheduleTask(const IDistributedTask* task, const TInstant scheduleInstant, bool unlockData) override {
        Scheduler.Schedule(new TTaskSchedule(task->GetTaskGuard(), *this, scheduleInstant, unlockData));
    }

    void RescheduleTask(IDTasksQueue::TGuard::TPtr guard, const TInstant scheduleInstant, bool unlockData = false) {
        Scheduler.Schedule(new TTaskSchedule(guard, *this, scheduleInstant, unlockData));
    }

public:

    using ITaskExecutor::EnqueueTask;

    TTaskExecutor(const TTaskExecutorConfig& config, IDistributedTaskContext* context);

    virtual void ClearOld() const override {
        TasksQueue->ClearOld(*TasksStorage, Activity, Config.GetWaitingDurationClearTasks());
        TasksStorage->ClearOld(Activity, Config.GetWaitingDurationClearData());
    }

    ~TTaskExecutor() {
        UnregisterGlobalMessageProcessor(this);
        CHECK_WITH_LOG(!IsActive());
        Scheduler.Stop();
    }

    virtual bool Process(IMessage* message) override;

    virtual TString Name() const override {
        return Config.GetName() + "_executor";
    }

    virtual IDistributedTaskContext* GetContext() const override {
        return Context;
    }

    virtual TThreadPool& GetExecutor() override {
        return TasksExecutor;
    }

    void WaitTasks(const bool withScheduled) {
        while (TasksExecutor.Size() || TTaskItemExecutor::ObjectCount() || (withScheduled && TTaskSchedule::ObjectCount())) {
            NanoSleep(1);
        }
    }

    NJson::TJsonValue GetDataInfo(const TString& identifier, const TCgiParameters* cgi = nullptr, TInstant deadline = TInstant::Max()) const {
        bool isTimeouted;
        THolder<IDistributedData> task(TasksStorage->RestoreDataUnsafe(identifier, deadline, &isTimeouted));
        if (!!task) {
            GetDataSignal.Signal(EGetDataStatus::Ok, 1);
            return task->GetDataInfo(cgi);
        } else {
            if (isTimeouted) {
                GetDataSignal.Signal(EGetDataStatus::Timeouted, 1);
                NJson::TJsonValue result;
                result.InsertValue("timeouted", true);
                NJson::TJsonValue data;
                data.InsertValue("ready", false);
                result.InsertValue("data", data);
                return result;
            }
            GetDataSignal.Signal(EGetDataStatus::NotFound, 1);
            return NJson::TJsonValue(NJson::JSON_NULL);
        }
    }

    THolder<IDistributedData> RestoreDataInfo(const TString& identifier) const {
        return THolder(TasksStorage->RestoreDataUnsafe(identifier));
    }

    NRTProc::TAbstractLock::TPtr LockData(const TString& identifier, TDuration timeout = TDuration::Zero()) const {
        return TasksStorage->LockData(identifier, timeout);
    }

    virtual bool StoreTask2(const IDistributedTask* task) const override;
    virtual void StoreLocalTask(IDistributedTask::TPtr task) const override;

    virtual void RemoveTask(const TTaskGuard& guard) const override {
        guard.RemoveTaskFromQueue();
        TasksStorage->RemoveTask(guard.GetTaskIdentifier());
    }

    virtual bool StoreData2(const IDistributedData* data) const override;

    virtual void RemoveData(const TString& identifier) const override {
        TasksStorage->RemoveData(identifier);
    }

    virtual bool EnqueueTask(const TString& taskId, const TVector<IDTasksQueue::TRestoreDataMeta>& data, const TVector<TString>& waitTasks, const TInstant startInstant = TInstant::Zero(), const TInstant deadline = TInstant::Max()) const override {
        IDTasksQueue::TTaskData taskData(taskId, waitTasks, data);
        taskData.SetIsLocal(TasksStorage->IsLocalTask(taskId)).SetDeadline(deadline);
        return TasksQueue->Enqueue(taskData.SetStartInstant(startInstant), false);
    }

    virtual bool IsActive() const override {
        return AtomicGet(Activity) == 1;
    }

    TVector<TString> GetQueueNodes() const {
        return TasksQueue->GetNodes();
    }

    ui32 GetQueueSize() const {
        return TasksQueue->GetSize();
    }

    void SetSelector(IQueueViewSelector::TPtr selector);
    void Start();
    void Stop();
};
