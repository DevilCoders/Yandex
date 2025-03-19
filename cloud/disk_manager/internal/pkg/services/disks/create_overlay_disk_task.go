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
)

////////////////////////////////////////////////////////////////////////////////

type createOverlayDiskTask struct {
	storage     resources.Storage
	scheduler   tasks.Scheduler
	poolService pools.Service
	nbsFactory  nbs.Factory
	state       *protos.CreateOverlayDiskTaskState
}

func (t *createOverlayDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.CreateOverlayDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	// TODO: Sync constant with pools service.
	if typedRequest.Params.BlockSize != 4096 {
		return fmt.Errorf("invalid BlockSize=%v", typedRequest.Params.BlockSize)
	}

	t.state = &protos.CreateOverlayDiskTaskState{
		Request: typedRequest,
	}

	return nil
}

func (t *createOverlayDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *createOverlayDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CreateOverlayDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *createOverlayDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request
	params := request.Params
	overlayDisk := params.Disk

	client, err := t.nbsFactory.GetClient(ctx, overlayDisk.ZoneId)
	if err != nil {
		return err
	}

	selfTaskID := execCtx.GetTaskID()

	diskMeta, err := t.storage.CreateDisk(ctx, resources.DiskMeta{
		ID:          overlayDisk.DiskId,
		ZoneID:      overlayDisk.ZoneId,
		SrcImageID:  request.SrcImageId,
		BlocksCount: params.BlocksCount,
		BlockSize:   params.BlockSize,
		Kind:        diskKindToString(params.Kind),
		CloudID:     params.CloudId,
		FolderID:    params.FolderId,

		CreateRequest: params,
		CreateTaskID:  selfTaskID,
		CreatingAt:    time.Now(),
		CreatedBy:     "", // TODO: Extract CreatedBy from execCtx
	})
	if err != nil {
		return err
	}

	if diskMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", params.Disk.DiskId),
		}
	}

	taskID, err := t.poolService.AcquireBaseDisk(
		headers.SetIncomingIdempotencyKey(ctx, selfTaskID+"_acquire"),
		&pools_protos.AcquireBaseDiskRequest{
			SrcImageId:      request.SrcImageId,
			OverlayDisk:     overlayDisk,
			OverlayDiskKind: params.Kind,
			OverlayDiskSize: uint64(params.BlockSize) * params.BlocksCount,
		},
	)
	if err != nil {
		return err
	}

	acquireResponse, err := t.scheduler.WaitTaskAsync(ctx, execCtx, taskID)
	if err != nil {
		return err
	}

	typedAcquireResponse, ok := acquireResponse.(*pools_protos.AcquireBaseDiskResponse)
	if !ok {
		return fmt.Errorf("invalid acquire response type: %v", acquireResponse)
	}

	baseDiskID := typedAcquireResponse.BaseDiskId
	if baseDiskID == "" {
		return fmt.Errorf("baseDiskID should not be empty")
	}

	baseDiskCheckpointID := typedAcquireResponse.BaseDiskCheckpointId
	if baseDiskCheckpointID == "" {
		return fmt.Errorf("baseDiskCheckpointID should not be empty")
	}

	err = client.Create(ctx, nbs.CreateDiskParams{
		ID:                   overlayDisk.DiskId,
		BaseDiskID:           baseDiskID,
		BaseDiskCheckpointID: baseDiskCheckpointID,
		BlocksCount:          params.BlocksCount,
		BlockSize:            params.BlockSize,
		Kind:                 params.Kind,
		CloudID:              params.CloudId,
		FolderID:             params.FolderId,
		TabletVersion:        params.TabletVersion,
		PlacementGroupID:     params.PlacementGroupId,
		StoragePoolName:      params.StoragePoolName,
		AgentIds:             params.AgentIds,
	})
	if err != nil {
		return err
	}

	diskMeta.BaseDiskID = baseDiskID
	diskMeta.BaseDiskCheckpointID = baseDiskCheckpointID
	diskMeta.CreatedAt = time.Now()

	return t.storage.DiskCreated(ctx, *diskMeta)
}

func (t *createOverlayDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	params := t.state.Request.Params
	overlayDisk := params.Disk

	client, err := t.nbsFactory.GetClient(ctx, overlayDisk.ZoneId)
	if err != nil {
		return err
	}

	selfTaskID := execCtx.GetTaskID()

	disk, err := t.storage.DeleteDisk(
		ctx,
		overlayDisk.DiskId,
		selfTaskID,
		time.Now(),
	)
	if err != nil {
		return err
	}

	if disk == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", overlayDisk.DiskId),
		}
	}

	err = client.Delete(ctx, overlayDisk.DiskId)
	if err != nil {
		return err
	}

	taskID, err := t.poolService.ReleaseBaseDisk(
		headers.SetIncomingIdempotencyKey(ctx, selfTaskID+"_release"),
		&pools_protos.ReleaseBaseDiskRequest{
			OverlayDisk: overlayDisk,
		},
	)
	if err != nil {
		return err
	}

	_, err = t.scheduler.WaitTaskAsync(ctx, execCtx, taskID)
	if err != nil {
		return err
	}

	return t.storage.DiskDeleted(ctx, overlayDisk.DiskId, time.Now())
}

func (t *createOverlayDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &disk_manager.CreateDiskMetadata{}, nil
}

func (t *createOverlayDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
