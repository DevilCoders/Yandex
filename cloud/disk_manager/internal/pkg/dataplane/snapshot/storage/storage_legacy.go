package storage

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	task_errors "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type legacyStorage struct {
	db         *persistence.YDBClient
	tablesPath string
	metrics    metrics.Metrics
}

////////////////////////////////////////////////////////////////////////////////

func (s *legacyStorage) ReadChunkMap(
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

func (s *legacyStorage) ReadChunk(
	ctx context.Context,
	chunk *common.Chunk,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.readChunk(ctx, session, chunk)
		},
	)
	if err != nil {
		logging.Error(ctx, "ReadChunk failed: %v", err)
	}

	return err
}

func (s *legacyStorage) CheckSnapshotReady(
	ctx context.Context,
	snapshotID string,
) (SnapshotMeta, error) {

	var info SnapshotMeta

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			info, err = s.checkSnapshotReady(
				ctx,
				session,
				snapshotID,
			)
			return err
		},
	)
	if err != nil {
		logging.Error(ctx, "CheckSnapshotReady failed: %v", err)
	}

	return info, err
}

////////////////////////////////////////////////////////////////////////////////

func (s *legacyStorage) CreateSnapshot(
	ctx context.Context,
	snapshotID string,
) error {

	return &task_errors.NonRetriableError{
		Err: fmt.Errorf("not implemented"),
	}
}

func (s *legacyStorage) SnapshotCreated(
	ctx context.Context,
	snapshotID string,
	size uint64,
	storageSize uint64,
	chunkCount uint32,
) error {

	return &task_errors.NonRetriableError{
		Err: fmt.Errorf("not implemented"),
	}
}

func (s *legacyStorage) DeletingSnapshot(
	ctx context.Context,
	snapshotID string,
) error {

	return &task_errors.NonRetriableError{
		Err: fmt.Errorf("not implemented"),
	}
}

func (s *legacyStorage) GetSnapshotsToDelete(
	ctx context.Context,
	deletingBefore time.Time,
	limit int,
) ([]*protos.DeletingSnapshotKey, error) {

	return nil, &task_errors.NonRetriableError{
		Err: fmt.Errorf("not implemented"),
	}
}

func (s *legacyStorage) DeleteSnapshotData(
	ctx context.Context,
	snapshotID string,
) error {

	return &task_errors.NonRetriableError{
		Err: fmt.Errorf("not implemented"),
	}
}

func (s *legacyStorage) ClearDeletingSnapshots(
	ctx context.Context,
	keys []*protos.DeletingSnapshotKey,
) error {

	return &task_errors.NonRetriableError{
		Err: fmt.Errorf("not implemented"),
	}
}

func (s *legacyStorage) ShallowCopySnapshot(
	ctx context.Context,
	srcSnapshotID string,
	dstSnapshotID string,
	milestoneChunkIndex uint32,
	saveProgress func(context.Context, uint32) error,
) error {

	return &task_errors.NonRetriableError{
		Err: fmt.Errorf("not implemented"),
	}
}

func (s *legacyStorage) WriteChunk(
	ctx context.Context,
	uniqueID string,
	snapshotID string,
	chunk common.Chunk,
	useS3 bool,
) (string, error) {

	return "", &task_errors.NonRetriableError{
		Err: fmt.Errorf("not implemented"),
	}
}

func (s *legacyStorage) RewriteChunk(
	ctx context.Context,
	uniqueID string,
	snapshotID string,
	chunk common.Chunk,
	useS3 bool,
) (string, error) {

	return "", &task_errors.NonRetriableError{
		Err: fmt.Errorf("not implemented"),
	}
}

func (s *legacyStorage) CheckSnapshotAlive(
	ctx context.Context,
	snapshotID string,
) error {

	return &task_errors.NonRetriableError{
		Err: fmt.Errorf("not implemented"),
	}
}

func (s *legacyStorage) GetDataChunkCount(
	ctx context.Context,
	snapshotID string,
) (uint64, error) {

	return 0, &task_errors.NonRetriableError{
		Err: fmt.Errorf("not implemented"),
	}
}
