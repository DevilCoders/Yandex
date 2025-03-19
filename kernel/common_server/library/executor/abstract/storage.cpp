#include "storage.h"
#include "task.h"
#include <kernel/common_server/util/blob_with_header.h>
#include <util/system/guard.h>

void IDTasksStorage::ClearOld(const TAtomic& active, const TDuration borderWaiting) const {
    try {
        TVector<TString> nodes = GetNodes();
        TVector<TString> idsForCheck;
        for (size_t i = 0; i < nodes.size() && AtomicGet(active); ) {
            size_t iNext = Min<size_t>(i + 10000, nodes.size());
            idsForCheck.clear();
            for (ui32 j = i; j < iNext; ++j) {
                if (nodes[j].StartsWith("data--")) {
                    idsForCheck.emplace_back(nodes[j].substr(6));
                }
            }
            TVector<TString> incorrectIds;
            TSet<TString> expiredIds;
            TVector<TAtomicSharedPtr<IDistributedData>> data;
            if (RestoreDataUnsafe(idsForCheck, data, &incorrectIds)) {
                for (auto&& d : data) {
                    if (!!d && d->IsExpired(borderWaiting)) {
                        expiredIds.emplace(d->GetIdentifier());
                        INFO_LOG << "Removing old data " << d->GetIdentifier() << " by deadline " << Endl;
                    }
                }
                if (incorrectIds.size()) {
                    if (!RemoveData(MakeSet(incorrectIds))) {
                        ERROR_LOG << "Cannot remove incorrect ids: " << incorrectIds.size() << Endl;
                    } else {
                        INFO_LOG << "Removed incorrect ids: " << incorrectIds.size() << Endl;
                    }
                }
                if (expiredIds.size()) {
                    if (!RemoveData(expiredIds)) {
                        ERROR_LOG << "Cannot remove expired ids: " << expiredIds.size() << Endl;
                    } else {
                        INFO_LOG << "Remove expired ids: " << expiredIds.size() << Endl;
                    }
                }
            } else {
                ERROR_LOG << "Cannot restore tasks" << Endl;
            }
            i = iNext;
        }
    } catch (...) {
        ERROR_LOG << "Cannot take info about full tasks list: " << CurrentExceptionMessage() << Endl;
    }
}

bool IDTasksStorage::IsLocalTask(const TString& taskId) const {
    ::TGuard<TMutex> g(MutexLocalTasks);
    return LocalTasks.contains(taskId);
}

void IDTasksStorage::RemoveTask(const TString& identifier) const {
    ::TGuard<TMutex> g(MutexLocalTasks);
    auto it = LocalTasks.find(identifier);
    if (it == LocalTasks.end()) {
        g.Release();
        RemoveDataImpl("task--" + identifier);
    } else {
        LocalTasks.erase(it);
    }
}

IDistributedTask::TPtr IDTasksStorage::RestoreTaskUnsafe(const TString& identifier) const {
    {
        ::TGuard<TMutex> g(MutexLocalTasks);
        auto it = LocalTasks.find(identifier);
        if (it != LocalTasks.end()) {
            return it->second;
        }
    }
    THolder<IDistributedTask> result;
    {
        TBlob data;
        if (ReadDataImpl("task--" + identifier, data) != EReadStatus::OK) {
            WARNING_LOG << "No task data in storage: " << identifier << Endl;
            return nullptr;
        }
        TBlobWithHeader<NFrontendProto::TTaskMeta> bwh;
        if (!bwh.Load(data)) {
            return nullptr;
        }

        const NFrontendProto::TTaskMeta& dataMeta = bwh.GetHeader();

        TTaskConstructionContext context(dataMeta.GetType(), identifier);
        result.Reset(IDistributedTask::TFactory::Construct(dataMeta.GetType(), context));
        if (!result) {
            ERROR_LOG << "Incorrect task type: '" << dataMeta.GetType() << "'" << Endl;
            return nullptr;
        }
        if (!result->ParseMetaFromProto(dataMeta)) {
            ERROR_LOG << "Incorrect task data: '" << dataMeta.DebugString() << "'" << Endl;
            return nullptr;
        }
        if (!result->Deserialize(bwh.GetData())) {
            WARNING_LOG << "Incorrect data in storage" << Endl;
            return nullptr;
        }
    }
    return result.Release();
}

IDistributedTask::TPtr IDTasksStorage::RestoreTask(const TString& identifier) const {
    return RestoreTaskUnsafe(identifier);
}

void IDTasksStorage::StoreLocalTask(IDistributedTask::TPtr data) const {
    if (!!data) {
        ::TGuard<TMutex> g(MutexLocalTasks);
        LocalTasks[data->GetIdentifier()] = data;
    }
}

bool IDTasksStorage::StoreTask2(const IDistributedTask* data) const {
    if (data) {
        NFrontendProto::TTaskMeta dataMeta;
        data->SerializeMetaToProto(dataMeta);

        TBlobWithHeader<NFrontendProto::TTaskMeta> bwh(dataMeta, data->Serialize());
        return StoreDataImpl2("task--" + data->GetIdentifier(), bwh.Save());
    }
    return true;
}
