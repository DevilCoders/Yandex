package disks

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	dataplane_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/performance"
	performance_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/performance/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer"
	transfer_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type createDiskFromImageTask struct {
	performanceConfig *performance_config.PerformanceConfig
	storage           resources.Storage
	scheduler         tasks.Scheduler
	transferService   transfer.Service
	nbsFactory        nbs.Factory
	state             *protos.CreateDiskFromImageTaskState
}

func (t *createDiskFromImageTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.CreateDiskFromImageRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.CreateDiskFromImageTaskState{
		Request: typedRequest,
	}

	return nil
}

func (t *createDiskFromImageTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *createDiskFromImageTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CreateDiskFromImageTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *createDiskFromImageTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request
	params := request.Params

	client, err := t.nbsFactory.GetClient(ctx, params.Disk.ZoneId)
	if err != nil {
		return err
	}

	selfTaskID := execCtx.GetTaskID()

	diskMeta, err := t.storage.CreateDisk(ctx, resources.DiskMeta{
		ID:          params.Disk.DiskId,
		ZoneID:      params.Disk.ZoneId,
		SrcImageID:  request.SrcImageId,
		BlocksCount: params.BlocksCount,
		BlockSize:   params.BlockSize,
		Kind:        diskKindToString(params.Kind),
		CloudID:     params.CloudId,
		FolderID:    params.FolderId,

		CreateRequest: request,
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

	err = client.Create(ctx, nbs.CreateDiskParams{
		ID:               params.Disk.DiskId,
		BlocksCount:      params.BlocksCount,
		BlockSize:        params.BlockSize,
		Kind:             params.Kind,
		CloudID:          params.CloudId,
		FolderID:         params.FolderId,
		TabletVersion:    params.TabletVersion,
		PlacementGroupID: params.PlacementGroupId,
		StoragePoolName:  params.StoragePoolName,
		AgentIds:         params.AgentIds,
	})
	if err != nil {
		return err
	}

	imageMeta, err := t.storage.GetImageMeta(ctx, request.SrcImageId)
	if err != nil {
		return err
	}

	if imageMeta != nil {
		execCtx.SetEstimate(performance.Estimate(
			imageMeta.StorageSize,
			t.performanceConfig.GetCreateDiskFromImageBandwidthMiBs(),
		))
	}

	var taskID string

	// Old images without metadata we consider as not dataplane.
	if imageMeta != nil && imageMeta.UseDataplaneTasks {
		taskID, err = t.scheduler.ScheduleZonalTask(
			headers.SetIncomingIdempotencyKey(ctx, selfTaskID),
			"dataplane.TransferFromSnapshotToDisk",
			"",
			params.Disk.ZoneId,
			&dataplane_protos.TransferFromSnapshotToDiskRequest{
				SrcSnapshotId: request.SrcImageId,
				DstDisk:       params.Disk,
			},
			request.OperationCloudId,
			request.OperationFolderId,
		)

		t.state.DataplaneTaskId = taskID
	} else if request.UseDataplaneTasksForLegacySnapshots {
		taskID, err = t.scheduler.ScheduleZonalTask(
			headers.SetIncomingIdempotencyKey(ctx, selfTaskID),
			"dataplane.TransferFromLegacySnapshotToDisk",
			"",
			params.Disk.ZoneId,
			&dataplane_protos.TransferFromSnapshotToDiskRequest{
				SrcSnapshotId: request.SrcImageId,
				DstDisk:       params.Disk,
			},
			request.OperationCloudId,
			request.OperationFolderId,
		)

		t.state.DataplaneTaskId = taskID
	} else {
		taskID, err = t.transferService.TransferFromImageToDisk(
			headers.SetIncomingIdempotencyKey(ctx, selfTaskID),
			&transfer_protos.TransferFromImageToDiskRequest{
				SrcImageId:        request.SrcImageId,
				Dst:               params.Disk,
				OperationCloudId:  request.OperationCloudId,
				OperationFolderId: request.OperationFolderId,
			},
		)

		t.state.TransferTaskId = taskID
	}
	if err != nil {
		return err
	}

	err = execCtx.SaveState(ctx)
	if err != nil {
		return err
	}

	_, err = t.scheduler.WaitTaskAsync(ctx, execCtx, taskID)
	if err != nil {
		return err
	}

	diskMeta.CreatedAt = time.Now()
	return t.storage.DiskCreated(ctx, *diskMeta)
}

func (t *createDiskFromImageTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	params := t.state.Request.Params

	client, err := t.nbsFactory.GetClient(ctx, params.Disk.ZoneId)
	if err != nil {
		return err
	}

	selfTaskID := execCtx.GetTaskID()

	disk, err := t.storage.DeleteDisk(
		ctx,
		params.Disk.DiskId,
		selfTaskID,
		time.Now(),
	)
	if err != nil {
		return err
	}

	if disk == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", params.Disk.DiskId),
		}
	}

	err = client.Delete(ctx, params.Disk.DiskId)
	if err != nil {
		return err
	}

	return t.storage.DiskDeleted(ctx, params.Disk.DiskId, time.Now())
}

func (t *createDiskFromImageTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	metadata := &disk_manager.CreateDiskMetadata{}

	if len(t.state.TransferTaskId) != 0 {
		message, err := t.scheduler.GetTaskMetadata(
			ctx,
			t.state.TransferTaskId,
		)
		if err != nil {
			return nil, err
		}

		transferMetadata, ok := message.(*transfer_protos.TransferFromImageToDiskMetadata)
		if ok {
			metadata.Progress = transferMetadata.Progress
		}
	} else if len(t.state.DataplaneTaskId) != 0 {
		message, err := t.scheduler.GetTaskMetadata(
			ctx,
			t.state.DataplaneTaskId,
		)
		if err != nil {
			return nil, err
		}

		transferMetadata, ok := message.(*dataplane_protos.TransferFromSnapshotToDiskMetadata)
		if ok {
			metadata.Progress = transferMetadata.Progress
		}
	}

	return metadata, nil
}

func (t *createDiskFromImageTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
