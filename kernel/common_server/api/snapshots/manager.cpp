#include "manager.h"
#include "controller.h"

namespace NCS {

    bool TSnapshotsManager::GetGroupSnapshots(const TString& groupId, const TSet<TDBSnapshot::ESnapshotStatus>& statusFilter, TVector<TDBSnapshot>& result) const {
        TVector<TDBSnapshot> snapshots;
        {
            auto session = BuildNativeSession(false);
            TSRCondition condition;
            auto& srMulti = condition.Ret<TSRMulti>();
            srMulti.InitNode<TSRBinary>("snapshot_group_id", groupId);
            if (statusFilter.size()) {
                srMulti.InitNode<TSRBinary>("status", statusFilter);
            }
            if (!RestoreObjectsBySRCondition(snapshots, condition, session)) {
                return false;
            }
        }
        std::swap(snapshots, result);
        return true;
    }

    bool TSnapshotsManager::GetGroupSnapshotCodes(const TString& groupId, TSet<TString>& snapshotCodes) const {
        TVector<TDBSnapshot> snapshots;
        {
            auto session = BuildNativeSession(false);
            if (!RestoreObjectsBySRCondition(snapshots, TSRCondition().Init<TSRBinary>("snapshot_group_id", groupId), session)) {
                return false;
            }
        }
        for (auto&& i : snapshots) {
            snapshotCodes.emplace(i.GetSnapshotCode());
        }
        return true;
    }

    bool TSnapshotsManager::GetSnapshotsByStatus(const TDBSnapshot::ESnapshotStatus status, TVector<TDBSnapshot>& result) const {
        auto session = BuildNativeSession(true);
        return RestoreObjectsBySRCondition(result, TSRCondition().Init<TSRBinary>("status", status), session);
    }

    TMaybe<TDBSnapshot> TSnapshotsManager::GetActualSnapshotInfo(const TString& groupId) const {
        TVector<TDBSnapshot> snapshots;
        CHECK_WITH_LOG(GetAllObjects(snapshots));
        TMaybe<TDBSnapshot> result;
        for (auto&& i : snapshots) {
            if (groupId != i.GetSnapshotGroupId()) {
                continue;
            }
            if (i.GetStatus() > TDBSnapshot::ESnapshotStatus::Ready) {
                continue;
            }
            if (!i.GetEnabled()) {
                continue;
            }
            if (!result || result->GetSnapshotId() < i.GetSnapshotId()) {
                result = i;
            }
        }
        return result;
    }

    TMaybe<TDBSnapshot> TSnapshotsManager::GetActualSnapshotConstruction(const TString& groupId) const {
        TVector<TDBSnapshot> snapshots;
        CHECK_WITH_LOG(GetAllObjects(snapshots));
        TMaybe<TDBSnapshot> result;
        for (auto&& i : snapshots) {
            if (groupId != i.GetSnapshotGroupId()) {
                continue;
            }
            if (i.GetStatus() != TDBSnapshot::ESnapshotStatus::InConstruction) {
                continue;
            }
            if (!result || result->GetSnapshotId() < i.GetSnapshotId()) {
                result = i;
            }
        }
        return result;
    }

    bool TSnapshotsManager::StartSnapshotConstruction(const TDBSnapshot& newSnapshotExt, ui32& snapshotId, const TString& userId, const ISnapshotsController& controller) const {
        auto gLogging = TFLRecords::StartContext().Method("StartSnapshotConstruction")("group_id", newSnapshotExt.GetSnapshotGroupId());
        auto snapshotObjectsOperator = controller.GetSnapshotObjectsManager(newSnapshotExt);
        if (!snapshotObjectsOperator) {
            TFLEventLog::Error("incorrect snapshot objects operator");
            return false;
        }
        auto session = TBase::BuildNativeSession(false);
        TDBSnapshot newSnapshot = newSnapshotExt;
        newSnapshot.SetSnapshotId(0);
        newSnapshot.SetStatus(TDBSnapshot::ESnapshotStatus::InConstruction);
        newSnapshot.SetLastStatusModification(Now());
        NStorage::TObjectRecordsSet<TDBSnapshot> snapshots;
        if (newSnapshot.GetSnapshotId()) {
            TFLEventLog::Error("snapshot_id not accepted external initialization");
            return false;
        }
        if (!AddObjects({ newSnapshot }, userId, session, &snapshots)) {
            TFLEventLog::Error("cannot add new snapshot");
            return false;
        }
        if (snapshots.size() != 1) {
            TFLEventLog::Error("cannot add new snapshot")("reason", "incorrect records count after add operation");
            return false;
        }
        snapshotId = snapshots.front().GetSnapshotId();
        if (!snapshotObjectsOperator->CreateSnapshot(snapshotId)) {
            TFLEventLog::Error("cannot create snapshot by objects operator");
            return false;
        }
        if (!session.Commit()) {
            TFLEventLog::Error("cannot commit transaction");
            return false;
        }
        return true;
    }

    bool TSnapshotsManager::UpdateForConstruction(const ui32 snapshotId, const TString& userId, NCS::TEntitySession& session) const {
        TSRCondition srUpdate;
        srUpdate.RetObject<TSRBinary>("status", TDBSnapshot::ESnapshotStatus::InConstruction);
        TSRCondition srCondition;
        srCondition.RetObject<TSRBinary>("snapshot_id", snapshotId);
        return UpdateRecord(srUpdate, srCondition, userId, session, nullptr, -1);
    }

