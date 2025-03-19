package s3logger

import (
	"container/list"
	"context"
	"io"
	"sync"
	"sync/atomic"
	"time"

	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/service/s3"

	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// ChunkUploader is an interface to chunks' storage.
type ChunkUploader interface {
	// UploadChunk saves chunk to storage.
	UploadChunk(ctx context.Context, index int, content []byte) error
	Disable()
}

// PipeProcessor reads data from stream, splits it into chunks and saves chunks to storage.
type PipeProcessor struct {
	uploader    ChunkUploader // responsible for syncing chunks to storage. Useful in tests.
	config      Config
	logger      log.Logger
	chunks      *list.List // chunks that are still writable or not synced
	syncMutex   sync.Mutex // protects from two simultaneosly going syncs
	chunksMutex sync.Mutex // protects chunks list
	chunkIndex  int        // next chunk serial number
	maxChunks   int        // max num of chunks in memory
	isClosed    atomic.Value
	closeOnce   sync.Once
	startBGSync sync.Once // ensures that background sync is started only once
}

func newPipeProcessor(uploader ChunkUploader, config Config, logger log.Logger) *PipeProcessor {
	return &PipeProcessor{
		uploader:  uploader,
		config:    config,
		logger:    logger,
		chunks:    list.New(),
		maxChunks: config.MaxPoolSize / config.ChunkSize,
	}
}

// ensureSyncStarted starts goroutine that syncs data to S3 in background
// if hasn't been started yet
func (pp *PipeProcessor) ensureSyncStarted() {
	pp.startBGSync.Do(func() {
		go pp.syncData()
	})
}

// syncData periodically saves modified chunks. It runs until processor is closed
func (pp *PipeProcessor) syncData() {
	ticker := time.NewTicker(pp.config.SyncInterval)
	defer ticker.Stop()

	for range ticker.C {
		pp.syncMutex.Lock()
		if pp.closed() {
			pp.syncMutex.Unlock()
			return
		}
		err := pp.sync(context.Background())
		pp.syncMutex.Unlock()
		if err != nil {
			pp.logger.Errorf("Failed to sync buffers: %s", err)
		}
	}
}

func (pp *PipeProcessor) closed() bool {
	val, ok := pp.isClosed.Load().(bool)
	if !ok {
		return false
	}
	return val
}

// currentChunk returns current writable (not full) chunk. Creates new chunk
// if necessary. Removes chunk from memory pool if pool size is exceeded.
func (pp *PipeProcessor) currentChunk() *LogChunk {
	pp.chunksMutex.Lock()
	defer pp.chunksMutex.Unlock()

	chunk := (*LogChunk)(nil)
	if pp.chunks.Back() != nil {
		chunk = pp.chunks.Back().Value.(*LogChunk)
	}
	if chunk == nil || chunk.full {
		chunk = NewChunk(pp.chunkIndex, pp.config.ChunkSize)
		pp.chunks.PushBack(chunk)
		pp.chunkIndex += 1

		if pp.chunks.Len() > pp.maxChunks {
			first := pp.chunks.Front()
			if first != nil {
				pp.chunks.Remove(first)
			}
		}
	}
	return chunk
}

// ReadFrom accepts reader stream and blocks until all data will be read
// out of stream. It does not wait until all data will be uploaded to S3 -
// it is done within separate goroutine.
func (pp *PipeProcessor) ReadFrom(reader io.Reader) (int64, error) {
	if pp.closed() {
		return 0, xerrors.New("writing to closed pipe processor")
	}
	pp.ensureSyncStarted()

	var counter int64
	counter = 0
	for {
		chunk := pp.currentChunk()
		n, err := chunk.ReadFrom(reader)
		counter += n
		if err != nil {
			if err == io.EOF {
				return counter, nil
			}
			return counter, err
		}
	}
}

// sync saves modified chunks to storage.
// Should be called with syncMutex held.
func (pp *PipeProcessor) sync(ctx context.Context) error {
	pp.chunksMutex.Lock()

	e := pp.chunks.Front()
	for e != nil {
		pp.chunksMutex.Unlock()

		chunk := e.Value.(*LogChunk)
		if chunk.dirty {
			index, content, offset := chunk.buildUploadRequest()
			childCtx, cancel := context.WithTimeout(ctx, pp.config.UploadTimeout)
			err := pp.uploader.UploadChunk(childCtx, index, content)
			cancel()
			if err != nil {
				return xerrors.Errorf("failed to upload chunk to s3: %w", err)
			} else {
				chunk.revisionUploaded(offset)
			}
		}

		pp.chunksMutex.Lock()
		next := e.Next()
		if chunk.closed {
			pp.chunks.Remove(e)
		}
		e = next
	}

	pp.chunksMutex.Unlock() // intentionally not using defer
	return nil
}

// Flush saves buffered output to backing storage (S3) with retries.
func (pp *PipeProcessor) Flush(ctx context.Context) error {
	pp.syncMutex.Lock()
	defer pp.syncMutex.Unlock()

	return retry.New(retry.Config{
		InitialInterval: pp.config.SyncInterval,
	}).RetryWithLog(
		ctx,
		func() error {
			err := pp.sync(ctx)

			var awsError awserr.Error
			if !xerrors.As(err, &awsError) {
				return err
			}

			// do not retry some errors
			if awsError.Code() == s3.ErrCodeNoSuchBucket || awsError.Code() == "AccessDenied" {
				return retry.Permanent(err)
			}
			return err
		},
		"flush buffered output",
		pp.logger,
	)
}

func (pp *PipeProcessor) Disable() {
	pp.uploader.Disable()
}

// Close method syncs data to storage and closes processor
func (pp *PipeProcessor) Close(ctx context.Context) error {
	var err error = nil
	pp.closeOnce.Do(func() {
		pp.isClosed.Store(true)
		err = pp.Flush(ctx)
		if err != nil {
			pp.logger.Errorf("Error closing output saver: %s", err)
		} else {
			pp.logger.Info("Output successfully saved to S3.")
		}
	})
	return err
}
