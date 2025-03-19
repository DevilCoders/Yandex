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
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type transferFromDiskToDiskTask struct {
	nbsFactory nbs_client.Factory
	config     *config.DataplaneConfig
	state      *protos.TransferFromDiskToDiskTaskState
}

func (t *transferFromDiskToDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.TransferFromDiskToDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.TransferFromDiskToDiskTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *transferFromDiskToDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *transferFromDiskToDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.TransferFromDiskToDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *transferFromDiskToDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	source, err := nbs.CreateDiskSource(
		ctx,
		t.nbsFactory,
		request.SrcDisk,
		"",
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

	target, err := nbs.CreateDiskTarget(
		ctx,
		t.nbsFactory,
		request.DstDisk,
		chunkSize,
		ignoreZeroChunks,
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
	return nil
}

func (t *transferFromDiskToDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *transferFromDiskToDiskTask) updateProgress() {
	if t.state.Progress == 1 {
		return
	}

	if t.state.ChunkCount != 0 {
		t.state.Progress =
			float64(t.state.MilestoneChunkIndex) / float64(t.state.ChunkCount)
	}
}

func (t *transferFromDiskToDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &protos.TransferFromDiskToDiskMetadata{
		Progress: t.state.Progress,
	}, nil
}

func (t *transferFromDiskToDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
