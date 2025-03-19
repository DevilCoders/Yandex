#include "task.h"

NJson::TJsonValue IDistributedTask::GetTaskInfo(const TCgiParameters* cgi /*= nullptr*/) const {
    NJson::TJsonValue result;
    NJson::TJsonValue dataJson;
    for (auto&& i : Data) {
        dataJson[i.first] = i.second->GetData() ? i.second->GetData()->GetDataInfo(cgi) : "EMPTY";
    }
    NJson::TJsonValue metaJson;
    metaJson.InsertValue("type", Type);
    metaJson.InsertValue("id", Identifier);
    result.InsertValue("meta", metaJson);
    result.InsertValue("data", dataJson);
    if (cgi && cgi->Has("dump", "eventlog")) {
        NJson::TJsonValue eventlog(NJson::JSON_ARRAY);
        for (auto&& i : Events) {
            NJson::TJsonValue item = i.SerializeToJson();
            eventlog.AppendValue(item);
        }
        result.InsertValue("eventlog", eventlog);
    }
    return result;
}

void IDistributedTask::SerializeMetaToProto(NFrontendProto::TTaskMeta& proto) const {
    proto.SetIdentifier(Identifier);
    proto.SetType(Type);
    proto.SetWriteEventLog(WriteEventLog);
    if (WriteEventLog) {
        *proto.MutableEventLog() = SerializeEventLogToProto();
    }
}

bool IDistributedTask::ParseMetaFromProto(const NFrontendProto::TTaskMeta& proto) {
    CHECK_WITH_LOG(Identifier == proto.GetIdentifier());
    CHECK_WITH_LOG(Type == proto.GetType());
    WriteEventLog = proto.GetWriteEventLog();
    if (WriteEventLog) {
        if (!DeserializeEventLogFromProto(proto.GetEventLog())) {
            return false;
        }
    }
    return true;
}

void IDistributedTask::OnTimeout(IDTasksQueue::TGuard::TPtr queueGuard, TMap<TString, IDDataStorage::TGuard::TPtr> data, ITaskExecutor* executor, TPtr self) noexcept {
    WARNING_LOG << "OnTimeout task_id: " << queueGuard->GetTaskIdentifier() << " OK" << Endl;
    QueueGuard = queueGuard;
    Data = data;
    Executor = executor;

    if (DoOnTimeout(self)) {
        Executor->RemoveTask(*QueueGuard);
    } else {
        WARNING_LOG << "Task " << queueGuard->GetTaskIdentifier() << " not removed on timeout" << Endl;
    }
}

void IDistributedTask::OnIncorrectData(IDTasksQueue::TGuard::TPtr queueGuard, TMap<TString, IDDataStorage::TGuard::TPtr> data, ITaskExecutor* executor, TPtr self) noexcept {
    WARNING_LOG << "OnIncorrectData task_id: " << queueGuard->GetTaskIdentifier() << " OK" << Endl;
    QueueGuard = queueGuard;
    Data = data;
    Executor = executor;

    if (DoOnIncorrectData(self)) {
        Executor->RemoveTask(*QueueGuard);
    } else {
        WARNING_LOG << "Task " << queueGuard->GetTaskIdentifier() << " not removed on incorrect data" << Endl;
    }
}

void IDistributedTask::Execute(IDTasksQueue::TGuard::TPtr queueGuard, TMap<TString, IDDataStorage::TGuard::TPtr> data, ITaskExecutor* executor, TPtr self) noexcept {
    DEBUG_LOG << "Execute task_id: " << queueGuard->GetTaskIdentifier() << " OK" << Endl;
    QueueGuard = queueGuard;
    Data = data;
    Executor = executor;

    if (DoExecute(self)) {
        Executor->RemoveTask(*QueueGuard);
    } else {
        WARNING_LOG << "Task " << queueGuard->GetTaskIdentifier() << " not removed after executing" << Endl;
    }
}
