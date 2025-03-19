package storage

import (
	"context"
	"fmt"
	"path"
	"sync"
	"time"

	ydb_table "github.com/ydb-platform/ydb-go-sdk/v3/table"
	ydb_options "github.com/ydb-platform/ydb-go-sdk/v3/table/options"
	ydb_named "github.com/ydb-platform/ydb-go-sdk/v3/table/result/named"
	ydb_types "github.com/ydb-platform/ydb-go-sdk/v3/table/types"
	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/common"
	dataplane_common "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/chunks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	task_errors "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/rtc/mediator/cityhash"
)

////////////////////////////////////////////////////////////////////////////////

func makeChunkID(
	uniqueID string,
	snapshotID string,
	chunk dataplane_common.Chunk,
) string {

	return fmt.Sprintf("%v.%v.%v", uniqueID, snapshotID, chunk.Index)
}

func makeShardID(s string) uint64 {
	return cityhash.Hash64([]byte(s))
}

////////////////////////////////////////////////////////////////////////////////

func (s *storageYDB) createSnapshot(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
) (err error) {

	defer s.metrics.StatOperation("createSnapshot")(&err)

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from snapshots
		where id = $id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(snapshotID)),
	))
	if err != nil {
		return err
	}
	defer res.Close()

	states, err := scanSnapshotStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}

		return err
	}

	if len(states) != 0 {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		state := states[0]

		if state.status >= snapshotStatusDeleting {
			return &task_errors.NonRetriableError{
				Err: fmt.Errorf(
					"can't create already deleting snapshot with id=%v",
					snapshotID,
				),
				Silent: true,
			}
		}

		// Should be idempotent.
		return nil
	}

	state := snapshotState{
		id:         snapshotID,
		creatingAt: time.Now(),
		status:     snapshotStatusCreating,
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $states as List<%v>;

		upsert into snapshots
		select *
		from AS_TABLE($states)
	`, s.tablesPath, snapshotStateStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$states", ydb_types.ListValue(state.structValue())),
	))
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) snapshotCreated(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
	size uint64,
	storageSize uint64,
	chunkCount uint32,
) (err error) {

	defer s.metrics.StatOperation("snapshotCreated")(&err)

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from snapshots
		where id = $id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(snapshotID)),
	))
	if err != nil {
		return err
	}
	defer res.Close()

	states, err := scanSnapshotStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}

		return err
	}

	if len(states) == 0 {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &task_errors.NonRetriableError{
			Err: fmt.Errorf("snapshot with id=%v is not found", snapshotID),
		}
	}

	state := states[0]

	if state.status == snapshotStatusReady {
		// Should be idempotent.
		return tx.Commit(ctx)
	}

	if state.status != snapshotStatusCreating {
		return &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"snapshot with id=%v and status=%v can't be created",
				snapshotID,
				snapshotStatusToString(state.status),
			),
			Silent: true,
		}
	}

	state.status = snapshotStatusReady
	state.createdAt = time.Now()
	state.size = size
	state.storageSize = storageSize
	state.chunkCount = chunkCount

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $states as List<%v>;

		upsert into snapshots
		select *
		from AS_TABLE($states)
	`, s.tablesPath, snapshotStateStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$states", ydb_types.ListValue(state.structValue())),
	))
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) deletingSnapshot(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
) (err error) {

	defer s.metrics.StatOperation("deletingSnapshot")(&err)

	deletingAt := time.Now()

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from snapshots
		where id = $id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(snapshotID)),
	))
	if err != nil {
		return err
	}
	defer res.Close()

	states, err := scanSnapshotStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}

		return err
	}

	var state snapshotState

	if len(states) != 0 {
		state = states[0]

		if state.status >= snapshotStatusDeleting {
			// Snapshot already marked as deleting.

			err = tx.Commit(ctx)
			if err != nil {
				return err
			}

			// Should be idempotent.
			return nil
		}
	}

	state.id = snapshotID
	state.status = snapshotStatusDeleting
	state.deletingAt = deletingAt

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $states as List<%v>;

		upsert into snapshots
		select *
		from AS_TABLE($states)
	`, s.tablesPath, snapshotStateStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$states", ydb_types.ListValue(state.structValue())),
	))
	if err != nil {
		return err
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $deleting_at as Timestamp;
		declare $snapshot_id as Utf8;

		upsert into deleting (deleting_at, snapshot_id)
		values ($deleting_at, $snapshot_id)
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$deleting_at", persistence.TimestampValue(deletingAt)),
		ydb_table.ValueParam("$snapshot_id", ydb_types.UTF8Value(snapshotID)),
	))
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) getSnapshotsToDelete(
	ctx context.Context,
	deletingBefore time.Time,
	limit int,
) (keys []*protos.DeletingSnapshotKey, err error) {

	defer s.metrics.StatOperation("getSnapshotsToDelete")(&err)

	res, err := s.db.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $deleting_before as Timestamp;
		declare $limit as Uint64;

		select *
		from deleting
		where deleting_at < $deleting_before
		limit $limit
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$deleting_before", persistence.TimestampValue(deletingBefore)),
		ydb_table.ValueParam("$limit", ydb_types.Uint64Value(uint64(limit))),
	))
	if err != nil {
		return nil, err
	}
	defer res.Close()

	for res.NextResultSet(ctx) {
		for res.NextRow() {
			key := &protos.DeletingSnapshotKey{}
			var deletingAt time.Time
			err = res.ScanNamed(
				ydb_named.OptionalWithDefault("deleting_at", &deletingAt),
				ydb_named.OptionalWithDefault("snapshot_id", &key.SnapshotId),
			)
			if err != nil {
				return nil, &task_errors.NonRetriableError{
					Err: fmt.Errorf("getSnapshotsToDelete: failed to parse row: %w", err),
				}
			}
			key.DeletingAt = timestamppb.New(deletingAt)
			keys = append(keys, key)
		}
	}

	return keys, res.Err()
}

func (s *storageYDB) deleteSnapshotData(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
) error {

	entries, errors := s.readChunkMap(ctx, session, snapshotID, 0, nil)

	err := s.processChunkMapEntries(
		ctx,
		entries,
		s.deleteWorkerCount,
		func(ctx context.Context, entry ChunkMapEntry) error {
			return s.deleteChunk(ctx, snapshotID, entry)
		},
	)
	if err != nil {
		return err
	}

	return <-errors
}

func (s *storageYDB) shallowCopyChunk(
	ctx context.Context,
	snapshotID string,
	entry ChunkMapEntry,
) (err error) {

	defer s.metrics.StatOperation("shallowCopyChunk")(&err)

	// First, create new chunk map entry. It is safe to create chunk map entry
	// before updating chunk blob's ref count because whole snapshot is not
	// ready yet. We do this to avoid orphaning blobs.
	// This operation is idempotent.
	_, err = s.db.ExecuteRW(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $shard_id as Uint64;
		declare $snapshot_id as Utf8;
		declare $chunk_index as Uint32;
		declare $chunk_id as Utf8;
		declare $stored_in_s3 as Bool;

		upsert into chunk_map (shard_id, snapshot_id, chunk_index, chunk_id, stored_in_s3)
		values ($shard_id, $snapshot_id, $chunk_index, $chunk_id, $stored_in_s3);
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$shard_id", ydb_types.Uint64Value(makeShardID(snapshotID))),
		ydb_table.ValueParam("$snapshot_id", ydb_types.UTF8Value(snapshotID)),
		ydb_table.ValueParam("$chunk_index", ydb_types.Uint32Value(entry.ChunkIndex)),
		ydb_table.ValueParam("$chunk_id", ydb_types.UTF8Value(entry.ChunkID)),
		ydb_table.ValueParam("$stored_in_s3", ydb_types.BoolValue(entry.StoredInS3)),
	))
	if err != nil {
		return err
	}

	if len(entry.ChunkID) == 0 {
		return nil
	}

	chunkStorage := s.getChunkStorage(entry.StoredInS3)
	return chunkStorage.RefChunk(ctx, snapshotID, entry.ChunkID)
}

func (s *storageYDB) deleteChunk(
	ctx context.Context,
	snapshotID string,
	entry ChunkMapEntry,
) (err error) {

	defer s.metrics.StatOperation("deleteChunk")(&err)

	// First, update chunk blob's ref count. We do this before deleting chunk
	// map entry to avoid orphaning blobs.
	if len(entry.ChunkID) != 0 {
		chunkStorage := s.getChunkStorage(entry.StoredInS3)
		err := chunkStorage.UnrefChunk(ctx, snapshotID, entry.ChunkID)
		if err != nil {
			return err
		}
	}

	// Second, delete chunk map entry.
	// This operation is idempotent.
	_, err = s.db.ExecuteRW(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $shard_id as Uint64;
		declare $snapshot_id as Utf8;
		declare $chunk_index as Uint32;

		delete from chunk_map
		where shard_id = $shard_id and
			snapshot_id = $snapshot_id and
			chunk_index = $chunk_index;
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$shard_id", ydb_types.Uint64Value(makeShardID(snapshotID))),
		ydb_table.ValueParam("$snapshot_id", ydb_types.UTF8Value(snapshotID)),
		ydb_table.ValueParam("$chunk_index", ydb_types.Uint32Value(entry.ChunkIndex)),
	))

	return err
}

func (s *storageYDB) clearDeletingSnapshots(
	ctx context.Context,
	keys []*protos.DeletingSnapshotKey,
) (err error) {

	defer s.metrics.StatOperation("clearDeletingSnapshots")(&err)

	for _, key := range keys {
		_, err := s.db.ExecuteRW(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $deleting_at as Timestamp;
			declare $snapshot_id as Utf8;
			declare $status as Int64;

			delete from snapshots
			where id = $snapshot_id and status = $status;

			delete from deleting
			where deleting_at = $deleting_at and snapshot_id = $snapshot_id
		`, s.tablesPath), ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$deleting_at", persistence.TimestampValue(key.DeletingAt.AsTime())),
			ydb_table.ValueParam("$snapshot_id", ydb_types.UTF8Value(key.SnapshotId)),
			ydb_table.ValueParam("$status", ydb_types.Int64Value(int64(snapshotStatusDeleting))),
		))
		if err != nil {
			return err
		}
	}

	return nil
}

