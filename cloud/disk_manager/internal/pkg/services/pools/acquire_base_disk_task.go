package pools

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type acquireBaseDiskTask struct {
	scheduler tasks.Scheduler
	storage   storage.Storage
	state     *protos.AcquireBaseDiskTaskState
}

func (t *acquireBaseDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.AcquireBaseDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.AcquireBaseDiskTaskState{
		Request:  typedRequest,
		BaseDisk: &types.Disk{},
	}
	t.state = state
	return nil
}

func (t *acquireBaseDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *acquireBaseDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.AcquireBaseDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *acquireBaseDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	logging.Debug(
		ctx,
		"acquireBaseDiskTask.Run imageID=%v overlayDisk=%v",
		request.SrcImageId,
		request.OverlayDisk,
	)

	baseDisk, err := t.storage.AcquireBaseDiskSlot(
		ctx,
		request.SrcImageId,
		storage.Slot{
			OverlayDisk:     request.OverlayDisk,
			OverlayDiskKind: request.OverlayDiskKind,
			OverlayDiskSize: request.OverlayDiskSize,
		},
	)
	if err != nil {
		return err
	}

	logging.Debug(
		ctx,
		"acquireBaseDiskTask.Run overlayDisk=%v acquired slot on baseDisk=%v",
		request.OverlayDisk,
		baseDisk,
	)

	t.state.BaseDisk = &types.Disk{
		ZoneId: baseDisk.ZoneID,
		DiskId: baseDisk.ID,
	}
	t.state.BaseDiskCheckpointId = baseDisk.CheckpointID

	err = execCtx.SaveState(ctx)
	if err != nil {
		return err
	}

	if baseDisk.Ready {
		logging.Debug(
			ctx,
			"acquireBaseDiskTask.Run imageID=%v overlayDisk=%v no need to create new disk",
			request.SrcImageId,
			request.OverlayDisk,
		)

		return nil
	}

	logging.Debug(
		ctx,
		"acquireBaseDiskTask.Run imageID=%v overlayDisk=%v waiting for new disk created by %v",
		request.SrcImageId,
		request.OverlayDisk,
		baseDisk.CreateTaskID,
	)
	_, err = t.scheduler.WaitTaskAsync(
		ctx,
		execCtx,
		baseDisk.CreateTaskID,
	)
	return err
}

func (t *acquireBaseDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	overlayDisk := t.state.Request.OverlayDisk

	logging.Debug(
		ctx,
		"acquireBaseDiskTask.Cancel imageID=%v overlayDisk=%v",
		t.state.Request.SrcImageId,
		overlayDisk,
	)

	_, err := t.storage.ReleaseBaseDiskSlot(
		ctx,
		overlayDisk,
	)
	return err
}

func (t *acquireBaseDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *acquireBaseDiskTask) GetResponse() proto.Message {
	return &protos.AcquireBaseDiskResponse{
		BaseDiskId:           t.state.BaseDisk.DiskId,
		BaseDiskCheckpointId: t.state.BaseDiskCheckpointId,
	}
}
