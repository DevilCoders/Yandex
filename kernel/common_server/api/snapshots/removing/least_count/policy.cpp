#include "policy.h"
#include <kernel/common_server/api/snapshots/object.h>
#include <kernel/common_server/api/snapshots/controller.h>

namespace NCS {
    TLeastCountRemovePolicy::TFactory::TRegistrator<TLeastCountRemovePolicy> TLeastCountRemovePolicy::Registrator(TLeastCountRemovePolicy::GetTypeName());

    NJson::TJsonValue TLeastCountRemovePolicy::DoSerializeToJson() const {
        NJson::TJsonValue result = NJson::JSON_MAP;
        TJsonProcessor::Write(result, "count", Count);
        TJsonProcessor::Write(result, "totally_removing", TotallyRemoving);
        TJsonProcessor::Write(result, "removing_count_limit", RemovingCountLimit);
        return result;
    }

    bool TLeastCountRemovePolicy::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        if (!TJsonProcessor::Read(jsonInfo, "count", Count)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "totally_removing", TotallyRemoving)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "removing_count_limit", RemovingCountLimit)) {
            return false;
        }
        return true;
    }

    NFrontend::TScheme TLeastCountRemovePolicy::DoGetScheme(const IBaseServer& /*server*/) const {
        NFrontend::TScheme result;
        result.Add<TFSNumeric>("count").SetDefault(3);
        result.Add<TFSNumeric>("removing_count_limit").SetDefault(1);
        result.Add<TFSBoolean>("totally_removing").SetDefault(true);
        return result;
    }

    bool TLeastCountRemovePolicy::MarkToRemove(const TString& groupId, const TString& userId, const ISnapshotsController& controller) const {
        TVector<TDBSnapshot> dbSnapshotsReady;
        if (!controller.GetSnapshotsManager().GetGroupSnapshots(groupId, { TDBSnapshot::ESnapshotStatus::Ready }, dbSnapshotsReady)) {
            TFLEventLog::Error("cannot restore group snapshots")("group_id", groupId);
            return false;
        }
        TVector<TDBSnapshot> dbSnapshotsRemoving;
        if (!controller.GetSnapshotsManager().GetGroupSnapshots(groupId, { TDBSnapshot::ESnapshotStatus::Removing }, dbSnapshotsRemoving)) {
            TFLEventLog::Error("cannot restore group snapshots")("group_id", groupId);
            return false;
        }

        const auto predSort = [](const TDBSnapshot& l, const TDBSnapshot& r) {
            return l.GetLastStatusModification() < r.GetLastStatusModification();
        };
        std::sort(dbSnapshotsReady.begin(), dbSnapshotsReady.end(), predSort);

        TSet<ui32> snapshotIdsForRemove;
        for (ui32 i = 0; i + Count < dbSnapshotsReady.size(); ++i) {
            if (dbSnapshotsRemoving.size() + snapshotIdsForRemove.size() >= RemovingCountLimit) {
                TFLEventLog::Warning("removing snapshots count limit is reached")("limit", RemovingCountLimit)("current", dbSnapshotsRemoving.size())("new_removing", snapshotIdsForRemove.size());
                break;
            }
            snapshotIdsForRemove.emplace(dbSnapshotsReady[i].GetSnapshotId());
        }
        auto session = controller.GetSnapshotsManager().BuildNativeSession(false);
        if (!controller.GetSnapshotsManager().UpdateForDelete(snapshotIdsForRemove, userId, session) || !session.Commit()) {
            TFLEventLog::Error("cannot update snapshots for delete");
            return false;
        }
        if (TotallyRemoving) {
            TVector<TDBSnapshot> dbSnapshotsRemoved;
            if (!controller.GetSnapshotsManager().GetGroupSnapshots(groupId, { TDBSnapshot::ESnapshotStatus::Removed }, dbSnapshotsRemoved)) {
                TFLEventLog::Error("cannot restore group snapshots")("group_id", groupId);
                return false;
            }
            TSet<TString> snapshotCodes;
            for (auto&& i : dbSnapshotsRemoved) {
                snapshotCodes.emplace(i.GetSnapshotCode());
            }
            auto session = controller.GetSnapshotsManager().BuildNativeSession(false);
            if (!controller.GetSnapshotsManager().RemoveObject(snapshotCodes, userId, session) || !session.Commit()) {
                TFLEventLog::Error("cannot remove snapshots totally");
                return false;
            }
        }
        return true;
    }

}
