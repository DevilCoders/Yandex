package storage

import (
	"golang.org/x/net/context"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
)

// UpdateProgressProbability is a probability that writing a snapshot chunk will update progress.
var UpdateProgressProbability = 1.0 / 16

// StatusUpdate is a manual status update.
type StatusUpdate struct {
	State string
	Desc  string
}

// SnapshotInternal is a set of internal snapshot fields.
type SnapshotInternal struct {
	Checksum  string
	ChunkSize int64
	Tree      string
}

// SnapshotFull is a union of internal and external snapshot fields.
type SnapshotFull struct {
	common.SnapshotInfo
	SnapshotInternal
}

type LockHolder interface {
	Close(ctx context.Context)
}

// Storage is a snapshot and chunks storage abstraction.
type Storage interface {
	BeginSnapshot(ctx context.Context, ci *common.CreationInfo) (id string, err error)
	BeginChunk(ctx context.Context, id string, offset int64) error
	StoreChunkData(ctx context.Context, snapshotID string, offset int64, data []byte) (err error)
	StoreChunksMetadata(ctx context.Context, snapshotID string, chunks []LibChunk) (err error)
	EndChunk(ctx context.Context, id string, chunk *LibChunk, offset int64, data []byte, progress float64) error
	EndSnapshot(ctx context.Context, id string) error

	BeginDeleteSnapshot(ctx context.Context, id string) error
	EndDeleteSnapshot(ctx context.Context, id string, forceLock bool) error
	EndDeleteSnapshotSync(ctx context.Context, id string, forceLock bool) error
	// use deleteNotOwned only if deleting failed snapshot, because it does not consider any child snapshots
	ClearSnapshotData(ctx context.Context, id string) error
	DeleteTombstone(ctx context.Context, id string) error

	GetLiveSnapshot(ctx context.Context, id string) (*common.SnapshotInfo, error)
	GetSnapshot(ctx context.Context, id string) (*common.SnapshotInfo, error)
	GetSnapshotInternal(ctx context.Context, id string) (*SnapshotInternal, error)
	GetSnapshotFull(ctx context.Context, id string) (*SnapshotFull, error)
	IsSnapshotCleared(ctx context.Context, id string) (bool, error)

	GetChunksFromCache(ctx context.Context, id string, drop bool) (ChunkMap, error)
	// ReadChunkBody returns chunk = nil if chunk is not in storage
	ReadChunkBody(ctx context.Context, id string, offset int64) (chunk *LibChunk, blob []byte, err error)
	ReadBlob(ctx context.Context, chunk *LibChunk, mustExist bool) (blob []byte, err error)

	List(ctx context.Context, r *ListRequest) ([]common.SnapshotInfo, error)
	ListGC(ctx context.Context, r *GCListRequest) ([]common.SnapshotInfo, error)
	ListBases(ctx context.Context, r *BaseListRequest) ([]common.SnapshotInfo, error)
	ListChangedChildren(ctx context.Context, id string) ([]common.ChangedChild, error)

	UpdateSnapshotStatus(ctx context.Context, id string, status StatusUpdate) error
	UpdateSnapshot(ctx context.Context, r *common.UpdateRequest) error
	UpdateSnapshotChecksum(ctx context.Context, id string, checksum string) error

	CopyChunkRefs(ctx context.Context, id string, tree string, queue []ChunkRef) error

	// lockedCTX must be used as context during locked operation
	// lockedCTX cancellation means that lock is unlocked now and it is invalid to perform operation
	// if so operation must cancel itself
	// holder.Close() must be called to release the lock
	LockSnapshot(ctx context.Context, lockID string, reason string) (lockedCTX context.Context, holder LockHolder, err error)
	LockSnapshotShared(ctx context.Context, lockID string, reason string) (lockedCTX context.Context, holder LockHolder, resErr error)

	// Allow max maxActiveLocks shared locks per lockID, return misc.ErrMaxShareLockExceeded if max count exceeded
	LockSnapshotSharedMax(ctx context.Context, lockID string, reason string, maxActiveLocks int) (lockedCTX context.Context, holder LockHolder, resErr error)
	LockSnapshotForce(ctx context.Context, lockID string, reason string) (lockedCTX context.Context, holder LockHolder, resErr error)

	PingContext(ctx context.Context) error
	Health(ctx context.Context) error
	Close() error
}
