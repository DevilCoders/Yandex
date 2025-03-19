package storage

import (
	"context"
	"crypto/sha512"
	"encoding/hex"
	"fmt"
	"hash"
	"math"
	"path"
	"strings"

	ydb_table "github.com/ydb-platform/ydb-go-sdk/v3/table"
	ydb_options "github.com/ydb-platform/ydb-go-sdk/v3/table/options"
	ydb_named "github.com/ydb-platform/ydb-go-sdk/v3/table/result/named"
	ydb_types "github.com/ydb-platform/ydb-go-sdk/v3/table/types"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/common"
	dataplane_common "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/compressor"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	task_errors "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	error_codes "a.yandex-team.ru/cloud/disk_manager/pkg/client/codes"
)

////////////////////////////////////////////////////////////////////////////////

func (s *legacyStorage) readChunkMap(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
	milestoneChunkIndex uint32,
	inflightQueue *common.InflightQueue,
) (<-chan ChunkMapEntry, <-chan error) {

	entries := make(chan ChunkMapEntry)
	errors := make(chan error, 1)

	snapshotInfo, err := s.readSnapshotInfo(ctx, session, snapshotID)
	if err != nil {
		errors <- err
		close(entries)
		close(errors)
		return entries, errors
	}

	res, err := session.StreamReadTable(
		ctx,
		path.Join(s.tablesPath, "snapshotchunks"),
		ydb_options.ReadOrdered(),
		ydb_options.ReadColumn("chunkoffset"),
		ydb_options.ReadColumn("chunkid"),
		ydb_options.ReadGreaterOrEqual(ydb_types.TupleValue(
			ydb_types.OptionalValue(ydb_types.StringValue([]byte(snapshotInfo.tree))),
			ydb_types.OptionalValue(ydb_types.StringValue([]byte(snapshotID))),
			ydb_types.OptionalValue(ydb_types.Int64Value(int64(milestoneChunkIndex))),
		)),
		ydb_options.ReadLessOrEqual(ydb_types.TupleValue(
			ydb_types.OptionalValue(ydb_types.StringValue([]byte(snapshotInfo.tree))),
			ydb_types.OptionalValue(ydb_types.StringValue([]byte(snapshotID))),
			ydb_types.OptionalValue(ydb_types.Int64Value(math.MaxInt64)),
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
				var (
					chunkOffset int64
					chunkID     []byte
				)
				err = res.ScanNamed(
					ydb_named.OptionalWithDefault("chunkoffset", &chunkOffset),
					ydb_named.OptionalWithDefault("chunkid", &chunkID),
				)
				if err != nil {
					errors <- &task_errors.NonRetriableError{
						Err: fmt.Errorf("readChunkMap: failed to parse row: %w", err),
					}
					return
				}

				entry := ChunkMapEntry{
					ChunkIndex: uint32(chunkOffset / snapshotInfo.chunkSize),
					ChunkID:    string(chunkID),
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

////////////////////////////////////////////////////////////////////////////////

func (s *legacyStorage) readChunk(
	ctx context.Context,
	session *persistence.Session,
	chunk *dataplane_common.Chunk,
) (err error) {

	defer s.metrics.StatOperation("legacy/readChunk")(&err)

	if len(chunk.ID) == 0 {
		return &task_errors.NonRetriableError{
			Err: fmt.Errorf("chunkID should not be empty"),
		}
	}

	chunkInfo, err := s.readChunkInfo(ctx, session, chunk.ID)
	if err != nil {
		return err
	}

	if chunkInfo.zero {
		chunk.Zero = true
		return nil
	}

	data, err := s.readChunkData(ctx, session, chunk.ID)
	if err != nil {
		return err
	}

	err = compressor.Decompress(chunkInfo.format, data, chunk.Data)
	if err != nil {
		return err
	}

	if len(chunk.Data) != int(chunkInfo.size) {
		return &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"chunk size mismatch: expected %v, actual %v",
				chunkInfo.size,
				len(chunk.Data),
			),
		}
	}

	checksumType := getChecksumType(chunkInfo.sum)
	checksum := getChecksum(chunk.Data, checksumType)

	if checksum != chunkInfo.sum {
		return &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"chunk checksum mismatch: expected %v, actual %v",
				chunkInfo.sum,
				checksum,
			),
		}
	}

	return nil
}

func (s *legacyStorage) checkSnapshotReady(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
) (meta SnapshotMeta, err error) {

	snapshotInfo, err := s.readSnapshotInfo(ctx, session, snapshotID)
	if err != nil {
		return SnapshotMeta{}, err
	}

	if snapshotInfo.state != "ready" {
		return SnapshotMeta{}, &task_errors.NonRetriableError{
			Err:    fmt.Errorf("snapshot with id=%v is not ready", snapshotID),
			Silent: true,
		}
	}

	return SnapshotMeta{
		Size:        uint64(snapshotInfo.size),
		StorageSize: uint64(snapshotInfo.realSize),
		ChunkCount:  uint32(snapshotInfo.realSize / snapshotInfo.chunkSize),
	}, nil
}

////////////////////////////////////////////////////////////////////////////////

type snapshotInfo struct {
	base      string
	tree      string
	chunkSize int64
	size      int64
	realSize  int64
	state     string
}

func (s *legacyStorage) readSnapshotInfo(
	ctx context.Context,
	session *persistence.Session,
	snapshotID string,
) (snapshotInfo, error) {

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $snapshot_id as String;

		select *
		from snapshotsext
		where id = $snapshot_id;
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$snapshot_id", ydb_types.StringValue([]byte(snapshotID))),
	))
	if err != nil {
		return snapshotInfo{}, err
	}
	defer res.Close()

	if !res.NextResultSet(ctx) || !res.NextRow() {
		return snapshotInfo{}, &task_errors.DetailedError{
			Err: &task_errors.NonRetriableError{
				Err: fmt.Errorf("snapshot with id=%v is not found", snapshotID),
			},
			Details: &task_errors.ErrorDetails{
				Code:     error_codes.BadSource,
				Message:  "snapshot or image not found",
				Internal: false,
			},
		}
	}
	var (
		base      []byte
		tree      []byte
		chunkSize int64
		size      int64
		realSize  int64
		state     []byte
	)
	err = res.ScanNamed(
		ydb_named.OptionalWithDefault("base", &base),
		ydb_named.OptionalWithDefault("tree", &tree),
		ydb_named.OptionalWithDefault("chunksize", &chunkSize),
		ydb_named.OptionalWithDefault("size", &size),
		ydb_named.OptionalWithDefault("realsize", &realSize),
		ydb_named.OptionalWithDefault("state", &state),
	)
	if err != nil {
		return snapshotInfo{}, &task_errors.DetailedError{
			Err: &task_errors.NonRetriableError{
				Err: fmt.Errorf("readSnapshotInfo: failed to parse row: %w", err),
			},
		}
	}
	return snapshotInfo{
		base:      string(base),
		tree:      string(tree),
		chunkSize: chunkSize,
		size:      size,
		realSize:  realSize,
		state:     string(state),
	}, res.Err()
}

