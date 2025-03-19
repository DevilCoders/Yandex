package nbs

import (
	"context"

	nbs_client "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
)

////////////////////////////////////////////////////////////////////////////////

type Session struct {
	client  nbs_client.Client
	session *nbs_client.Session
	metrics *clientMetrics
}

func (s *Session) BlockSize() uint32 {
	return s.session.Volume().BlockSize
}

func (s *Session) BlockCount() uint64 {
	return s.session.Volume().BlocksCount
}

func (s *Session) IsOverlayDisk() bool {
	return len(s.session.Volume().BaseDiskId) != 0
}

func (s *Session) Read(
	ctx context.Context,
	startIndex uint64,
	blockCount uint32,
	checkpointID string,
	chunk *common.Chunk,
) (err error) {

	defer s.metrics.StatRequest("Read")(&err)

	blocks, err := s.session.ReadBlocks(
		ctx,
		startIndex,
		blockCount,
		checkpointID,
	)
	if err != nil {
		return wrapError(err)
	}

	if nbs_client.AllBlocksEmpty(blocks) {
		chunk.Zero = true
		return nil
	}

	return nbs_client.JoinBlocks(s.BlockSize(), blockCount, blocks, chunk.Data)
}

func (s *Session) Write(
	ctx context.Context,
	startIndex uint64,
	data []byte,
) (err error) {

	defer s.metrics.StatRequest("Write")(&err)

	err = s.session.WriteBlocks(ctx, startIndex, [][]byte{data})
	return wrapError(err)
}

func (s *Session) Zero(
	ctx context.Context,
	startIndex uint64,
	blockCount uint32,
) (err error) {

	defer s.metrics.StatRequest("Zero")(&err)

	err = s.session.ZeroBlocks(ctx, startIndex, blockCount)
	return wrapError(err)
}

func (s *Session) Close(ctx context.Context) {
	var err error

	defer s.metrics.StatRequest("Close")(&err)

	_ = s.session.UnmountVolume(ctx)
	s.session.Close()
	_ = s.client.Close()
}
