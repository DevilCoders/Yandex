#include "content.h"
#include <kernel/common_server/api/snapshots/abstract.h>
#include <kernel/common_server/api/snapshots/object.h>

namespace NCS {

    bool ISnapshotContentFetcher::FetchData(const TDBSnapshot& snapshotInfo, const ISnapshotsController& controller, const IBaseServer& server, const TString& userId) const {
        auto gLogging = TFLRecords::StartContext()("snapshot_code", snapshotInfo.GetSnapshotCode())("snapshot_id", snapshotInfo.GetSnapshotId());
        if (!DoFetchData(snapshotInfo, controller, server, userId)) {
            TFLEventLog::Error("cannot fetch data");
            return false;
        }
        if (!controller.SetSnapshotIsReady(snapshotInfo.GetSnapshotCode())) {
            TFLEventLog::Error("cannot set snapshot status as ready");
            return false;
        }
        return true;
    }

}