////////////////////////////////////////////////////////////////////////////////

type chunkInfo struct {
	sum    string
	format string
	size   int64
	refcnt int64
	zero   bool
}

func (s *legacyStorage) readChunkInfo(
	ctx context.Context,
	session *persistence.Session,
	chunkID string,
) (chunkInfo, error) {

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $chunk_id as String;

		select *
		from chunks
		where id = $chunk_id;
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$chunk_id", ydb_types.StringValue([]byte(chunkID))),
	))
	if err != nil {
		return chunkInfo{}, err
	}
	defer res.Close()

	if !res.NextResultSet(ctx) || !res.NextRow() {
		return chunkInfo{}, &task_errors.NonRetriableError{
			Err: fmt.Errorf("chunk not found: %v", chunkID),
		}
	}
	var (
		sum    []byte
		format []byte
		size   int64
		refcnt int64
		zero   bool
	)
	err = res.ScanNamed(
		ydb_named.OptionalWithDefault("sum", &sum),
		ydb_named.OptionalWithDefault("format", &format),
		ydb_named.OptionalWithDefault("size", &size),
		ydb_named.OptionalWithDefault("refcnt", &refcnt),
		ydb_named.OptionalWithDefault("zero", &zero),
	)
	if err != nil {
		return chunkInfo{}, &task_errors.NonRetriableError{
			Err: fmt.Errorf("readChunkInfo: failed to parse row: %w", err),
		}
	}
	return chunkInfo{
		sum:    string(sum),
		format: string(format),
		size:   size,
		refcnt: refcnt,
		zero:   zero,
	}, res.Err()
}

func (s *legacyStorage) readChunkData(
	ctx context.Context,
	session *persistence.Session,
	chunkID string,
) ([]byte, error) {

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $chunk_id as String;

		select *
		from blobs_on_hdd
		where id = $chunk_id;
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$chunk_id", ydb_types.StringValue([]byte(chunkID))),
	))
	if err != nil {
		return nil, err
	}
	defer res.Close()

	if !res.NextResultSet(ctx) || !res.NextRow() {
		return nil, &task_errors.NonRetriableError{
			Err: fmt.Errorf("chunk not found: %v", chunkID),
		}
	}
	var data []byte
	err = res.ScanNamed(
		ydb_named.OptionalWithDefault("data", &data),
	)
	if err != nil {
		return nil, &task_errors.NonRetriableError{
			Err: fmt.Errorf("readChunkData: failed to parse row: %w", err),
		}
	}
	return data, res.Err()
}

////////////////////////////////////////////////////////////////////////////////

func getChecksumType(checksum string) string {
	parts := strings.SplitN(checksum, ":", 2)
	if len(parts) == 2 {
		return parts[0]
	}
	return ""
}

func getChecksum(data []byte, checksumType string) string {
	var hasher hash.Hash
	if checksumType == "sha512" {
		hasher = sha512.New()
	} else {
		return ""
	}

	_, err := hasher.Write(data)
	if err != nil {
		return ""
	}

	return fmt.Sprintf("%v:%v", checksumType, hex.EncodeToString(hasher.Sum(nil)))
}
