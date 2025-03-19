package url

import (
	"context"
	"fmt"
	"net/http"
	"sync"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/common"
	dataplane_common "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
)

////////////////////////////////////////////////////////////////////////////////

type urlSource struct {
	chunkMap      map[uint32]ChunkMapEntry
	chunkMapMutex sync.Mutex
	chunkIndices  *common.ChannelWithFeedback

	imageMapReader *ImageMapReader
	chunkSize      uint64
}

func (s *urlSource) Next(
	ctx context.Context,
	milestoneChunkIndex uint32,
	processedChunkIndices chan uint32,
) (<-chan uint32, <-chan error) {

	common.Assert(
		s.chunkIndices == nil,
		"urlSource.Next should be called once",
	)

	s.chunkIndices = common.CreateChannelWithFeedback(
		milestoneChunkIndex,
		processedChunkIndices,
	)

	errors := make(chan error, 1)

	go func() {
		defer s.chunkIndices.Close()
		defer close(errors)

		entries, entriesErrors := s.imageMapReader.Read(ctx, s.chunkSize)

		var entry ChunkMapEntry
		more := true

		for more {
			select {
			case entry, more = <-entries:
				if !more {
					break
				}

				// Cache chunk ids for further reads.
				s.chunkMapMutex.Lock()
				s.chunkMap[entry.chunkIndex] = entry
				s.chunkMapMutex.Unlock()

				err := s.chunkIndices.Send(ctx, entry.chunkIndex)
				if err != nil {
					errors <- err
					return
				}
			case <-ctx.Done():
				errors <- ctx.Err()
				return
			}
		}

		err := <-entriesErrors
		if err != nil {
			logging.Error(ctx, "Error while read image map: %v", err)
			errors <- err
		}
	}()

	return s.chunkIndices.GetChannel(), errors
}

func (s *urlSource) Read(
	ctx context.Context,
	chunk *dataplane_common.Chunk,
) (err error) {

	s.chunkMapMutex.Lock()
	mapEntry, ok := s.chunkMap[chunk.Index]
	s.chunkMapMutex.Unlock()

	if !ok {
		// TODO: maybe reread chunk map
		return fmt.Errorf(
			"failed to find chunk id for index %d",
			chunk.Index,
		)
	}

	urlImageReader := NewImageReader(*mapEntry.imageMap, s.chunkSize, http.DefaultClient)
	offset := urlImageReader.GetBlockSize() * uint64(chunk.Index)
	err = urlImageReader.Read(ctx, offset, chunk)
	if err != nil {
		return err
	}

	// Evict successully read chunk id to keep low memory footprint.
	s.chunkMapMutex.Lock()
	delete(s.chunkMap, chunk.Index)
	s.chunkMapMutex.Unlock()

	return nil
}

func (s *urlSource) GetMilestoneChunkIndex() uint32 {
	common.Assert(
		s.chunkIndices != nil,
		"urlSource.chunkIndices should not be nil",
	)

	return s.chunkIndices.GetMilestone()
}

func (s *urlSource) GetChunkCount(ctx context.Context) (uint32, error) {
	size := s.imageMapReader.Size()

	if size%s.chunkSize == 0 {
		return uint32(size / s.chunkSize), nil
	}
	return uint32(size/s.chunkSize) + 1, nil
}

func (s *urlSource) Close(ctx context.Context) {
}

////////////////////////////////////////////////////////////////////////////////

func CreateURLSource(
	ctx context.Context,
	URL string,
	chunkSize uint64,
) (dataplane_common.Source, error) {

	mapReader, err := NewImageMapReader(ctx, URL)
	if err != nil {
		return nil, err
	}

	return &urlSource{
		chunkSize:      chunkSize,
		chunkMap:       make(map[uint32]ChunkMapEntry),
		imageMapReader: mapReader,
	}, nil
}
