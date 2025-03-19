#pragma once
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/storage/records/t_record.h>
#include "fetching/abstract/context.h"
#include "storage/abstract/storage.h"
#include "object.h"
#include "manager.h"

namespace NCS {
    class ISnapshotsController {
    public:
        ISnapshotsController() = default;
        virtual ~ISnapshotsController() = default;

        virtual NSnapshots::TObjectsManagerContainer GetSnapshotObjectsManager(const TDBSnapshot& snapshotInfo) const = 0;
        virtual bool StoreSnapshotFetcherContext(const TString& snapshotCode, const TSnapshotContentFetcherContext& context) const = 0;
        virtual bool SetSnapshotIsReady(const TString& snapshotCode) const = 0;
        virtual const TSnapshotsManager& GetSnapshotsManager() const = 0;
        virtual const TSnapshotGroupsManager& GetGroupsManager() const = 0;
        template <class TObject>
        bool GetObjectInfo(const TVector<NCS::NStorage::TTableRecordWT>& objectIds, const TDBSnapshot& snapshotInfo, TVector<TObject>& result) const {
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
        }

        virtual bool GetObjectInfo(const TVector<NCS::NStorage::TTableRecordWT>& objectIds, const TDBSnapshot& snapshotInfo, TVector<NSnapshots::TMappedObject>& result) const = 0;
    };
}
