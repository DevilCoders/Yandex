#pragma once
#include "manager.h"
#include "abstract.h"
#include <kernel/common_server/abstract/frontend.h>
#include "storage/abstract/storage.h"

namespace NCS {
    class TSnapshotsController: public ISnapshotsController {
    private:
        using TBase = ISnapshotsController;
        THolder<TSnapshotGroupsManager> GroupsManager;
        THolder<TSnapshotsManager> SnapshotsManager;
        const TSnapshotsControllerConfig Config;
        const IBaseServer& Server;
    public:
        virtual bool StoreSnapshotFetcherContext(const TString& snapshotCode, const TSnapshotContentFetcherContext& context) const override {
            return SnapshotsManager->UpdateContext(snapshotCode, context, "controller");
        }
        virtual bool SetSnapshotIsReady(const TString& snapshotCode) const override {
            return SnapshotsManager->FinishSnapshotConstruction(snapshotCode, "controller");
        }
        TSnapshotsController(const TSnapshotsControllerConfig& config, const IBaseServer& server)
            : Config(config)
            , Server(server)
        {
            NStorage::IDatabase::TPtr db = Server.GetDatabase(config.GetDBName());
            CHECK_WITH_LOG(db);
            GroupsManager = MakeHolder<TSnapshotGroupsManager>(db, config.GetSnapshotGroupsManagerConfig().GetHistoryConfig(), Server);
            SnapshotsManager = MakeHolder<TSnapshotsManager>(db, config.GetSnapshotsManagerConfig().GetHistoryConfig());
        }

        virtual const TSnapshotGroupsManager& GetGroupsManager() const override {
            return *GroupsManager;
        }
        virtual const TSnapshotsManager& GetSnapshotsManager() const override {
            return *SnapshotsManager;
        }

        bool Start() {
            return GroupsManager->Start() && SnapshotsManager->Start();
        }

        bool Stop() {
            if (!GroupsManager->Stop() || !SnapshotsManager->Stop()) {
                return false;
            }
            return true;
        }

        TMaybe<TDBSnapshot> GetActualSnapshotInfo(const TString& groupId) const {
            auto groupObject = GroupsManager->GetCustomObject(groupId);
            if (!groupObject) {
                return Nothing();
            }
            if (!!groupObject->GetDefaultSnapshotCode()) {
                return SnapshotsManager->GetCustomObject(groupObject->GetDefaultSnapshotCode());
            }
            return SnapshotsManager->GetActualSnapshotInfo(groupId);
        };

        virtual NSnapshots::TObjectsManagerContainer GetSnapshotObjectsManager(const TDBSnapshot& snapshotInfo) const override {
            auto group = GetGroupsManager().GetCustomObject(snapshotInfo.GetSnapshotGroupId());
            if (!group) {
                TFLEventLog::Error("incorrect snapshot group")("group_id", snapshotInfo.GetSnapshotGroupId())("snapshot_code", snapshotInfo.GetSnapshotCode());
                return NSnapshots::TObjectsManagerContainer();
            }
            return group->GetObjectsManager();
        }

        virtual bool GetObjectInfo(const TVector<NCS::NStorage::TTableRecordWT>& objectIds, const TDBSnapshot& snapshotInfo, TVector<NSnapshots::TMappedObject>& result) const override {
            auto gLogging = TFLRecords::StartContext()("group_id", snapshotInfo.GetSnapshotGroupId())("snapshot_id", snapshotInfo.GetSnapshotId());
            auto snapshotOperator = GetSnapshotObjectsManager(snapshotInfo);
            if (!snapshotOperator) {
                TFLEventLog::Error("cannot determ snapshot objects manager");
                return false;
            }
            if (!snapshotOperator->GetObjects(objectIds, snapshotInfo.GetSnapshotId(), result)) {
                return false;
            }
            return true;
        };

    };
}