func (s *storageYDB) shallowCopySnapshot(
	ctx context.Context,
	session *persistence.Session,
	srcSnapshotID string,
	dstSnapshotID string,
	milestoneChunkIndex uint32,
	saveProgress func(context.Context, uint32) error,
) error {

	processedIndices := make(chan uint32, s.shallowCopyInflightLimit)
	inflightQueue := common.CreateInflightQueue(milestoneChunkIndex, processedIndices)

	var updaterError <-chan error

	if saveProgress != nil {
		var waitUpdater func()
		waitUpdater, updaterError = common.ProgressUpdater(
			ctx,
			saveProgressPeriod,
			func(ctx context.Context) error {
				return saveProgress(ctx, inflightQueue.GetMilestone())
			},
		)
		defer waitUpdater()
	}

	entries, errors := s.readChunkMap(
		ctx,
		session,
		srcSnapshotID,
		milestoneChunkIndex,
		inflightQueue,
	)

	err := s.processChunkMapEntries(
		ctx,
		entries,
		s.shallowCopyWorkerCount,
		func(ctx context.Context, entry ChunkMapEntry) error {
			err := s.shallowCopyChunk(ctx, dstSnapshotID, entry)
			if err != nil {
				return err
			}

			select {
			case processedIndices <- entry.ChunkIndex:
			case <-ctx.Done():
				return ctx.Err()
			case err := <-updaterError:
				return err
			}

			return nil
		},
	)
	if err != nil {
		return err
	}

	return <-errors
}

