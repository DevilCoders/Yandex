package dataplane

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"

	nbs_client "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type createSnapshotFromDiskTask struct {
	nbsFactory nbs_client.Factory
	storage    storage.Storage
	config     *config.DataplaneConfig
	state      *protos.CreateSnapshotFromDiskTaskState
}

func (t *createSnapshotFromDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.CreateSnapshotFromDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.CreateSnapshotFromDiskTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *createSnapshotFromDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *createSnapshotFromDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CreateSnapshotFromDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *createSnapshotFromDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	err := t.storage.CreateSnapshot(ctx, request.DstSnapshotId)
	if err != nil {
		return err
	}

	if len(request.BaseSnapshotId) != 0 {
		// TODO: NBS-2999: current implementation generates garbage chunks in
		// some cases. Fix it.
		err = t.storage.ShallowCopySnapshot(
			ctx,
			request.BaseSnapshotId,
			request.DstSnapshotId,
			t.state.ShallowCopyMilestoneChunkIndex,
			func(ctx context.Context, milestoneChunkIndex uint32) error {
				_, err := t.storage.CheckSnapshotReady(ctx, request.BaseSnapshotId)
				if err != nil {
					return err
				}

				err = t.storage.CheckSnapshotAlive(ctx, request.DstSnapshotId)
				if err != nil {
					return err
				}

				t.state.ShallowCopyMilestoneChunkIndex = milestoneChunkIndex
				return execCtx.SaveState(ctx)
			},
		)
		if err != nil {
			return err
		}

		_, err := t.storage.CheckSnapshotReady(ctx, request.BaseSnapshotId)
		if err != nil {
			return err
		}
	}

	source, err := nbs.CreateDiskSource(
		ctx,
		t.nbsFactory,
		request.SrcDisk,
		request.SrcDiskBaseCheckpointId,
		request.SrcDiskCheckpointId,
		chunkSize,
		t.config.GetUseGetChangedBlocks(),
	)
	if err != nil {
		return err
	}
	defer source.Close(ctx)

	chunkCount, err := source.GetChunkCount(ctx)
	if err != nil {
		return err
	}

	t.state.ChunkCount = chunkCount

	ignoreZeroChunks := true
	rewriteChunks := false

	if len(request.BaseSnapshotId) != 0 {
		// Don't ignore zero chunks for incremental snapshots.
		ignoreZeroChunks = false
		rewriteChunks = true
	}

	target := snapshot.CreateSnapshotTarget(
		execCtx.GetTaskID(),
		request.DstSnapshotId,
		t.storage,
		ignoreZeroChunks,
		rewriteChunks,
		request.UseS3,
	)
	defer target.Close(ctx)

	err = common.Transfer(
		ctx,
		source,
		target,
		t.state.TransferMilestoneChunkIndex,
		func(ctx context.Context) error {
			err = t.storage.CheckSnapshotAlive(ctx, request.DstSnapshotId)
			if err != nil {
				return err
			}

			t.state.TransferMilestoneChunkIndex = source.GetMilestoneChunkIndex()
			t.updateProgress()

			return execCtx.SaveState(ctx)
		},
		int(t.config.GetReaderCount()),
		int(t.config.GetWriterCount()),
		int(t.config.GetChunksInflightLimit()),
		chunkSize,
	)
	if err != nil {
		return err
	}

	t.state.TransferMilestoneChunkIndex = t.state.ChunkCount
	t.state.Progress = 1

	dataChunkCount, err := t.storage.GetDataChunkCount(
		ctx,
		request.DstSnapshotId,
	)
	if err != nil {
		return err
	}

	size := uint64(chunkCount) * chunkSize
	storageSize := dataChunkCount * chunkSize

	t.state.SnapshotSize = size
	t.state.SnapshotStorageSize = storageSize

	return t.storage.SnapshotCreated(
		ctx,
		request.DstSnapshotId,
		size,
		storageSize,
		t.state.ChunkCount,
	)
}

func (t *createSnapshotFromDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.storage.DeletingSnapshot(ctx, t.state.Request.DstSnapshotId)
}

func (t *createSnapshotFromDiskTask) updateProgress() {
	if t.state.Progress == 1 {
		return
	}

	if t.state.ChunkCount != 0 {
		t.state.Progress =
			float64(t.state.TransferMilestoneChunkIndex) / float64(t.state.ChunkCount)
	}
}

func (t *createSnapshotFromDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &protos.CreateSnapshotFromDiskMetadata{
		Progress: t.state.Progress,
	}, nil
}

func (t *createSnapshotFromDiskTask) GetResponse() proto.Message {
	return &protos.CreateSnapshotFromDiskResponse{
		SnapshotSize:        t.state.SnapshotSize,
		SnapshotStorageSize: t.state.SnapshotStorageSize,
	}
}
