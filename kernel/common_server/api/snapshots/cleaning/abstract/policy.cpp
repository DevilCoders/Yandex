#include "policy.h"

#include <kernel/common_server/api/snapshots/controller.h>

namespace NCS {

    NJson::TJsonValue ISnapshotObjectsCleaningPolicy::SerializeToJson() const {
        NJson::TJsonValue result;
        TJsonProcessor::WriteContainerArrayStrings(result, "allowed_statuses_for_cleaning", AllowedStatusesForCleaning);
        return result;
    }

    bool ISnapshotObjectsCleaningPolicy::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        return TJsonProcessor::ReadContainer(jsonInfo, "allowed_statuses_for_cleaning", AllowedStatusesForCleaning);
    }

    NFrontend::TScheme ISnapshotObjectsCleaningPolicy::GetScheme(const IBaseServer& /*server*/) const {
        NFrontend::TScheme result;
        result.Add<TFSVariants>("allowed_statuses_for_cleaning").InitVariants<NCS::TDBSnapshot::ESnapshotStatus>().SetMultiSelect(true);
        return result;
    }

    bool ISnapshotObjectsCleaningPolicy::CleanSnapshotObjects(const TString& groupId, const ISnapshotsController& controller) const {
        TVector<NCS::TDBSnapshot> snapshots;
        if (!controller.GetSnapshotsManager().GetGroupSnapshots(groupId, AllowedStatusesForCleaning, snapshots)) {
            TFLEventLog::Error("cannot_get_group_snapshots");
            return false;
        }
        TSRCondition cleaningCondition;
        if (!PrepareCleaningCondition(cleaningCondition)) {
            TFLEventLog::Error("cannot_prepare_cleaning_condition")("group_id", groupId);
            return false;
        }
        for (auto&& snapshotInfo : snapshots) {
            const auto& snapshotsObjectManager = controller.GetSnapshotObjectsManager(snapshotInfo);
            if (!snapshotsObjectManager->RemoveSnapshotObjectsBySRCondition(snapshotInfo.GetSnapshotId(), cleaningCondition)) {
                TFLEventLog::Error("cannot_remove_snapshot_objects");
                return false;
            }
        }
        return true;
    };

}
