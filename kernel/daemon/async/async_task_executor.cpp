#include "async_task_executor.h"

void TAsyncTaskExecutor::TTask::SetStatus(TStatus status, const TString& message) {
    TWriteGuard g(Mutex);
    Status = status;
    if (!!message)
        Reply.InsertValue("message", message);
    Reply.InsertValue("task_status", ToString(Status));
    Reply.InsertValue("id", GetDescription());
}

TAsyncTaskExecutor::TTask::TTask(const TString& id)
    : Id(id)
    , Status(stsCreated)
{
}

void TAsyncTaskExecutor::TTask::FillInfo(NJson::TJsonValue& info) const {
    TReadGuard g(Mutex);
    info = Reply;
}

TString TAsyncTaskExecutor::TTask::GetDescription() const {
    return Id;
}

void TAsyncTaskExecutor::AddTask(TTask::TPtr task, NJson::TJsonValue& result) {
    TWriteGuard rg(RWMutex);
    TTasksMap::const_iterator i = TasksMap.find(task->GetDescription());
    if (i == TasksMap.end()) {
        TasksMap[task->GetDescription()] = task;
        task->SetStatus(TTask::stsEnqueued);
        CHECK_WITH_LOG(Tasks.Add(task.Get()));
        task->FillInfo(result);
    } else {
        i->second->FillInfo(result);
    }
}

const TAsyncTaskExecutor::TTask::TPtr TAsyncTaskExecutor::GetTask(const TString& taskId) const {
    TReadGuard rg(RWMutex);
    TTasksMap::const_iterator i = TasksMap.find(taskId);
    if (i == TasksMap.end())
        return nullptr;
    else
        return i->second;
}

void TAsyncTaskExecutor::Start() {
    Tasks.Start(Threads);
}

void TAsyncTaskExecutor::Stop() {
    Tasks.Stop();
    TasksMap.clear();
}
