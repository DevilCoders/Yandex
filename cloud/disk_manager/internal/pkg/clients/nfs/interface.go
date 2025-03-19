package nfs

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type CreateFilesystemParams struct {
	FolderID    string
	CloudID     string
	BlockSize   uint32
	BlocksCount uint64
	StorageKind types.FilesystemStorageKind
}

type FilesystemPerformanceProfile struct {
	MaxReadBandwidth  uint32
	MaxReadIops       uint32
	MaxWriteBandwidth uint32
	MaxWriteIops      uint32
}

type FilesystemModel struct {
	BlockSize          uint32
	BlocksCount        uint64
	ChannelsCount      uint32
	Kind               types.FilesystemStorageKind
	PerformanceProfile FilesystemPerformanceProfile
}

////////////////////////////////////////////////////////////////////////////////

type Client interface {
	Close() error

	Create(
		ctx context.Context,
		filesystemID string,
		params CreateFilesystemParams,
	) error

	Delete(
		ctx context.Context,
		filesystemID string,
	) error

	Resize(
		ctx context.Context,
		filesystemID string,
		size uint64,
	) error

	DescribeModel(
		ctx context.Context,
		blocksCount uint64,
		blockSize uint32,
		kind types.FilesystemStorageKind,
	) (FilesystemModel, error)
}

////////////////////////////////////////////////////////////////////////////////

type Factory interface {
	CreateClient(ctx context.Context, zoneID string) (Client, error)

	// Returns client from default zone. Use it carefully.
	CreateClientFromDefaultZone(ctx context.Context) (Client, error)
}
