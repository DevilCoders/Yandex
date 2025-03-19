package common

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/common"
)

////////////////////////////////////////////////////////////////////////////////

type Source interface {
	// Should be called once.
	// Takes feedback channel that produces indices of chunks which have been
	// processed (needed for backpressure to reduce congestion and for computing
	// last valid state to restore from).
	// Returns channel that produces indices of chunks to read.
	Next(
		ctx context.Context,
		milestoneChunkIndex uint32,
		processedChunkIndices chan uint32,
	) (chunkIndices <-chan uint32, errors <-chan error)

	Read(ctx context.Context, chunk *Chunk) error

	// Returns index to restore from.
	GetMilestoneChunkIndex() uint32

	GetChunkCount(ctx context.Context) (uint32, error)

	Close(ctx context.Context)
}

type Target interface {
	Write(ctx context.Context, chunk Chunk) error
	Close(ctx context.Context)
}

////////////////////////////////////////////////////////////////////////////////

const (
	saveProgressPeriod = time.Second
)

////////////////////////////////////////////////////////////////////////////////

// Transfers data from |source| to |target|.
// Takes |saveProgress| function which is periodically called in order to
// save execution state persistently.
func Transfer(
	ctx context.Context,
	source Source,
	target Target,
	milestoneChunkIndex uint32,
	saveProgress func(context.Context) error,
	readerCount int,
	writerCount int,
	chunksInflightLimit int,
	chunkSize int,
) error {

	var cancelCtx func()
	ctx, cancelCtx = context.WithCancel(ctx)
	defer cancelCtx()

	processedChunkIndices := make(chan uint32, chunksInflightLimit)

	chunkIndices, chunkIndicesErrors := source.Next(
		ctx,
		milestoneChunkIndex,
		processedChunkIndices,
	)

	readerErrors := make(chan error, readerCount)
	writerErrors := make(chan error, writerCount)

	chunks := make(chan *Chunk, chunksInflightLimit)

	chunkPool := sync.Pool{
		New: func() interface{} {
			return &Chunk{
				Data: make([]byte, chunkSize),
			}
		},
	}

	for i := 0; i < readerCount; i++ {
		go func() {
			for {
				select {
				case chunkIndex, more := <-chunkIndices:
					if !more {
						readerErrors <- nil
						return
					}

					chunk := chunkPool.Get().(*Chunk)
					chunk.Index = chunkIndex
					chunk.Zero = false

					err := source.Read(ctx, chunk)
					if err != nil {
						readerErrors <- err
						return
					}

					select {
					case chunks <- chunk:
					case <-ctx.Done():
						readerErrors <- ctx.Err()
						return
					}
				case <-ctx.Done():
					readerErrors <- ctx.Err()
					return
				}
			}
		}()
	}

	for i := 0; i < writerCount; i++ {
		go func() {
			for {
				select {
				case chunk, more := <-chunks:
					if !more {
						writerErrors <- nil
						return
					}
					chunkIndex := chunk.Index

					err := target.Write(ctx, *chunk)
					chunkPool.Put(chunk)
					if err != nil {
						writerErrors <- err
						return
					}

					select {
					case processedChunkIndices <- chunkIndex:
					case <-ctx.Done():
						writerErrors <- ctx.Err()
						return
					}
				case <-ctx.Done():
					writerErrors <- ctx.Err()
					return
				}
			}
		}()
	}

	waitUpdater, updaterError := common.ProgressUpdater(
		ctx,
		saveProgressPeriod,
		saveProgress,
	)
	defer waitUpdater()

	chunkIndicesDone := false
	readersDone := 0
	writersDone := 0

	chunksClosed := false

	for {
		select {
		case err := <-chunkIndicesErrors:
			if err != nil {
				return err
			}

			chunkIndicesDone = true
		case err := <-readerErrors:
			if err != nil {
				return err
			}

			readersDone++
		case err := <-writerErrors:
			if err != nil {
				return err
			}

			writersDone++
		case <-ctx.Done():
			return ctx.Err()
		case err := <-updaterError:
			return err
		}

		if chunkIndicesDone && readersDone == readerCount {
			if !chunksClosed {
				// It's safe to close chunks since we know that all readers are
				// finished their work.
				close(chunks)
				chunksClosed = true
			}

			if writersDone == writerCount {
				return nil
			}
		}
	}
}
