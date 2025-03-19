package pools

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type retireBaseDisksTask struct {
	scheduler tasks.Scheduler
	storage   storage.Storage
	state     *protos.RetireBaseDisksTaskState
}

func (t *retireBaseDisksTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.RetireBaseDisksRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.RetireBaseDisksTaskState{
		Request: typedRequest,
	}
	return nil
}

func (t *retireBaseDisksTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *retireBaseDisksTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.RetireBaseDisksTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *retireBaseDisksTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request
	imageID := request.ImageId
	zoneID := request.ZoneId
	useBaseDiskAsSrc := request.UseBaseDiskAsSrc

	// TODO: can't lock deleted pool, but useBaseDiskAsSrc should work even
	// when pool is deleted.
	shouldLockPool := !useBaseDiskAsSrc
	var lockID string
	if shouldLockPool {
		lockID = execCtx.GetTaskID()
	}

	if len(t.state.BaseDiskIds) == 0 {
		if shouldLockPool {
			locked, err := t.storage.LockPool(ctx, imageID, zoneID, lockID)
			if err != nil {
				return err
			}

			if !locked {
				return &errors.NonCancellableError{
					Err: fmt.Errorf(
						"failed to lock pool, imageID=%v, zoneID=%v",
						imageID,
						zoneID,
					),
				}
			}
		}

		baseDisks, err := t.storage.ListBaseDisks(ctx, imageID, zoneID)
		if err != nil {
			return err
		}

		logging.Info(
			ctx,
			"pools.RetireBaseDisks listed base disks: %v",
			baseDisks,
		)

		t.state.BaseDiskIds = make([]string, 0)
		for _, baseDisk := range baseDisks {
			t.state.BaseDiskIds = append(t.state.BaseDiskIds, baseDisk.ID)
		}

		err = execCtx.SaveState(ctx)
		if err != nil {
			return err
		}
	}

	for _, baseDiskID := range t.state.BaseDiskIds {
		idempotencyKey := fmt.Sprintf("%v_%v", execCtx.GetTaskID(), baseDiskID)

		var srcDisk *types.Disk
		var srcDiskCheckpointID string
		if t.state.Request.UseBaseDiskAsSrc {
			srcDisk = &types.Disk{
				ZoneId: zoneID,
				DiskId: baseDiskID,
			}
			// Note: we use image id as checkpoint id.
			srcDiskCheckpointID = imageID
		}

		taskID, err := t.scheduler.ScheduleTask(
			headers.SetIncomingIdempotencyKey(ctx, idempotencyKey),
			"pools.RetireBaseDisk",
			fmt.Sprintf(
				"Retire base disks from pool, imageID=%v, zoneID=%v",
				imageID,
				zoneID,
			),
			&protos.RetireBaseDiskRequest{
				BaseDiskId:          baseDiskID,
				SrcDisk:             srcDisk,
				SrcDiskCheckpointId: srcDiskCheckpointID,
			},
			"",
			"",
		)
		if err != nil {
			return err
		}

		_, err = t.scheduler.WaitTaskAsync(ctx, execCtx, taskID)
		if err != nil {
			return err
		}
	}

	return t.storage.UnlockPool(ctx, imageID, zoneID, lockID)
}

func (t *retireBaseDisksTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.storage.UnlockPool(
		ctx,
		t.state.Request.ImageId,
		t.state.Request.ZoneId,
		execCtx.GetTaskID(), // lockID
	)
}

func (t *retireBaseDisksTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *retireBaseDisksTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