func (s *storageYDB) writeChunk(
	ctx context.Context,
	uniqueID string,
	snapshotID string,
	chunk dataplane_common.Chunk,
	useS3 bool,
) (chunkID string, err error) {

	if chunk.Zero {
		return s.writeZeroChunk(ctx, snapshotID, chunk)
	} else {
		return s.writeDataChunk(ctx, uniqueID, snapshotID, chunk, useS3)
	}
}

func (s *storageYDB) rewriteChunk(
	ctx context.Context,
	uniqueID string,
	snapshotID string,
	chunk dataplane_common.Chunk,
	useS3 bool,
) (chunkID string, err error) {

	// Unref existing chunk blob before writting new one.
	res, err := s.db.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $shard_id as Uint64;
		declare $snapshot_id as Utf8;
		declare $chunk_index as Uint32;

		select chunk_id, stored_in_s3
		from chunk_map
		where shard_id = $shard_id and snapshot_id = $snapshot_id and chunk_index = $chunk_index
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$shard_id", ydb_types.Uint64Value(makeShardID(snapshotID))),
		ydb_table.ValueParam("$snapshot_id", ydb_types.UTF8Value(snapshotID)),
		ydb_table.ValueParam("$chunk_index", ydb_types.Uint32Value(chunk.Index)),
	))
	if err != nil {
		return "", err
	}
	defer res.Close()

	for res.NextResultSet(ctx) {
		for res.NextRow() {
			var storedInS3 bool
			err = res.ScanNamed(
				ydb_named.OptionalWithDefault("chunk_id", &chunkID),
				ydb_named.OptionalWithDefault("stored_in_s3", &storedInS3),
			)
			if err != nil {
				return "", &task_errors.NonRetriableError{
					Err: fmt.Errorf("rewriteChunk: failed to parse row: %w", err),
				}
			}

			if len(chunkID) == 0 {
				continue
			}

			chunkStorage := s.getChunkStorage(storedInS3)
			err := chunkStorage.UnrefChunk(ctx, snapshotID, chunkID)
			if err != nil {
				return "", err
			}
		}
	}
	if err = res.Err(); err != nil {
		return "", err
	}

	// Now write new chunk as usual.
	return s.writeChunk(ctx, uniqueID, snapshotID, chunk, useS3)
}

