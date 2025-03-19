package storage

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/protos"
)

////////////////////////////////////////////////////////////////////////////////

type SnapshotMeta struct {
	// Snapshot virtual size, i.e. the minimum amount of disk space needed to restore.
	Size uint64
	// Snapshot real size, i.e. the amount of disk space occupied in storage.
	StorageSize uint64

	ChunkCount uint32
}

////////////////////////////////////////////////////////////////////////////////

type ChunkMapEntry struct {
	ChunkIndex uint32
	ChunkID    string
	StoredInS3 bool
}

////////////////////////////////////////////////////////////////////////////////

type Storage interface {
	CreateSnapshot(ctx context.Context, snapshotID string) error

	SnapshotCreated(
		ctx context.Context,
		snapshotID string,
		size uint64,
		storageSize uint64,
		chunkCount uint32,
	) error

	DeletingSnapshot(ctx context.Context, snapshotID string) error

	GetSnapshotsToDelete(
		ctx context.Context,
		deletingBefore time.Time,
		limit int,
	) ([]*protos.DeletingSnapshotKey, error)

	DeleteSnapshotData(ctx context.Context, snapshotID string) error

	ClearDeletingSnapshots(
		ctx context.Context,
		keys []*protos.DeletingSnapshotKey,
	) error

	ShallowCopySnapshot(
		ctx context.Context,
		srcSnapshotID string,
		dstSnapshotID string,
		milestoneChunkIndex uint32,
		saveProgress func(context.Context, uint32) error,
	) error

	WriteChunk(
		ctx context.Context,
		uniqueID string,
		snapshotID string,
		chunk common.Chunk,
		useS3 bool,
	) (string, error)

	RewriteChunk(
		ctx context.Context,
		uniqueID string,
		snapshotID string,
		chunk common.Chunk,
		useS3 bool,
	) (string, error)

	ReadChunkMap(
		ctx context.Context,
		snapshotID string,
		milestoneChunkIndex uint32,
	) (<-chan ChunkMapEntry, <-chan error)

	ReadChunk(ctx context.Context, chunk *common.Chunk) error

	CheckSnapshotReady(ctx context.Context, snapshotID string) (SnapshotMeta, error)

	CheckSnapshotAlive(ctx context.Context, snapshotID string) error

	// Returns number of non-zero chunks.
	GetDataChunkCount(ctx context.Context, snapshotID string) (uint64, error)
}
