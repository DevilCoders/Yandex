package snapshot

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type ResourceInfo struct {
	Size        int64
	StorageSize int64
}

type TaskState struct {
	TaskID           string
	Offset           int64
	HeartbeatTimeout time.Duration
	PingPeriod       time.Duration
	BackoffTimeout   time.Duration
}

type CreateImageFromURLState struct {
	SrcURL            string
	Format            string
	DstImageID        string
	FolderID          string
	OperationCloudID  string
	OperationFolderID string
	State             TaskState
}

type CreateImageFromImageState struct {
	SrcImageID        string
	DstImageID        string
	FolderID          string
	OperationCloudID  string
	OperationFolderID string
	State             TaskState
}

type CreateImageFromSnapshotState struct {
	SrcSnapshotID     string
	DstImageID        string
	FolderID          string
	OperationCloudID  string
	OperationFolderID string
	State             TaskState
}

type CreateImageFromDiskState struct {
	SrcDisk             *types.Disk
	SrcDiskCheckpointID string
	DstImageID          string
	FolderID            string
	OperationCloudID    string
	OperationFolderID   string
	State               TaskState
}

type CreateSnapshotFromDiskState struct {
	SrcDisk                 *types.Disk
	SrcDiskCheckpointID     string
	SrcDiskBaseCheckpointID string
	DstSnapshotID           string
	DstBaseSnapshotID       string
	FolderID                string
	OperationCloudID        string
	OperationFolderID       string
	State                   TaskState
}

type TransferFromImageToDiskState struct {
	SrcImageID        string
	DstDisk           *types.Disk
	OperationCloudID  string
	OperationFolderID string
	State             TaskState
}

type TransferFromSnapshotToDiskState struct {
	SrcSnapshotID     string
	DstDisk           *types.Disk
	OperationCloudID  string
	OperationFolderID string
	State             TaskState
}

type DeleteImageState struct {
	ImageID           string
	OperationCloudID  string
	OperationFolderID string
	State             TaskState
}

type DeleteSnapshotState struct {
	SnapshotID        string
	OperationCloudID  string
	OperationFolderID string
	State             TaskState
}

type TaskStatus struct {
	Finished bool
	Progress float64
	Offset   int64
	Error    error
}

type SaveStateFunc = func(offset int64, progress float64) error

////////////////////////////////////////////////////////////////////////////////

type Client interface {
	Close() error

	CreateImageFromURL(
		ctx context.Context,
		state CreateImageFromURLState,
		saveState SaveStateFunc,
	) (ResourceInfo, error)

	CreateImageFromImage(
		ctx context.Context,
		state CreateImageFromImageState,
		saveState SaveStateFunc,
	) (ResourceInfo, error)

	CreateImageFromSnapshot(
		ctx context.Context,
		state CreateImageFromSnapshotState,
		saveState SaveStateFunc,
	) (ResourceInfo, error)

	CreateImageFromDisk(
		ctx context.Context,
		state CreateImageFromDiskState,
		saveState SaveStateFunc,
	) (ResourceInfo, error)

	CreateSnapshotFromDisk(
		ctx context.Context,
		state CreateSnapshotFromDiskState,
		saveState SaveStateFunc,
	) (ResourceInfo, error)

	TransferFromImageToDisk(
		ctx context.Context,
		state TransferFromImageToDiskState,
		saveState SaveStateFunc,
	) error

	TransferFromSnapshotToDisk(
		ctx context.Context,
		state TransferFromSnapshotToDiskState,
		saveState SaveStateFunc,
	) error

	DeleteImage(
		ctx context.Context,
		state DeleteImageState,
		saveState SaveStateFunc,
	) error

	DeleteSnapshot(
		ctx context.Context,
		state DeleteSnapshotState,
		saveState SaveStateFunc,
	) error

	CheckResourceReady(
		ctx context.Context,
		resourceID string,
	) (ResourceInfo, error)

	// Used in tests.
	DeleteTask(ctx context.Context, taskID string) error
}

////////////////////////////////////////////////////////////////////////////////

type Factory interface {
	CreateClient(ctx context.Context) (Client, error)
	CreateClientFromZone(ctx context.Context, zoneID string) (Client, error)
}
