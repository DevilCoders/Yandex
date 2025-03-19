package disks

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools"
	pools_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type deleteDiskTask struct {
	storage     resources.Storage
	scheduler   tasks.Scheduler
	poolService pools.Service
	nbsFactory  nbs.Factory
	state       *protos.DeleteDiskTaskState
}

func (t *deleteDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.DeleteDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.DeleteDiskTaskState{
		Request: typedRequest,
	}

	return nil
}

func (t *deleteDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *deleteDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.DeleteDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *deleteDiskTask) deleteDisk(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	selfTaskID := execCtx.GetTaskID()
	request := t.state.Request
	diskID := request.Disk.DiskId

	diskMeta, err := t.storage.DeleteDisk(
		ctx,
		diskID,
		selfTaskID,
		time.Now(),
	)
	if err != nil {
		return err
	}

	if diskMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", diskID),
		}
	}

	zoneID := diskMeta.ZoneID
	if len(zoneID) == 0 {
		zoneID = request.Disk.ZoneId
	}

	client, err := t.nbsFactory.GetClient(ctx, zoneID)
	if err != nil {
		return err
	}

	err = client.Delete(ctx, diskID)
	if err != nil {
		return err
	}

	var taskID string

	// Only overlay disks (created from image) should be released.
	if len(diskMeta.SrcImageID) != 0 {
		taskID, err = t.poolService.ReleaseBaseDisk(
			headers.SetIncomingIdempotencyKey(ctx, selfTaskID),
			&pools_protos.ReleaseBaseDiskRequest{
				OverlayDisk: &types.Disk{
					ZoneId: zoneID,
					DiskId: diskID,
				},
			},
		)
		if err != nil {
			return err
		}

		_, err = t.scheduler.WaitTaskAsync(ctx, execCtx, taskID)
		if err != nil {
			return err
		}
	}

	return t.storage.DiskDeleted(ctx, diskID, time.Now())
}

func (t *deleteDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	err := t.deleteDisk(ctx, execCtx)
	return errors.MakeRetriable(err, true /* ignoreRetryLimit */)
}

func (t *deleteDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.deleteDisk(ctx, execCtx)
}

func (t *deleteDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &disk_manager.DeleteDiskMetadata{
		DiskId: &disk_manager.DiskId{
			ZoneId: t.state.Request.Disk.ZoneId,
			DiskId: t.state.Request.Disk.DiskId,
		},
	}, nil
}

func (t *deleteDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
