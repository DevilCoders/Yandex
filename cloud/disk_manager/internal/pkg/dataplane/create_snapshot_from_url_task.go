package dataplane

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"

	nbs_client "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/url"
)

////////////////////////////////////////////////////////////////////////////////

type createSnapshotFromURLTask struct {
	nbsFactory nbs_client.Factory
	storage    storage.Storage
	config     *config.DataplaneConfig
	state      *protos.CreateSnapshotFromURLTaskState
}

func (t *createSnapshotFromURLTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.CreateSnapshotFromURLRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type: %T", request, request)
	}

	state := &protos.CreateSnapshotFromURLTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *createSnapshotFromURLTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *createSnapshotFromURLTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CreateSnapshotFromURLTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *createSnapshotFromURLTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	err := t.storage.CreateSnapshot(ctx, request.DstSnapshotId)
	if err != nil {
		return err
	}

	source, err := url.CreateURLSource(
		ctx,
		request.SrcURL,
		chunkSize,
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

	target := snapshot.CreateSnapshotTarget(
		execCtx.GetTaskID(),
		request.DstSnapshotId,
		t.storage,
		true,  // ignoreZeroChunks
		false, // rewriteChunks
		request.UseS3,
	)
	defer target.Close(ctx)

	err = common.Transfer(
		ctx,
		source,
		target,
		t.state.MilestoneChunkIndex,
		func(ctx context.Context) error {
			err = t.storage.CheckSnapshotAlive(ctx, request.DstSnapshotId)
			if err != nil {
				return err
			}

			t.state.MilestoneChunkIndex = source.GetMilestoneChunkIndex()
			t.updateProgress()
			return execCtx.SaveState(ctx)
		},
		int(t.config.GetCreateSnapshotFromURLReaderCount()),
		int(t.config.GetWriterCount()),
		int(t.config.GetChunksInflightLimit()),
		chunkSize,
	)
	if err != nil {
		return err
	}

	t.state.MilestoneChunkIndex = t.state.ChunkCount
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

func (t *createSnapshotFromURLTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.storage.DeletingSnapshot(ctx, t.state.Request.DstSnapshotId)
}

func (t *createSnapshotFromURLTask) updateProgress() {
	if t.state.Progress == 1 {
		return
	}

	if t.state.ChunkCount != 0 {
		t.state.Progress =
			float64(t.state.MilestoneChunkIndex) / float64(t.state.ChunkCount)
	}
}

func (t *createSnapshotFromURLTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &protos.CreateSnapshotFromURLMetadata{
		Progress: t.state.Progress,
	}, nil
}

func (t *createSnapshotFromURLTask) GetResponse() proto.Message {
	return &protos.CreateSnapshotFromURLResponse{
		SnapshotSize:        t.state.SnapshotSize,
		SnapshotStorageSize: t.state.SnapshotStorageSize,
	}
}
