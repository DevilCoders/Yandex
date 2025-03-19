package chunks

import (
	"context"
	"errors"
	"fmt"

	ydb_table "github.com/ydb-platform/ydb-go-sdk/v3/table"
	ydb_named "github.com/ydb-platform/ydb-go-sdk/v3/table/result/named"
	ydb_types "github.com/ydb-platform/ydb-go-sdk/v3/table/types"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/compressor"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	task_errors "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/rtc/mediator/cityhash"
)

////////////////////////////////////////////////////////////////////////////////

// Stores chunks data and metadata in YDB.
type StorageYDB struct {
	storageCommon
	metrics metrics.Metrics
}

func NewStorageYDB(
	db *persistence.YDBClient,
	tablesPath string,
	metrics metrics.Metrics,
) *StorageYDB {

	return &StorageYDB{
		storageCommon: newStorageCommon(db, tablesPath),
		metrics:       metrics,
	}
}

////////////////////////////////////////////////////////////////////////////////

func (s *StorageYDB) ReadChunk(
	ctx context.Context,
	chunk *common.Chunk,
) (err error) {

	defer s.metrics.StatOperation(metrics.OperationReadChunkBlob)(&err)

	res, err := s.db.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $shard_id as Uint64;
		declare $chunk_id as Utf8;

		select * from chunk_blobs
		where shard_id = $shard_id and
			chunk_id = $chunk_id and
			referer = "";
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$shard_id", ydb_types.Uint64Value(makeShardID(chunk.ID))),
		ydb_table.ValueParam("$chunk_id", ydb_types.UTF8Value(chunk.ID)),
	))
	if err != nil {
		return err
	}
	defer res.Close()

	if !res.NextResultSet(ctx) || !res.NextRow() {
		return &task_errors.NonRetriableError{
			Err: errors.New("chunk not found"),
		}
	}

	var (
		data        []byte
		checksum    uint32
		compression string
	)
	err = res.ScanNamed(
		ydb_named.OptionalWithDefault("data", &data),
		ydb_named.OptionalWithDefault("checksum", &checksum),
		ydb_named.OptionalWithDefault("compression", &compression),
	)
	if err != nil {
		return &task_errors.NonRetriableError{
			Err: fmt.Errorf("readChunk: failed to parse row: %w", err),
		}
	}

	err = compressor.Decompress(compression, data, chunk.Data)
	if err != nil {
		return err
	}

	actualChecksum := chunkChecksum(chunk.Data)
	if checksum != actualChecksum {
		return &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"ydb chunk checksum mismatch: expected %v, actual %v",
				checksum,
				actualChecksum,
			),
		}
	}

	return res.Err()
}

func (s *StorageYDB) WriteChunk(
	ctx context.Context,
	referer string,
	chunkID string,
	data []byte,
	compression string,
) (compressedDataLen int, err error) {

	defer s.metrics.StatOperation(metrics.OperationWriteChunkBlob)(&err)

	checksum := chunkChecksum(data)

	compressedData, err := compressor.Compress(compression, data)
	if err != nil {
		return 0, err
	}

	err = s.writeToChunkBlobs(
		ctx,
		referer,
		chunkID,
		compressedData,
		checksum,
		compression,
	)
	return len(compressedData), err
}

func (s *StorageYDB) RefChunk(
	ctx context.Context,
	referer string,
	chunkID string,
) (err error) {

	defer s.metrics.StatOperation(metrics.OperationRefChunkBlob)(&err)
	return s.refChunk(ctx, referer, chunkID)
}

func (s *StorageYDB) UnrefChunk(
	ctx context.Context,
	referer string,
	chunkID string,
) (err error) {

	defer s.metrics.StatOperation(metrics.OperationUnrefChunkBlob)(&err)
	_, err = s.unrefChunk(ctx, referer, chunkID)
	return err
}

////////////////////////////////////////////////////////////////////////////////

func makeShardID(chunkID string) uint64 {
	return cityhash.Hash64([]byte(chunkID))
}
