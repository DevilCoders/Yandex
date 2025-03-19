package snapshot

import (
	"context"
	"fmt"
	"sync"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/accounting"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/common"
	dataplane_common "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage"
)

////////////////////////////////////////////////////////////////////////////////

type snapshotSource struct {
	snapshotID string
	storage    storage.Storage

	chunkMap      map[uint32]dataplane_common.Chunk
	chunkMapMutex sync.Mutex
	chunkIndices  *common.ChannelWithFeedback
}

func (s *snapshotSource) Next(
	ctx context.Context,
	milestoneChunkIndex uint32,
	processedChunkIndices chan uint32,
) (<-chan uint32, <-chan error) {

	common.Assert(
		s.chunkIndices == nil,
		"snapshotSource.Next should be called once",
	)

	s.chunkIndices = common.CreateChannelWithFeedback(
		milestoneChunkIndex,
		processedChunkIndices,
	)

	errors := make(chan error, 1)

	go func() {
		defer s.chunkIndices.Close()
		defer close(errors)

		entries, entriesErrors := s.storage.ReadChunkMap(
			ctx,
			s.snapshotID,
			milestoneChunkIndex,
		)

		var entry storage.ChunkMapEntry
		more := true

		for more {
			select {
			case entry, more = <-entries:
				if !more {
					break
				}

				// Cache chunk ids for further reads.
				s.chunkMapMutex.Lock()
				s.chunkMap[entry.ChunkIndex] = dataplane_common.Chunk{
					ID:         entry.ChunkID,
					StoredInS3: entry.StoredInS3,
				}
				s.chunkMapMutex.Unlock()

				err := s.chunkIndices.Send(ctx, entry.ChunkIndex)
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
			errors <- err
		}
	}()

	return s.chunkIndices.GetChannel(), errors
}

func (s *snapshotSource) Read(
	ctx context.Context,
	chunk *dataplane_common.Chunk,
) (err error) {

	s.chunkMapMutex.Lock()
	readChunk, ok := s.chunkMap[chunk.Index]
	s.chunkMapMutex.Unlock()

	chunk.ID = readChunk.ID
	chunk.StoredInS3 = readChunk.StoredInS3

	if !ok {
		// TODO: maybe reread chunk map
		return fmt.Errorf(
			"failed to find chunk id for index %d",
			chunk.Index,
		)
	}

	if len(chunk.ID) != 0 {
		err = s.storage.ReadChunk(ctx, chunk)
	} else {
		chunk.Zero = true
	}
	if err == nil {
		// Evict successully read chunk id to keep low memory footprint.
		s.chunkMapMutex.Lock()
		delete(s.chunkMap, chunk.Index)
		s.chunkMapMutex.Unlock()

		accounting.OnSnapshotRead(s.snapshotID, len(chunk.Data))
	}

	return err
}

func (s *snapshotSource) GetMilestoneChunkIndex() uint32 {
	common.Assert(
		s.chunkIndices != nil,
		"snapshotSource.chunkIndices should not be nil",
	)

	return s.chunkIndices.GetMilestone()
}

func (s *snapshotSource) GetChunkCount(ctx context.Context) (uint32, error) {
	meta, err := s.storage.CheckSnapshotReady(ctx, s.snapshotID)
	if err != nil {
		return 0, err
	}

	return meta.ChunkCount, nil
}

func (s *snapshotSource) Close(ctx context.Context) {
}

////////////////////////////////////////////////////////////////////////////////

func CreateSnapshotSource(
	snapshotID string,
	storage storage.Storage,
) dataplane_common.Source {

	return &snapshotSource{
		snapshotID: snapshotID,
		storage:    storage,
		chunkMap:   make(map[uint32]dataplane_common.Chunk),
	}
}
