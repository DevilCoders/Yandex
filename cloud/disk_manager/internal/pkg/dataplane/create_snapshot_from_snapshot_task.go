package dataplane

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type createSnapshotFromSnapshotTask struct {
	storage storage.Storage
	config  *config.DataplaneConfig
	state   *protos.CreateSnapshotFromSnapshotTaskState
}

func (t *createSnapshotFromSnapshotTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.CreateSnapshotFromSnapshotRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.CreateSnapshotFromSnapshotTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *createSnapshotFromSnapshotTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *createSnapshotFromSnapshotTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CreateSnapshotFromSnapshotTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *createSnapshotFromSnapshotTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	srcMeta, err := t.storage.CheckSnapshotReady(ctx, request.SrcSnapshotId)
	if err != nil {
		return err
	}

	t.state.ChunkCount = srcMeta.ChunkCount

	err = t.storage.CreateSnapshot(ctx, request.DstSnapshotId)
	if err != nil {
		return err
	}

	err = t.storage.ShallowCopySnapshot(
		ctx,
		request.SrcSnapshotId,
		request.DstSnapshotId,
		t.state.MilestoneChunkIndex,
		func(ctx context.Context, milestoneChunkIndex uint32) error {
			_, err := t.storage.CheckSnapshotReady(ctx, request.SrcSnapshotId)
			if err != nil {
				return err
			}

			err = t.storage.CheckSnapshotAlive(ctx, request.DstSnapshotId)
			if err != nil {
				return err
			}

			t.state.MilestoneChunkIndex = milestoneChunkIndex
			t.updateProgress()

			return execCtx.SaveState(ctx)
		},
	)
	if err != nil {
		return err
	}

	t.state.MilestoneChunkIndex = t.state.ChunkCount
	t.state.Progress = 1

	srcMeta, err = t.storage.CheckSnapshotReady(ctx, request.SrcSnapshotId)
	if err != nil {
		return err
	}

	t.state.SnapshotSize = srcMeta.Size
	t.state.SnapshotStorageSize = srcMeta.StorageSize

	return t.storage.SnapshotCreated(
		ctx,
		request.DstSnapshotId,
		srcMeta.Size,
		srcMeta.StorageSize,
		srcMeta.ChunkCount,
	)
}

func (t *createSnapshotFromSnapshotTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.storage.DeletingSnapshot(ctx, t.state.Request.DstSnapshotId)
}

func (t *createSnapshotFromSnapshotTask) updateProgress() {
	if t.state.Progress == 1 {
		return
	}

	if t.state.ChunkCount != 0 {
		t.state.Progress =
			float64(t.state.MilestoneChunkIndex) / float64(t.state.ChunkCount)
	}
}

func (t *createSnapshotFromSnapshotTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &protos.CreateSnapshotFromSnapshotMetadata{
		Progress: t.state.Progress,
	}, nil
}

func (t *createSnapshotFromSnapshotTask) GetResponse() proto.Message {
	return &protos.CreateSnapshotFromSnapshotResponse{
		SnapshotSize:        t.state.SnapshotSize,
		SnapshotStorageSize: t.state.SnapshotStorageSize,
	}
}