    bool TSnapshotsManager::RestoreByIds(const TSet<ui32>& objectIds, TVector<TDBSnapshot>& objects, NCS::TEntitySession& session) const {
        if (objectIds.empty()) {
            return true;
        }
        return RestoreObjectsBySRCondition(objects, TSRCondition().Init<TSRBinary>("snapshot_id", objectIds), session);
    }

    bool TSnapshotsManager::UpdateForDelete(const TSet<ui32>& objectIds, const TString& userId, NCS::TEntitySession& session) const {
        if (objectIds.empty()) {
            return true;
        }
        TSRCondition srUpdate;
        srUpdate.RetObject<TSRBinary>("status", TDBSnapshot::ESnapshotStatus::Removing);
        TSRCondition srCondition;
        auto& srMulti = srCondition.RetObject<TSRMulti>();
        srMulti.InitNode<TSRBinary>("snapshot_id", objectIds);
        srMulti.InitNode<TSRBinary>("status", TDBSnapshot::ESnapshotStatus::Ready);
        return UpdateRecord(srUpdate, srCondition, userId, session, nullptr, -1);
    }

    bool TSnapshotsManager::RemovePermanently(const TSet<ui32>& objectIds, const TString& userId, NCS::TEntitySession& session) const {
        if (objectIds.empty()) {
            return true;
        }
        TSRCondition condition;
        auto& srMulti = condition.Ret<TSRMulti>();
        srMulti.InitNode<TSRBinary>("status", TDBSnapshot::ESnapshotStatus::Removed);
        srMulti.InitNode<TSRBinary>("snapshot_id", objectIds);
        return RemoveRecords(condition, userId, session);
    }

    bool TSnapshotsManager::UpdateAsRemoved(const TSet<ui32>& objectIds, const TString& userId, NCS::TEntitySession& session) const {
        if (objectIds.empty()) {
            return true;
        }
        TSRCondition srUpdate;
        srUpdate.RetObject<TSRBinary>("status", TDBSnapshot::ESnapshotStatus::Removed);
        TSRCondition srCondition;
        srCondition.RetObject<TSRBinary>("snapshot_id", objectIds);
        return UpdateRecord(srUpdate, srCondition, userId, session, nullptr, -1);
    }

    bool TSnapshotsManager::DoUpsertObject(const TDBSnapshot& object, const TString& userId, NCS::TEntitySession& session, bool* isUpdateExt, TDBSnapshot* resultObject) const {
        TDBSnapshot localObject = object;
        localObject.SetLastStatusModification(ModelingNow());
//        TMaybe<TDBSnapshot> snapshot;
//        if (!RestoreObject(object.GetSnapshotId(), snapshot, session)) {
//            session::Error("cannot restore snapshot");
//            return false;
//        }
//        if (!snapshot) {
//            session::Error("snapshot must been started");
//            return false;
//        }
//        if (snapshot->GetStatus() != TDBSnapshot::ESnapshotStatus::Ready) {
//            session::Error("snapshot is not ready for modification")("status", snapshot->GetStatus());
//            return false;
//        }
//        if (object.GetStatus() != snapshot->GetStatus()) {
//            session::Error("snapshot status is immutable")("current_status", snapshot->GetStatus());
//            return false;
//        }
        return TBase::DoUpsertObject(localObject, userId, session, isUpdateExt, resultObject);
    }

    bool TSnapshotsManager::FinishSnapshotConstruction(const TString& snapshotCode, const TString& userId) const {
        auto gLogging = TFLRecords::StartContext().Method("FinishSnapshotConstruction")("snapshot_code", snapshotCode);
        auto session = TBase::BuildNativeSession(false);
        TMaybe<TDBSnapshot> snapshot;
        if (!RestoreObject(snapshotCode, snapshot, session)) {
            TFLEventLog::Error("cannot restore snapshot");
            return false;
        }
        if (!snapshot) {
            TFLEventLog::Error("restored snapshot is empty");
            return false;
        }
        if (snapshot->GetStatus() != TDBSnapshot::ESnapshotStatus::InConstruction) {
            TFLEventLog::Error("snapshot status is incorrect")("actual_status", snapshot->GetStatus());
            return false;
        }
        snapshot->SetStatus(TDBSnapshot::ESnapshotStatus::Ready).SetEnabled(true);
        if (!TBase::UpsertObject(*snapshot, userId, session)) {
            TFLEventLog::Error("cannot upsert object into db");
            return false;
        }
        if (!session.Commit()) {
            TFLEventLog::Error("cannot commit transaction");
            return false;
        }
        return true;
    }

    bool TSnapshotsManager::UpdateContext(const TString& snapshotCode, const TSnapshotContentFetcherContext& newContext, const TString& userId) const {
        auto gLogging = TFLRecords::StartContext().Method("UpdateContext")("snapshot_code", snapshotCode);
        auto session = TBase::BuildNativeSession(false);
        TMaybe<TDBSnapshot> snapshot;
        if (!RestoreObject(snapshotCode, snapshot, session)) {
            TFLEventLog::Error("cannot restore snapshot");
            return false;
        }
        if (!snapshot) {
            TFLEventLog::Error("restored snapshot is empty");
            return false;
        }
        if (snapshot->GetStatus() != TDBSnapshot::ESnapshotStatus::InConstruction) {
            TFLEventLog::Error("snapshot status is incorrect")("actual_status", snapshot->GetStatus());
            return false;
        }
        snapshot->SetFetchingContext(newContext);
        if (!TBase::UpsertObject(*snapshot, userId, session)) {
            TFLEventLog::Error("cannot upsert object into db");
            return false;
        }
        if (!session.Commit()) {
            TFLEventLog::Error("cannot commit transaction");
            return false;
        }
        return true;
    }

}
