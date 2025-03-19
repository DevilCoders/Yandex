package nbs

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/common"
	dataplane_common "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

const (
	maxChangedByteCountPerIteration = uint64((1 << 20) * 4096)
)

////////////////////////////////////////////////////////////////////////////////

type diskSource struct {
	client           nbs.Client
	session          nbs.Session
	diskID           string
	baseCheckpointID string
	checkpointID     string
	blocksInChunk    uint64
	blockCount       uint64
	chunkCount       uint32

	useGetChangedBlocks              bool
	maxChangedBlockCountPerIteration uint64

	chunkIndices *common.ChannelWithFeedback
}

func (s *diskSource) generateChunkIndicesDefault(
	ctx context.Context,
	milestoneChunkIndex uint32,
) error {

	for chunkIndex := milestoneChunkIndex; chunkIndex < s.chunkCount; chunkIndex++ {
		err := s.chunkIndices.Send(ctx, chunkIndex)
		if err != nil {
			return err
		}
	}

	return nil
}

func (s *diskSource) generateChunkIndicesUsingGetChangedBlocks(
	ctx context.Context,
	milestoneChunkIndex uint32,
) error {

	totalBlockCount := s.blockCount

	blockIndex := uint64(milestoneChunkIndex) * s.blocksInChunk
	for blockIndex < totalBlockCount {
		blockCount := uint32(s.maxChangedBlockCountPerIteration)
		if uint64(blockCount) > totalBlockCount-blockIndex {
			blockCount = uint32(totalBlockCount - blockIndex)
		}

		blockMask, err := s.client.GetChangedBlocks(
			ctx,
			s.diskID,
			blockIndex,
			blockCount,
			s.baseCheckpointID,
			s.checkpointID,
		)
		if err != nil {
			return err
		}

		chunkIndex := uint32(blockIndex / s.blocksInChunk)

		i := 0
		for i < len(blockMask) {
			// blocksInChunk should be multiple of 8.
			chunkEnd := i + int(s.blocksInChunk/8)

			for i < len(blockMask) && i < chunkEnd {
				if blockMask[i] != 0 {
					err := s.chunkIndices.Send(ctx, chunkIndex)
					if err != nil {
						return err
					}

					i = chunkEnd
					break
				}

				i++
			}

			chunkIndex++
		}

		blockIndex += s.maxChangedBlockCountPerIteration
	}

	return nil
}

func (s *diskSource) Next(
	ctx context.Context,
	milestoneChunkIndex uint32,
	processedChunkIndices chan uint32,
) (<-chan uint32, <-chan error) {

	common.Assert(
		s.chunkIndices == nil,
		"diskSource.Next should be called once",
	)

	s.chunkIndices = common.CreateChannelWithFeedback(
		milestoneChunkIndex,
		processedChunkIndices,
	)

	errors := make(chan error, 1)

	go func() {
		defer s.chunkIndices.Close()
		defer close(errors)

		var err error
		if s.useGetChangedBlocks {
			err = s.generateChunkIndicesUsingGetChangedBlocks(
				ctx,
				milestoneChunkIndex,
			)
		} else {
			err = s.generateChunkIndicesDefault(ctx, milestoneChunkIndex)
		}
		if err != nil {
			errors <- err
		}
	}()

	return s.chunkIndices.GetChannel(), errors
}

func (s *diskSource) Read(
	ctx context.Context,
	chunk *dataplane_common.Chunk,
) error {

	startIndex := uint64(chunk.Index) * s.blocksInChunk
	// blockCount should be multiple of blocksInChunk.

	err := s.session.Read(
		ctx,
		startIndex,
		uint32(s.blocksInChunk),
		s.checkpointID,
		chunk,
	)
	if err != nil {
		return err
	}

	return nil
}

func (s *diskSource) GetMilestoneChunkIndex() uint32 {
	common.Assert(
		s.chunkIndices != nil,
		"diskSource.chunkIndices should not be nil",
	)

	return s.chunkIndices.GetMilestone()
}

func (s *diskSource) GetChunkCount(ctx context.Context) (uint32, error) {
	return s.chunkCount, nil
}

func (s *diskSource) Close(ctx context.Context) {
	s.session.Close(ctx)
}

////////////////////////////////////////////////////////////////////////////////

func CreateDiskSource(
	ctx context.Context,
	factory nbs.Factory,
	disk *types.Disk,
	baseCheckpointID string,
	checkpointID string,
	chunkSize uint32,
	useGetChangedBlocks bool,
) (dataplane_common.Source, error) {

	client, err := factory.GetClient(ctx, disk.ZoneId)
	if err != nil {
		return nil, err
	}

	session, err := client.MountRO(ctx, disk.DiskId)
	if err != nil {
		return nil, err
	}

	blockSize := session.BlockSize()
	blockCount := session.BlockCount()

	err = validate(blockCount, chunkSize, blockSize)
	if err != nil {
		return nil, err
	}

	blocksInChunk := uint64(chunkSize / blockSize)
	if blocksInChunk%8 != 0 {
		return nil, &errors.NonRetriableError{
			Err: fmt.Errorf(
				"blocksInChunk should be multiple of 8, blocksInChunk=%v",
				blocksInChunk,
			),
		}
	}

	maxChangedBlockCountPerIteration := maxChangedByteCountPerIteration / uint64(blockSize)

	if maxChangedBlockCountPerIteration%blocksInChunk != 0 {
		return nil, &errors.NonRetriableError{
			Err: fmt.Errorf(
				"maxChangedBlockCountPerIteration should be multiple of blocksInChunk, maxChangedBlockCountPerIteration=%v, blocksInChunk=%v",
				maxChangedBlockCountPerIteration,
				blocksInChunk,
			),
		}
	}

	chunkCount := uint32(blockCount / blocksInChunk)

	if len(baseCheckpointID) != 0 {
		// Force GetChangedBlocks using for incremental snapshots.
		useGetChangedBlocks = true
	}

	return &diskSource{
		client:           client,
		session:          session,
		diskID:           disk.DiskId,
		baseCheckpointID: baseCheckpointID,
		checkpointID:     checkpointID,
		blocksInChunk:    blocksInChunk,
		blockCount:       blockCount,
		chunkCount:       chunkCount,

		useGetChangedBlocks:              useGetChangedBlocks,
		maxChangedBlockCountPerIteration: maxChangedBlockCountPerIteration,
	}, nil
}
