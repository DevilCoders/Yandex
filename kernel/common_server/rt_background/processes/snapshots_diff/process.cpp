#include "process.h"

#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/rt_background/manager.h>
#include <kernel/common_server/proto/background.pb.h>
#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <kernel/common_server/rt_background/tasks/abstract/object.h>
#include <kernel/common_server/api/snapshots/controller.h>
#include <util/random/random.h>
#include "task.h"

namespace NCS {
    TSnapshotsDiffProcess::TFactory::TRegistrator<TSnapshotsDiffProcess> TSnapshotsDiffProcess::Registrator(TSnapshotsDiffProcess::GetTypeName());

    TAtomicSharedPtr<IRTBackgroundProcessState> TSnapshotsDiffProcess::DoExecute(TAtomicSharedPtr<IRTBackgroundProcessState> /*state*/, const TExecutionContext& context) const {
        const auto& server = context.GetServer();
        auto tasksManager = server.GetRTBackgroundManager()->GetTasksManager(TasksManagerId);
        if (!tasksManager) {
            TFLEventLog::Error("incorrect tasks manager")("tasks_manager_id", TasksManagerId);
            return nullptr;
        }
        TSet<TString> currentTaskIds;
        if (!tasksManager->RestoreCurrentTaskIds(QueueId, GetRTProcessName(), currentTaskIds)) {
            TFLEventLog::Error("cannot restore task ids")("tasks_manager_id", TasksManagerId);
            return nullptr;
        }

        TVector<TDBSnapshotsGroup> snapshotGroups;
        TVector<NCS::NBackground::TRTQueueTaskContainer> newTasks;
        if (!context.GetServer().GetSnapshotsController().GetGroupsManager().GetAllObjects(snapshotGroups)) {
            TFLEventLog::Error("cannot restore snapshot groups");
            return nullptr;
        }
        for (auto&& i : snapshotGroups) {
            if (!i.GetSnapshotsDiffPolicy()) {
                continue;
            }
            if (currentTaskIds.contains(i.GetGroupId())) {
                continue;
            }

            const TString internalTaskId = i.GetGroupId();
            auto action = MakeHolder<TSnapshotsDiffTask>(internalTaskId);
            NCS::NBackground::TRTQueueTaskContainer task(new NCS::NBackground::IRTQueueTask(action.Release()));
            task->SetOwnerId(GetRTProcessName()).SetQueueId(QueueId).SetInternalTaskId(internalTaskId).AddTag("group_id", i.GetGroupId());
            newTasks.emplace_back(std::move(task));
        }

        if (!tasksManager->AddTasks(newTasks)) {
            TFLEventLog::Error("cannot store queue tasks");
            return nullptr;
        }
        return MakeAtomicShared<IRTBackgroundProcessState>();
    }

}
