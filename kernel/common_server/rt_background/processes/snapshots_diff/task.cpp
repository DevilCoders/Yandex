#include "task.h"
#include <kernel/common_server/api/snapshots/controller.h>
#include <kernel/common_server/resources/manager.h>
#include <kernel/common_server/util/enum_cast.h>
#include <kernel/common_server/api/snapshots/objects/mapped/object.h>

namespace NCS {
    TSnapshotsDiffTask::TFactory::TRegistrator<TSnapshotsDiffTask> TSnapshotsDiffTask::Registrator(TSnapshotsDiffTask::GetTypeName());
    TSnapshotsDiffState::TFactory::TRegistrator<TSnapshotsDiffState> TSnapshotsDiffState::Registrator(TSnapshotsDiffState::GetTypeName());

    bool TSnapshotsDiffTask::DoDeserializeFromProto(const NCommonServerProto::TSnapshotsDiffTask& proto) {
        SnapshotsGroupId = proto.GetSnapshotsGroupId();
        return true;
    }

    NCommonServerProto::TSnapshotsDiffTask TSnapshotsDiffTask::DoSerializeToProto() const {
        NCommonServerProto::TSnapshotsDiffTask result;
        result.SetSnapshotsGroupId(SnapshotsGroupId);
        return result;
    }

    TAtomicSharedPtr<TSnapshotsDiffState> TSnapshotsDiffTask::DoExecuteImpl(const NCS::NBackground::IRTQueueTask& /*task*/, TAtomicSharedPtr<TSnapshotsDiffState> state, const IBaseServer& bServer) const {
        auto gLogging = TFLRecords::StartContext()("&snapshot_group_id", SnapshotsGroupId);

        TMaybe<TDBSnapshotsGroup> sGroup = bServer.GetSnapshotsController().GetGroupsManager().GetCustomObject(SnapshotsGroupId);
        if (!sGroup) {
            TFLEventLog::Error("incorrect group for snapshots comparing");
            return nullptr;
        }
        if (!sGroup->GetSnapshotsDiffPolicy()) {
            TFLEventLog::Error("no comparing policy in snapshots group");
            return nullptr;
        }

        TMaybe<TDBSnapshot> dbSnapshot = bServer.GetSnapshotsController().GetSnapshotsManager().GetActualSnapshotInfo(SnapshotsGroupId);
        if (!dbSnapshot || (state && dbSnapshot->GetSnapshotId() <= state->GetLastSnapshotId())) {
            return state;
        }

        if (!!state) {
            TMaybe<TDBSnapshot> dbSnapshotOld = bServer.GetSnapshotsController().GetSnapshotsManager().GetSnapshotById(state->GetLastSnapshotId());
            if (!dbSnapshotOld) {
                TFLEventLog::Error("cannot restore old snapshot")("snapshot_id", state->GetLastSnapshotId());
                return nullptr;
            }
            if (dbSnapshotOld->GetSnapshotGroupId() != SnapshotsGroupId) {
                TFLEventLog::Error("incorrect old snapshot")("snapshot_id", state->GetLastSnapshotId())("real_group_id", dbSnapshot->GetSnapshotGroupId());
                return nullptr;
            }
            TVector<NSnapshots::TMappedObject> oldObjects;
            if (!bServer.GetSnapshotsController().GetSnapshotObjectsManager(*dbSnapshot)->GetAllObjects(state->GetLastSnapshotId(), oldObjects)) {
                TFLEventLog::Error("cannot restore old objects")("snapshot_id", state->GetLastSnapshotId());
                return nullptr;
            }
            TVector<NSnapshots::TMappedObject> newObjects;
            if (!bServer.GetSnapshotsController().GetSnapshotObjectsManager(*dbSnapshot)->GetAllObjects(dbSnapshot->GetSnapshotId(), newObjects)) {
                TFLEventLog::Error("cannot restore new objects")("snapshot_id", dbSnapshot->GetSnapshotId());
                return nullptr;
            }
            if (!sGroup->GetSnapshotsDiffPolicy()->Compare(*sGroup, oldObjects, newObjects, bServer)) {
                TFLEventLog::Error("cannot compare objects")("new_snapshot_id", dbSnapshot->GetSnapshotId())("old_snapshot_id", state->GetLastSnapshotId());
                return nullptr;
            }
        }

        THolder<TSnapshotsDiffState> nextState = MakeHolder<TSnapshotsDiffState>();
        nextState->SetLastSnapshotId(dbSnapshot->GetSnapshotId());
        return nextState.Release();
    }
}
