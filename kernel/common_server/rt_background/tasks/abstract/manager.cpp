#include "manager.h"

namespace NCS {
    namespace NBackground {

        bool ITasksManager::RestoreCurrentTaskIds(const TString& queueId, const TString& ownerId, TSet<TString>& currentTaskIds) const {
            TVector<TRTQueueTaskContainer> tasks;
            if (!RestoreOwnerTasks(queueId, ownerId, tasks, false)) {
                return false;
            }
            TSet<TString> result;
            for (auto&& i : tasks) {
                result.emplace(i.GetInternalTaskId());
            }
            std::swap(result, currentTaskIds);
            return true;
        }

    }
}
