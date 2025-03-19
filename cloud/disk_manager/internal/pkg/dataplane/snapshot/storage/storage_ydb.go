package storage

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/chunks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
)

////////////////////////////////////////////////////////////////////////////////

const (
	// TODO: move to config.
	saveProgressPeriod = time.Second
)

////////////////////////////////////////////////////////////////////////////////

type storageYDB struct {
	db                       *persistence.YDBClient
	tablesPath               string
	metrics                  metrics.Metrics
	deleteWorkerCount        int
	shallowCopyWorkerCount   int
	shallowCopyInflightLimit int
	chunkCompression         string
	chunkStorageS3           *chunks.StorageS3
	chunkStorageYDB          *chunks.StorageYDB
}

func (s *storageYDB) CreateSnapshot(
	ctx context.Context,
	snapshotID string,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.createSnapshot(
				ctx,
				session,
				snapshotID,
			)
		},
	)
	if err != nil {
		logging.Error(ctx, "CreateSnapshot failed: %v", err)
	}

	return err
}

func (s *storageYDB) SnapshotCreated(
	ctx context.Context,
	snapshotID string,
	size uint64,
	storageSize uint64,
	chunkCount uint32,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.snapshotCreated(
				ctx,
				session,
				snapshotID,
				size,
				storageSize,
				chunkCount,
			)
		},
	)
	if err != nil {
		logging.Error(ctx, "SnapshotCreated failed: %v", err)
	}

	return err
}

func (s *storageYDB) DeletingSnapshot(
	ctx context.Context,
	snapshotID string,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.deletingSnapshot(
				ctx,
				session,
				snapshotID,
			)
		},
	)
	if err != nil {
		logging.Error(ctx, "DeletingSnapshot failed: %v", err)
	}

	return err
}

func (s *storageYDB) GetSnapshotsToDelete(
	ctx context.Context,
	deletingBefore time.Time,
	limit int,
) ([]*protos.DeletingSnapshotKey, error) {

	keys, err := s.getSnapshotsToDelete(
		ctx,
		deletingBefore,
		limit,
	)
	if err != nil {
		logging.Error(ctx, "GetSnapshotsToDelete failed: %v", err)
	}

	return keys, err
}

func (s *storageYDB) DeleteSnapshotData(
	ctx context.Context,
	snapshotID string,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.deleteSnapshotData(
				ctx,
				session,
				snapshotID,
			)
		},
	)
	if err != nil {
		logging.Error(ctx, "DeleteSnapshotData failed: %v", err)
	}

	return err
}

func (s *storageYDB) ClearDeletingSnapshots(
	ctx context.Context,
	keys []*protos.DeletingSnapshotKey,
) error {

	err := s.clearDeletingSnapshots(ctx, keys)
	if err != nil {
		logging.Error(ctx, "ClearDeletingSnapshots failed: %v", err)
	}

	return err
}

func (s *storageYDB) ShallowCopySnapshot(
	ctx context.Context,
	srcSnapshotID string,
	dstSnapshotID string,
	milestoneChunkIndex uint32,
	saveProgress func(context.Context, uint32) error,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.shallowCopySnapshot(
				ctx,
				session,
				srcSnapshotID,
				dstSnapshotID,
				milestoneChunkIndex,
				saveProgress,
			)
		},
	)
	if err != nil {
		logging.Error(ctx, "ShallowCopySnapshot failed: %v", err)
	}

	return err
}

func (s *storageYDB) WriteChunk(
	ctx context.Context,
	uniqueID string,
	snapshotID string,
	chunk common.Chunk,
	useS3 bool,
) (string, error) {

	chunkID, err := s.writeChunk(ctx, uniqueID, snapshotID, chunk, useS3)
	if err != nil {
		logging.Error(ctx, "WriteChunk failed: %v", err)
	}

	return chunkID, err
}

func (s *storageYDB) RewriteChunk(
	ctx context.Context,
	uniqueID string,
	snapshotID string,
	chunk common.Chunk,
	useS3 bool,
) (string, error) {

	chunkID, err := s.rewriteChunk(ctx, uniqueID, snapshotID, chunk, useS3)
	if err != nil {
		logging.Error(ctx, "RewriteChunk failed: %v", err)
	}

	return chunkID, err
}

func (s *storageYDB) ReadChunkMap(
	ctx context.Context,
	snapshotID string,
	milestoneChunkIndex uint32,
) (<-chan ChunkMapEntry, <-chan error) {

	var entries <-chan ChunkMapEntry
	var errors <-chan error

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			entries, errors = s.readChunkMap(
				ctx,
				session,
				snapshotID,
				milestoneChunkIndex,
				nil,
			)
			return nil
		},
	)
	if err != nil {
		logging.Error(ctx, "ReadChunkMap failed: %v", err)

		entries := make(chan ChunkMapEntry)
		errors := make(chan error, 1)
		errors <- err
		close(entries)
		close(errors)
		return entries, errors
	}

	return entries, errors
}

func (s *storageYDB) ReadChunk(
	ctx context.Context,
	chunk *common.Chunk,
) error {

	err := s.readChunk(ctx, chunk)
	if err != nil {
		logging.Error(ctx, "ReadChunk failed: %v", err)
	}

	return err
}

func (s *storageYDB) CheckSnapshotReady(
	ctx context.Context,
	snapshotID string,
) (SnapshotMeta, error) {

	meta, err := s.checkSnapshotReady(ctx, snapshotID)
	if err != nil {
		logging.Error(ctx, "CheckSnapshotReady failed: %v", err)
	}

	return meta, err
}

func (s *storageYDB) CheckSnapshotAlive(
	ctx context.Context,
	snapshotID string,
) error {

	err := s.checkSnapshotAlive(ctx, snapshotID)
	if err != nil {
		logging.Error(ctx, "CheckSnapshotAlive failed: %v", err)
	}

	return err
}

func (s *storageYDB) GetDataChunkCount(
	ctx context.Context,
	snapshotID string,
) (uint64, error) {

	dataChunkCount, err := s.getDataChunkCount(ctx, snapshotID)
	if err != nil {
		logging.Error(ctx, "GetDataChunkCount failed: %v", err)
	}

	return dataChunkCount, err
}
