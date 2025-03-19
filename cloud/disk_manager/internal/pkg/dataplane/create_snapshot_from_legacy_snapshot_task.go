package dataplane

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type createSnapshotFromLegacySnapshotTask struct {
	storage       storage.Storage
	legacyStorage storage.Storage
	config        *config.DataplaneConfig
	state         *protos.CreateSnapshotFromLegacySnapshotTaskState
}

func (t *createSnapshotFromLegacySnapshotTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.CreateSnapshotFromLegacySnapshotRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.CreateSnapshotFromLegacySnapshotTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *createSnapshotFromLegacySnapshotTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *createSnapshotFromLegacySnapshotTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CreateSnapshotFromLegacySnapshotTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *createSnapshotFromLegacySnapshotTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	srcMeta, err := t.legacyStorage.CheckSnapshotReady(ctx, request.SrcSnapshotId)
	if err != nil {
		return err
	}

	t.state.ChunkCount = srcMeta.ChunkCount

	err = t.storage.CreateSnapshot(ctx, request.DstSnapshotId)
	if err != nil {
		return err
	}

	source := snapshot.CreateSnapshotSource(request.SrcSnapshotId, t.legacyStorage)
	defer source.Close(ctx)

	ignoreZeroChunks := true
	rewriteChunks := false

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
		t.state.MilestoneChunkIndex,
		func(ctx context.Context) error {
			_, err := t.legacyStorage.CheckSnapshotReady(ctx, request.SrcSnapshotId)
			if err != nil {
				return err
			}

			err = t.storage.CheckSnapshotAlive(ctx, request.DstSnapshotId)
			if err != nil {
				return err
			}

			t.state.MilestoneChunkIndex = source.GetMilestoneChunkIndex()
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

	t.state.MilestoneChunkIndex = t.state.ChunkCount
	t.state.Progress = 1

	srcMeta, err = t.legacyStorage.CheckSnapshotReady(ctx, request.SrcSnapshotId)
	if err != nil {
		return err
	}

	dataChunkCount, err := t.storage.GetDataChunkCount(
		ctx,
		request.DstSnapshotId,
	)
	if err != nil {
		return err
	}

	size := srcMeta.Size
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

func (t *createSnapshotFromLegacySnapshotTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.storage.DeletingSnapshot(ctx, t.state.Request.DstSnapshotId)
}

func (t *createSnapshotFromLegacySnapshotTask) updateProgress() {
	if t.state.Progress == 1 {
		return
	}

	if t.state.ChunkCount != 0 {
		t.state.Progress =
			float64(t.state.MilestoneChunkIndex) / float64(t.state.ChunkCount)
	}
}

func (t *createSnapshotFromLegacySnapshotTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &protos.CreateSnapshotFromLegacySnapshotMetadata{
		Progress: t.state.Progress,
	}, nil
}

func (t *createSnapshotFromLegacySnapshotTask) GetResponse() proto.Message {
	return &protos.CreateSnapshotFromLegacySnapshotResponse{
		SnapshotSize:        t.state.SnapshotSize,
		SnapshotStorageSize: t.state.SnapshotStorageSize,
	}
}
