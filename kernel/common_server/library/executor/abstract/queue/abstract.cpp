#include "abstract.h"
#include "common.h"
#include <kernel/common_server/util/logging/tskv_log.h>
#include <util/string/type.h>

TVector<TString> IDistributedQueue::GetAllTasks() const {
    TVector<TString> result = GetAllTasksImpl();
    TGuard<TMutex> g(MutexLocalTasks);
    for (auto&& i : LocalTasks) {
        result.push_back(i.first);
    }
    return result;
}

ui32 IDistributedQueue::GetSize() const {
    TGuard<TMutex> g(MutexLocalTasks);
    return GetSizeImpl() + LocalTasks.size();
}

void IDistributedQueue::RemoveTask(const TString& taskId) {
    TGuard<TMutex> g(MutexLocalTasks);
    auto it = LocalTasks.find(taskId);
    if (it == LocalTasks.end()) {
        g.Release();
        RemoveTaskImpl(taskId);
    } else {
        LocalTasks.erase(it);
    }

}

TAtomicSharedPtr<TTaskGuard> IDistributedQueue::GetTask(const TString& taskId) {

    TMap<TString, TString> taskInfo;
    NUtil::TTSKVRecordParser::Parse<';', '='>(taskId, taskInfo);

    auto lock = IsTrue(taskInfo["fake_lock"]) ? FakeLock(taskId) : WriteLock(taskId, TDuration::Zero());
    if (!!lock) {
        TGuard<TMutex> g(MutexLocalTasks);
        auto it = LocalTasks.find(taskId);
        TTaskData::TPtr taskData;
        if (it == LocalTasks.end()) {
            g.Release();
            taskData = LoadTask(taskId);
        } else {
            taskData = MakeAtomicShared<TTaskData>(it->second);
        }
        return MakeAtomicShared<TTaskGuard>(this, lock, taskData);
    }
    return MakeAtomicShared<TTaskGuard>();
}

TAtomicSharedPtr<TTaskGuard> IDistributedQueue::PutTask(const TTaskData& data, const bool rewrite) {
    auto lock = WriteLock(data.GetTaskIdentifier(), TDuration::Zero());
    if (!lock) {
        return MakeAtomicShared<TTaskGuard>();
    } else {
        if (data.IsLocal()) {
            TGuard<TMutex> g(MutexLocalTasks);
            if (rewrite) {
                LocalTasks[data.GetTaskIdentifier()] = data;
            } else {
                LocalTasks.emplace(data.GetTaskIdentifier(), data);
            }
            return MakeAtomicShared<TTaskGuard>(this, lock, MakeAtomicShared<TTaskData>(data));
        } else {
            if (SaveTask(data, rewrite)) {
                return MakeAtomicShared<TTaskGuard>(this, lock, MakeAtomicShared<TTaskData>(data));
            } else {
                return MakeAtomicShared<TTaskGuard>();
            }
        }
    }
}
