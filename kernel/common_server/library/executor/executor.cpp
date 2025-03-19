#include "executor.h"
#include <kernel/common_server/library/executor/clean/task.h>
#include <kernel/daemon/messages.h>

TTaskExecutor::TTaskExecutor(const TTaskExecutorConfig& config, IDistributedTaskContext* context)
    : Config(config)
    , Context(context)
    , NewTaskSignal(config.GetName())
    , TasksCountSignal(config.GetName())
    , DataRestoreTimeSignal(config.GetName())
    , DataRestoreFailSignal(config.GetName())
    , TaskRestoreFailSignal(config.GetName())
    , DataLockFailSignal(config.GetName())
    , GetDataSignal({ "frontend-executor-" + config.GetName() + "-get_data" }, false)
{
    RegisterGlobalMessageProcessor(this);
    TasksQueue = Config.GetQueueConfig().Construct(Context);
    TasksStorage = Config.GetStorageConfig().Construct();
}

bool TTaskExecutor::Process(IMessage* message) {
    TCollectServerInfo* messageInfo = dynamic_cast<TCollectServerInfo*>(message);
    if (messageInfo) {
        messageInfo->Fields[Name()]["guards_count"] = TTaskGuard::ObjectCount();
        return true;
    }
    return false;
}

void TTaskExecutor::StoreLocalTask(IDistributedTask::TPtr task) const {
    if (Config.EventLogUsage() || task->GetWriteEventLog()) {
        task->AddEvent("put");
    }
    TasksStorage->StoreLocalTask(task);
}

bool TTaskExecutor::StoreTask2(const IDistributedTask* task) const {
    if (Config.EventLogUsage() || task->GetWriteEventLog()) {
        task->AddEvent("put");
    }
    return TasksStorage->StoreTask2(task);
}

bool TTaskExecutor::StoreData2(const IDistributedData* data) const {
    return TasksStorage->StoreData2(data);
}

void TTaskExecutor::SetSelector(IQueueViewSelector::TPtr selector) {
    TasksQueue->SetSelector(selector);
}

class TTaskExecutor::TStoragesCleaner: public IObjectInQueue {
private:
    TTaskExecutor* Executor;
public:

    TStoragesCleaner(TTaskExecutor* executor)
        : Executor(executor)
    {

    }

    void Process(void* /*threadSpecificResource*/) override {
        while (Executor->IsActive()) {
            {
                TBlob info;
                auto lock = Executor->TasksQueue->GetOriginalQueue()->WriteLock("deprecated-cleaner-check", TDuration::Seconds(10));
                IDDataStorage::EReadStatus infoStatus = Executor->TasksStorage->ReadInfo("deprecated-cleaner-instant", info);
                if (!!lock && infoStatus != IDDataStorage::EReadStatus::Timeout) {
                    TString str;
                    if (infoStatus == IDDataStorage::EReadStatus::OK) {
                        str = TString(info.AsCharPtr(), info.Size());
                    }
                    ui32 ts;
                    if (!TryFromString(str, ts) || TInstant::Seconds(ts) + Executor->Config.GetClearingRegularity() < Now()) {
                        Executor->ClearOld();
                        const TString startTs = ::ToString(Now().Seconds());
                        Executor->TasksStorage->StoreInfo2("deprecated-cleaner-instant", TBlob::FromString(startTs));
                    }
                }
            }
            Executor->EventStop.WaitT(Executor->GetCleaningInterval());
        }
    }
};

void TTaskExecutor::Start() {
    CHECK_WITH_LOG(!IsActive());
    AtomicSet(Activity, 1);
    TasksQueue->Start(Config.GetThreadsCountView());
    TasksExecutor.Start(Config.GetThreadsCountExecute());
    TasksWatcher.Start(Config.GetThreadsCountDequeue());
    for (ui32 i = 0; i < Config.GetThreadsCountDequeue(); ++i) {
        CHECK_WITH_LOG(TasksWatcher.AddAndOwn(MakeHolder<TTaskWatcher>(*this)));
    }
    StoragesCleaner.Start(1);
    StoragesCleaner.SafeAddAndOwn(MakeHolder<TStoragesCleaner>(this));
}

void TTaskExecutor::Stop() {
    CHECK_WITH_LOG(IsActive());
    AtomicSet(Activity, 0);
    EventStop.Signal();
    StoragesCleaner.Stop();
    TasksQueue->Stop();
    TasksWatcher.Stop();
    TasksExecutor.Stop();
}

void TTaskExecutor::TTaskItemExecutor::Process(void* /*threadSpecificResource*/) {
    TTaskExecutor& ownerLocal = Owner;
    {
        THolder<TTaskItemExecutor> this_(this);
        if (!!TaskQueueGuard) {
            DEBUG_LOG << "Execute task_id: " << TaskQueueGuard->GetTaskIdentifier() << " ..." << Endl;
            const TInstant takeInstant = Now();
            IDistributedTask::TPtr task = Owner.TasksStorage->RestoreTask(TaskQueueGuard->GetTaskIdentifier());
            if (!!task) {
                TMap<TString, IDTasksStorage::TGuard::TPtr> dataWithGuard;
                if (!TaskQueueGuard->LockData()) {
                    Owner.DataLockFailSignal.Signal(1);
                    if (TaskQueueGuard->GetData().IsExpired()) {
                        task->OnTimeout(TaskQueueGuard, dataWithGuard, &Owner, task);
                    } else {
                        Owner.RescheduleTask(TaskQueueGuard, Now() + Owner.Config.GetUnlockDataWaitingDuration());
                    }
                    return;
                }
                if (TaskQueueGuard->GetData().IsExpired()) {
                    task->OnTimeout(TaskQueueGuard, dataWithGuard, &Owner, task);
                } else {
                    for (auto&& i : TaskQueueGuard->GetData().GetRestoreData()) {
                        if (i.GetExistanceRequirement() == EExistanceRequirement::LockPool) {
                            continue;
                        }
                        auto dataGuard = Owner.TasksStorage->RestoreDataLinkUnsafe(i.GetIdentifier());
                        if (!i.IsExistsCompatible(dataGuard->GetData().Get())) {
                            WARNING_LOG << "Incorrect data_id: " << i.GetIdentifier() << " for task_id: " << TaskQueueGuard->GetTaskIdentifier() << Endl;
                            Owner.DataRestoreFailSignal.Signal(1);
                            task->OnIncorrectData(TaskQueueGuard, dataWithGuard, &Owner, task);
                            return;
                        } else {
                            dataWithGuard[i.GetFindTag()] = dataGuard;
                        }
                    }
                    const TDuration dTask = takeInstant - TaskQueueGuard->GetData().GetEnqueueInstant();
                    const TDuration dData = Now() - takeInstant;
                    Owner.DataRestoreTimeSignal.Signal(dData.MilliSeconds());

                    if (Owner.Config.EventLogUsage() || task->GetWriteEventLog()) {
                        task->AddEvent("get(" + dTask.ToString() + ", " + dData.ToString() + ")");
                    }

                    task->Execute(TaskQueueGuard, dataWithGuard, &Owner, task);
                }
            } else {
                WARNING_LOG << "Incorrect task_id: " << TaskQueueGuard->GetTaskIdentifier() << Endl;
                Owner.TaskRestoreFailSignal.Signal(1);
                Owner.RemoveTask(*TaskQueueGuard);
            }
        }
    }
    ownerLocal.TasksCountSignal.Signal(TTaskItemExecutor::ObjectCount());
}
