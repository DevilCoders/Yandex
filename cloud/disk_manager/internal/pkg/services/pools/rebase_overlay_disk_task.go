package pools

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type rebaseOverlayDiskTask struct {
	storage    storage.Storage
	nbsFactory nbs.Factory
	state      *protos.RebaseOverlayDiskTaskState
}

func (t *rebaseOverlayDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.RebaseOverlayDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.RebaseOverlayDiskTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *rebaseOverlayDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *rebaseOverlayDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.RebaseOverlayDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *rebaseOverlayDiskTask) rebaseOverlayDisk(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	logging.Info(
		ctx,
		"rebaseOverlayDiskTask: taskID=%v, req=%v",
		execCtx.GetTaskID(),
		t.state.Request,
	)

	client, err := t.nbsFactory.GetClient(ctx, request.OverlayDisk.ZoneId)
	if err != nil {
		return err
	}

	err = t.storage.OverlayDiskRebasing(ctx, storage.RebaseInfo{
		OverlayDisk:      request.OverlayDisk,
		BaseDiskID:       request.BaseDiskId,
		TargetBaseDiskID: request.TargetBaseDiskId,
		SlotGeneration:   request.SlotGeneration,
	})
	if err != nil {
		return err
	}

	err = client.Rebase(
		ctx,
		func() error { return execCtx.SaveState(ctx) },
		request.OverlayDisk.DiskId,
		request.BaseDiskId,
		request.TargetBaseDiskId,
	)
	if err != nil {
		return err
	}

	return t.storage.OverlayDiskRebased(ctx, storage.RebaseInfo{
		OverlayDisk:      request.OverlayDisk,
		BaseDiskID:       request.BaseDiskId,
		TargetBaseDiskID: request.TargetBaseDiskId,
		SlotGeneration:   request.SlotGeneration,
	})
}

func (t *rebaseOverlayDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return errors.MakeRetriable(
		t.rebaseOverlayDisk(ctx, execCtx),
		true, // ignoreRetryLimit
	)
}

func (t *rebaseOverlayDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *rebaseOverlayDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *rebaseOverlayDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