func (s *storageYDB) writeDataChunk(
	ctx context.Context,
	uniqueID string,
	snapshotID string,
	chunk dataplane_common.Chunk,
	useS3 bool,
) (chunkID string, err error) {

	defer s.metrics.StatOperation("writeDataChunk")(&err)

	chunkID = makeChunkID(uniqueID, snapshotID, chunk)

	// First, create chunk map entry. It is safe to create chunk map entry before
	// writing chunk blob because whole snapshot is not ready yet. We do this to
	// avoid orphaning blobs.
	// This operation is idempotent.
	_, err = s.db.ExecuteRW(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $shard_id as Uint64;
		declare $snapshot_id as Utf8;
		declare $chunk_index as Uint32;
		declare $chunk_id as Utf8;
		declare $stored_in_s3 as Bool;

		upsert into chunk_map (shard_id, snapshot_id, chunk_index, chunk_id, stored_in_s3)
		values ($shard_id, $snapshot_id, $chunk_index, $chunk_id, $stored_in_s3)
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$shard_id", ydb_types.Uint64Value(makeShardID(snapshotID))),
		ydb_table.ValueParam("$snapshot_id", ydb_types.UTF8Value(snapshotID)),
		ydb_table.ValueParam("$chunk_index", ydb_types.Uint32Value(chunk.Index)),
		ydb_table.ValueParam("$chunk_id", ydb_types.UTF8Value(chunkID)),
		ydb_table.ValueParam("$stored_in_s3", ydb_types.BoolValue(useS3)),
	))
	if err != nil {
		return "", err
	}

	chunkStorage := s.getChunkStorage(useS3)
	compressedDataLen, err := chunkStorage.WriteChunk(
		ctx,
		snapshotID,
		chunkID,
		chunk.Data,
		s.chunkCompression,
	)
	if err != nil {
		return "", err
	}

	if len(s.chunkCompression) != 0 {
		s.metrics.OnChunkCompressed(len(chunk.Data), compressedDataLen)
	}

	return chunkID, nil
}

