package nbs

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type CreateDiskParams struct {
	ID                   string
	BaseDiskID           string
	BaseDiskCheckpointID string
	BlocksCount          uint64
	// When 0, uses DefaultBlockSize from config.
	BlockSize             uint32
	Kind                  types.DiskKind
	CloudID               string
	FolderID              string
	TabletVersion         uint32
	PlacementGroupID      string
	PartitionsCount       uint32
	IsSystem              bool
	EncryptionMode        types.EncryptionMode
	EncryptionKeyFilePath string
	StoragePoolName       string
	AgentIds              []string
}

type AssignDiskParams struct {
	ID         string
	InstanceID string
	Token      string
	Host       string
}

type DiskPerformanceProfile struct {
	MaxReadBandwidth   uint32
	MaxPostponedWeight uint32
	ThrottlingEnabled  bool
	MaxReadIops        uint32
	BoostTime          uint32
	BoostRefillTime    uint32
	BoostPercentage    uint32
	MaxWriteBandwidth  uint32
	MaxWriteIops       uint32
	BurstPercentage    uint32
}

type DiskModel struct {
	BlockSize           uint32
	BlocksCount         uint64
	ChannelsCount       uint32
	Kind                types.DiskKind
	PerformanceProfile  DiskPerformanceProfile
	MergedChannelsCount uint32
	MixedChannelsCount  uint32
}

type DiskParams struct {
	BlockSize   uint32
	BlocksCount uint64
}

type PlacementGroup struct {
	GroupID           string
	PlacementStrategy types.PlacementStrategy
	DiskIDs           []string
	Racks             []string
}

type DiskStats struct {
	// In bytes.
	StorageSize uint64
}

////////////////////////////////////////////////////////////////////////////////

type Client interface {
	Create(ctx context.Context, params CreateDiskParams) error

	Delete(ctx context.Context, diskID string) error

	CreateCheckpoint(
		ctx context.Context,
		diskID string,
		checkpointID string,
	) error

	DeleteCheckpoint(
		ctx context.Context,
		diskID string,
		checkpointID string,
	) error

	DeleteCheckpointData(
		ctx context.Context,
		diskID string,
		checkpointID string,
	) error

	Resize(
		ctx context.Context,
		saveState func() error,
		diskID string,
		size uint64,
	) error

	Alter(
		ctx context.Context,
		saveState func() error,
		diskID string, cloudID string,
		folderID string,
	) error

	Rebase(
		ctx context.Context,
		saveState func() error,
		diskID string,
		baseDiskID string,
		targetBaseDiskID string,
	) error

	Assign(ctx context.Context, params AssignDiskParams) error

	Unassign(ctx context.Context, diskID string) error

	DescribeModel(
		ctx context.Context,
		blocksCount uint64,
		blockSize uint32,
		kind types.DiskKind,
		tabletVersion uint32,
	) (DiskModel, error)

	Describe(
		ctx context.Context,
		diskID string,
	) (DiskParams, error)

	CreatePlacementGroup(
		ctx context.Context,
		groupID string,
		placementStrategy types.PlacementStrategy,
	) error

	DeletePlacementGroup(ctx context.Context, groupID string) error

	AlterPlacementGroupMembership(
		ctx context.Context,
		saveState func() error,
		groupID string,
		disksToAdd []string,
		disksToRemove []string,
	) error

	ListPlacementGroups(ctx context.Context) ([]string, error)

	DescribePlacementGroup(
		ctx context.Context,
		groupID string,
	) (PlacementGroup, error)

	MountRO(ctx context.Context, diskID string) (Session, error)

	MountRW(ctx context.Context, diskID string) (Session, error)

	GetChangedBlocks(
		ctx context.Context,
		diskID string,
		startIndex uint64,
		blockCount uint32,
		baseCheckpointID string,
		checkpointID string,
	) ([]byte, error)

	GetCheckpointSize(
		ctx context.Context,
		saveState func(blockIndex uint64, checkpointSize uint64) error,
		diskID string,
		checkpointID string,
		milestoneBlockIndex uint64,
		milestoneCheckpointSize uint64,
	) error

	Stat(ctx context.Context, diskID string) (DiskStats, error)

	// Used in tests.
	ValidateCrc32(diskID string, contentSize uint64, expectedCrc32 uint32) error

	// Used in tests.
	MountForReadWrite(diskID string) (func(), error)

	// Used in tests.
	Write(diskID string, startIndex int, bytes []byte) error

	// Used in tests.
	GetCheckpoints(ctx context.Context, diskID string) ([]string, error)
}

////////////////////////////////////////////////////////////////////////////////

type Factory interface {
	HasClient(zoneID string) bool

	GetClient(ctx context.Context, zoneID string) (Client, error)

	// Returns client from default zone. Use it carefully.
	GetClientFromDefaultZone(ctx context.Context) (Client, error)
}
