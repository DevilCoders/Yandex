package dataplane

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

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

type transferFromSnapshotToDiskTask struct {
	nbsFactory nbs_client.Factory
	storage    storage.Storage
	config     *config.DataplaneConfig
	state      *protos.TransferFromSnapshotToDiskTaskState
}

func (t *transferFromSnapshotToDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.TransferFromSnapshotToDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.TransferFromSnapshotToDiskTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *transferFromSnapshotToDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *transferFromSnapshotToDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.TransferFromSnapshotToDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *transferFromSnapshotToDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	srcMeta, err := t.storage.CheckSnapshotReady(ctx, request.SrcSnapshotId)
	if err != nil {
		return err
	}

	t.state.ChunkCount = srcMeta.ChunkCount

	source := snapshot.CreateSnapshotSource(request.SrcSnapshotId, t.storage)
	defer source.Close(ctx)

	target, err := nbs.CreateDiskTarget(
		ctx,
		t.nbsFactory,
		request.DstDisk,
		chunkSize,
		false, // ignoreZeroChunks
	)
	if err != nil {
		return err
	}
	defer target.Close(ctx)

	err = common.Transfer(
		ctx,
		source,
		target,
		t.state.MilestoneChunkIndex,
		func(ctx context.Context) error {
			_, err := t.storage.CheckSnapshotReady(ctx, request.SrcSnapshotId)
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

	_, err = t.storage.CheckSnapshotReady(ctx, request.SrcSnapshotId)
	return err
}

func (t *transferFromSnapshotToDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *transferFromSnapshotToDiskTask) updateProgress() {
	if t.state.Progress == 1 {
		return
	}

	if t.state.ChunkCount != 0 {
		t.state.Progress =
			float64(t.state.MilestoneChunkIndex) / float64(t.state.ChunkCount)
	}
}

func (t *transferFromSnapshotToDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &protos.TransferFromSnapshotToDiskMetadata{
		Progress: t.state.Progress,
	}, nil
}

func (t *transferFromSnapshotToDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
