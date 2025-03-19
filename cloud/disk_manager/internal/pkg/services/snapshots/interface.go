package snapshots

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
)

////////////////////////////////////////////////////////////////////////////////

type Service interface {
	CreateSnapshot(
		ctx context.Context,
		req *disk_manager.CreateSnapshotRequest,
	) (string, error)

	DeleteSnapshot(
		ctx context.Context,
		req *disk_manager.DeleteSnapshotRequest,
	) (string, error)

	RestoreDiskFromSnapshot(
		ctx context.Context,
		req *disk_manager.RestoreDiskFromSnapshotRequest,
	) (string, error)
}
