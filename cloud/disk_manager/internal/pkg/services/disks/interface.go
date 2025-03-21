package disks

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
)

////////////////////////////////////////////////////////////////////////////////

type Service interface {
	CreateDisk(
		ctx context.Context,
		req *disk_manager.CreateDiskRequest,
	) (string, error)

	DeleteDisk(
		ctx context.Context,
		req *disk_manager.DeleteDiskRequest,
	) (string, error)

	ResizeDisk(
		ctx context.Context,
		req *disk_manager.ResizeDiskRequest,
	) (string, error)

	AlterDisk(
		ctx context.Context,
		req *disk_manager.AlterDiskRequest,
	) (string, error)

	AssignDisk(
		ctx context.Context,
		req *disk_manager.AssignDiskRequest,
	) (string, error)

	UnassignDisk(
		ctx context.Context,
		req *disk_manager.UnassignDiskRequest,
	) (string, error)

	DescribeDiskModel(
		ctx context.Context,
		req *disk_manager.DescribeDiskModelRequest,
	) (*disk_manager.DiskModel, error)

	StatDisk(
		ctx context.Context,
		req *disk_manager.StatDiskRequest,
	) (*disk_manager.DiskStats, error)

	MigrateDisk(
		ctx context.Context,
		req *disk_manager.MigrateDiskRequest,
	) (string, error)

	SendMigrationSignal(
		ctx context.Context,
		req *disk_manager.SendMigrationSignalRequest,
	) error
}
