package chunks

import (
	"context"
	"encoding/json"
	"fmt"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/compressor"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	task_errors "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

// Stores chunks metadata in YDB and data in S3.
type StorageS3 struct {
	storageCommon
	s3        *persistence.S3Client
	metrics   metrics.Metrics
	bucket    string
	keyPrefix string
}

func NewStorageS3(
	db *persistence.YDBClient,
	s3 *persistence.S3Client,
	bucket string,
	keyPrefix string,
	tablesPath string,
	metrics metrics.Metrics,
) *StorageS3 {

	return &StorageS3{
		storageCommon: newStorageCommon(db, tablesPath),
		s3:            s3,
		bucket:        bucket,
		keyPrefix:     keyPrefix,
		metrics:       metrics,
	}
}

////////////////////////////////////////////////////////////////////////////////

func (s *StorageS3) ReadChunk(
	ctx context.Context,
	chunk *common.Chunk,
) (err error) {

	defer s.metrics.StatOperation(metrics.OperationReadChunkBlob)(&err)

	objBytes, err := s.s3.GetObject(ctx, s.bucket, s.newS3Key(chunk.ID))
	if err != nil {
		return err
	}

	var obj s3Object
	err = json.Unmarshal(objBytes, &obj)
	if err != nil {
		return &task_errors.NonRetriableError{Err: err}
	}

	err = compressor.Decompress(obj.Compression, obj.Data, chunk.Data)
	if err != nil {
		return err
	}

	actualChecksum := chunkChecksum(chunk.Data)
	if obj.Checksum != actualChecksum {
		return &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"s3 chunk checksum mismatch: expected %v, actual %v",
				obj.Checksum,
				actualChecksum,
			),
		}
	}

	return nil
}

func (s *StorageS3) WriteChunk(
	ctx context.Context,
	referer string,
	chunkID string,
	data []byte,
	compression string,
) (compressedDataLen int, err error) {

	defer s.metrics.StatOperation(metrics.OperationWriteChunkBlob)(&err)

	errGroup, ctx := errgroup.WithContext(ctx)

	errGroup.Go(func() error {
		return s.writeChunkMetadata(ctx, referer, chunkID)
	})

	writtenDataLen := 0
	errGroup.Go(func() error {
		var err error
		writtenDataLen, err = s.writeChunkData(ctx, chunkID, data, compression)
		return err
	})

	err = errGroup.Wait()
	if err != nil {
		return 0, err
	}

	return writtenDataLen, nil
}

func (s *StorageS3) RefChunk(
	ctx context.Context,
	referer string,
	chunkID string,
) (err error) {

	defer s.metrics.StatOperation(metrics.OperationRefChunkBlob)(&err)
	return s.refChunk(ctx, referer, chunkID)
}

func (s *StorageS3) UnrefChunk(
	ctx context.Context,
	referer string,
	chunkID string,
) (err error) {

	defer s.metrics.StatOperation(metrics.OperationUnrefChunkBlob)(&err)

	var refCount uint32
	refCount, err = s.unrefChunkMetadata(ctx, referer, chunkID)
	if err != nil {
		return err
	}

	if refCount == 0 {
		err = s.deleteChunkData(ctx, chunkID)
		if err != nil {
			return err
		}
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func (s *StorageS3) writeChunkMetadata(
	ctx context.Context,
	referer string,
	chunkID string,
) error {

	return s.writeToChunkBlobs(
		ctx,
		referer,
		chunkID,
		[]byte{}, // data
		0,        // checksum
		"",       // compression
	)
}

func (s *StorageS3) writeChunkData(
	ctx context.Context,
	chunkID string,
	data []byte,
	compression string,
) (int, error) {

	checksum := chunkChecksum(data)

	compressedData, err := compressor.Compress(compression, data)
	if err != nil {
		return 0, err
	}
	data = compressedData

	obj := s3Object{
		Data:        data,
		Checksum:    checksum,
		Compression: compression,
	}

	objBytes, err := json.Marshal(obj)
	if err != nil {
		return 0, &task_errors.NonRetriableError{Err: err}
	}

	err = s.s3.PutObject(ctx, s.bucket, s.newS3Key(chunkID), objBytes)
	if err != nil {
		return 0, err
	}

	return len(data), nil
}

func (s *StorageS3) unrefChunkMetadata(
	ctx context.Context,
	referer string,
	chunkID string,
) (uint32, error) {

	return s.unrefChunk(ctx, referer, chunkID)
}

func (s *StorageS3) deleteChunkData(
	ctx context.Context,
	chunkID string,
) error {

	return s.s3.DeleteObject(ctx, s.bucket, s.newS3Key(chunkID))
}

////////////////////////////////////////////////////////////////////////////////

type s3Object struct {
	Data        []byte
	Checksum    uint32
	Compression string
}

func (s *StorageS3) newS3Key(chunkID string) string {
	return fmt.Sprintf("%v/%v", s.keyPrefix, chunkID)
}
