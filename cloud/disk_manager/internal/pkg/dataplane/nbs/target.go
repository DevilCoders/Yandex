package nbs

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type diskTarget struct {
	client           nbs.Client
	session          nbs.Session
	blocksInChunk    uint64
	ignoreZeroChunks bool
}

func (t *diskTarget) Write(
	ctx context.Context,
	chunk common.Chunk,
) error {

	if t.ignoreZeroChunks && chunk.Zero {
		return nil
	}

	startIndex := uint64(chunk.Index) * t.blocksInChunk

	var err error
	if chunk.Zero {
		// blockCount should be multiple of blocksInChunk.
		err = t.session.Zero(ctx, startIndex, uint32(t.blocksInChunk))
	} else {
		// TODO: normalize chunk data.
		err = t.session.Write(ctx, startIndex, chunk.Data)
	}

	return err
}

func (t *diskTarget) Close(ctx context.Context) {
	t.session.Close(ctx)
}

////////////////////////////////////////////////////////////////////////////////

func CreateDiskTarget(
	ctx context.Context,
	factory nbs.Factory,
	disk *types.Disk,
	chunkSize uint32,
	ignoreZeroChunks bool,
) (common.Target, error) {

	client, err := factory.GetClient(ctx, disk.ZoneId)
	if err != nil {
		return nil, err
	}

	session, err := client.MountRW(ctx, disk.DiskId)
	if err != nil {
		return nil, err
	}

	blockSize := session.BlockSize()

	err = validate(session.BlockCount(), chunkSize, blockSize)
	if err != nil {
		return nil, err
	}

	return &diskTarget{
		client:           client,
		session:          session,
		blocksInChunk:    uint64(chunkSize / blockSize),
		ignoreZeroChunks: ignoreZeroChunks,
	}, nil
}