func (s *storageYDB) writeZeroChunk(
	ctx context.Context,
	snapshotID string,
	chunk dataplane_common.Chunk,
) (chunkID string, err error) {

	defer s.metrics.StatOperation("writeZeroChunk")(&err)

	_, err = s.db.ExecuteRW(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $shard_id as Uint64;
		declare $snapshot_id as Utf8;
		declare $chunk_index as Uint32;

		upsert into chunk_map (shard_id, snapshot_id, chunk_index, chunk_id)
		values ($shard_id, $snapshot_id, $chunk_index, "");
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$shard_id", ydb_types.Uint64Value(makeShardID(snapshotID))),
		ydb_table.ValueParam("$snapshot_id", ydb_types.UTF8Value(snapshotID)),
		ydb_table.ValueParam("$chunk_index", ydb_types.Uint32Value(chunk.Index)),
	))

	return "", err
}

func (s *storageYDB) readChunkMap(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
	milestoneChunkIndex uint32,
	inflightQueue *common.InflightQueue,
) (<-chan ChunkMapEntry, <-chan error) {

	entries := make(chan ChunkMapEntry)
	errors := make(chan error, 1)

	shardID := makeShardID(snapshotID)

	res, err := session.StreamReadTable(
		ctx,
		path.Join(s.tablesPath, "chunk_map"),
		ydb_options.ReadOrdered(),
		ydb_options.ReadColumn("chunk_index"),
		ydb_options.ReadColumn("chunk_id"),
		ydb_options.ReadColumn("stored_in_s3"),
		ydb_options.ReadGreaterOrEqual(ydb_types.TupleValue(
			ydb_types.OptionalValue(ydb_types.Uint64Value(shardID)),
			ydb_types.OptionalValue(ydb_types.UTF8Value(snapshotID)),
			ydb_types.OptionalValue(ydb_types.Uint32Value(milestoneChunkIndex)),
		)),
		ydb_options.ReadLessOrEqual(ydb_types.TupleValue(
			ydb_types.OptionalValue(ydb_types.Uint64Value(shardID)),
			ydb_types.OptionalValue(ydb_types.UTF8Value(snapshotID)),
			ydb_types.OptionalValue(ydb_types.Uint32Value(^uint32(0))),
		)),
	)
	if err != nil {
		errors <- err
		close(entries)
		close(errors)
		return entries, errors
	}

	go func() {
		defer res.Close()
		defer close(entries)
		defer close(errors)

		for res.NextResultSet(ctx) {
			for res.NextRow() {
				var entry ChunkMapEntry
				err = res.ScanNamed(
					ydb_named.OptionalWithDefault("chunk_index", &entry.ChunkIndex),
					ydb_named.OptionalWithDefault("chunk_id", &entry.ChunkID),
					ydb_named.OptionalWithDefault("stored_in_s3", &entry.StoredInS3),
				)
				if err != nil {
					errors <- &task_errors.NonRetriableError{
						Err: fmt.Errorf("readChunkMap: failed to parse row: %w", err),
					}
					return
				}
				if inflightQueue != nil {
					err := inflightQueue.Add(ctx, entry.ChunkIndex)
					if err != nil {
						errors <- err
						return
					}
				}

				select {
				case entries <- entry:
				case <-ctx.Done():
					errors <- ctx.Err()
					return
				}
			}
		}

		err = res.Err()
		if err != nil {
			errors <- &task_errors.RetriableError{Err: err}
		}
	}()

	return entries, errors
}

func (s *storageYDB) processChunkMapEntries(
	ctx context.Context,
	entries <-chan ChunkMapEntry,
	workerCount int,
	process func(ctx context.Context, entry ChunkMapEntry) error,
) error {

	var wg sync.WaitGroup
	wg.Add(workerCount)
	// Should wait right after context cancelling.
	defer wg.Wait()

	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	workerErrors := make(chan error, workerCount)

	for i := 0; i < workerCount; i++ {
		go func() {
			defer wg.Done()

			var entry ChunkMapEntry
			more := true

			for more {
				select {
				case entry, more = <-entries:
					if !more {
						break
					}

					err := process(ctx, entry)
					if err != nil {
						workerErrors <- err
						return
					}
				case <-ctx.Done():
					workerErrors <- ctx.Err()
					return
				}
			}

			workerErrors <- nil
		}()
	}

	// Wait for all workers to complete.
	for i := 0; i < workerCount; i++ {
		err := <-workerErrors
		if err != nil {
			return err
		}
	}

	return nil
}

func (s *storageYDB) readChunk(
	ctx context.Context,
	chunk *dataplane_common.Chunk,
) (err error) {

	defer s.metrics.StatOperation("readChunk")(&err)

	if len(chunk.ID) == 0 {
		return &task_errors.NonRetriableError{
			Err: fmt.Errorf("chunkID should not be empty"),
		}
	}

	chunkStorage := s.getChunkStorage(chunk.StoredInS3)
	return chunkStorage.ReadChunk(ctx, chunk)
}

func (s *storageYDB) getSnapshot(
	ctx context.Context,
	snapshotID string,
) (state *snapshotState, err error) {

	defer s.metrics.StatOperation("getSnapshot")(&err)

	res, err := s.db.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from snapshots
		where id = $id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(snapshotID)),
	))
	if err != nil {
		return nil, err
	}
	defer res.Close()

	states, err := scanSnapshotStates(ctx, res)
	if err != nil {
		return nil, err
	}

	if len(states) != 0 {
		return &states[0], nil
	} else {
		return nil, nil
	}
}

func (s *storageYDB) checkSnapshotReady(
	ctx context.Context,
	snapshotID string,
) (meta SnapshotMeta, err error) {

	state, err := s.getSnapshot(ctx, snapshotID)
	if err != nil {
		return SnapshotMeta{}, err
	}

	if state == nil {
		return SnapshotMeta{}, &task_errors.NonRetriableError{
			Err: fmt.Errorf("snapshot with id=%v is not found", snapshotID),
		}
	}

	if state.status != snapshotStatusReady {
		return SnapshotMeta{}, &task_errors.NonRetriableError{
			Err:    fmt.Errorf("snapshot with id=%v is not ready", snapshotID),
			Silent: true,
		}
	}

	return SnapshotMeta{
		Size:        state.size,
		StorageSize: state.storageSize,
		ChunkCount:  state.chunkCount,
	}, nil
}

func (s *storageYDB) checkSnapshotAlive(
	ctx context.Context,
	snapshotID string,
) (err error) {

	state, err := s.getSnapshot(ctx, snapshotID)
	if err != nil {
		return err
	}

	if state == nil {
		return &task_errors.NonRetriableError{
			Err: fmt.Errorf("snapshot with id=%v is not found", snapshotID),
		}
	}

	if state.status >= snapshotStatusDeleting {
		return &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"snapshot with id=%v status=%v is not alive",
				snapshotID,
				snapshotStatusToString(state.status),
			),
			Silent: true,
		}
	}

	return nil
}

func (s *storageYDB) getDataChunkCount(
	ctx context.Context,
	snapshotID string,
) (dataChunkCount uint64, err error) {

	defer s.metrics.StatOperation("getDataChunkCount")(&err)

	res, err := s.db.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $shard_id as Uint64;
		declare $snapshot_id as Utf8;

		select count_if(chunk_id != '') as data_chunk_count
		from chunk_map
		where shard_id = $shard_id and snapshot_id = $snapshot_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$shard_id", ydb_types.Uint64Value(makeShardID(snapshotID))),
		ydb_table.ValueParam("$snapshot_id", ydb_types.UTF8Value(snapshotID)),
	))
	if err != nil {
		return 0, err
	}
	defer res.Close()

	dataChunkCount = uint64(0)

	if !res.NextResultSet(ctx) || !res.NextRow() {
		return 0, res.Err()
	}
	err = res.ScanNamed(
		ydb_named.OptionalWithDefault("data_chunk_count", &dataChunkCount),
	)
	if err != nil {
		return 0, &task_errors.NonRetriableError{
			Err: fmt.Errorf("getDataChunkCount: failed to parse row: %w", err),
		}
	}
	return dataChunkCount, res.Err()
}

func (s *storageYDB) getChunkStorage(useS3 bool) chunks.Storage {
	if useS3 {
		return s.chunkStorageS3
	} else {
		return s.chunkStorageYDB
	}
}
