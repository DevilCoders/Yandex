package pools

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	dataplane_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer"
	transfer_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type createBaseDiskTask struct {
	cloudID  string
	folderID string

	scheduler       tasks.Scheduler
	storage         storage.Storage
	nbsFactory      nbs.Factory
	transferService transfer.Service
	resourceStorage resources.Storage
	state           *protos.CreateBaseDiskTaskState
}

func (t *createBaseDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.CreateBaseDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.CreateBaseDiskTaskState{
		Request: typedRequest,
		Step:    0,
	}
	return nil
}

func (t *createBaseDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *createBaseDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CreateBaseDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *createBaseDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return tasks.RunSteps(t.state.Step, []tasks.StepFunc{
		func(stop *bool) error {
			baseDisk := t.state.Request.BaseDisk

			client, err := t.nbsFactory.GetClient(ctx, baseDisk.ZoneId)
			if err != nil {
				return err
			}

			baseDiskSize := t.state.Request.BaseDiskSize

			var baseDiskBlockCount uint64
			if baseDiskSize == 0 {
				baseDiskBlockCount = defaultBaseDiskBlockCount
			} else {
				if baseDiskSize%baseDiskBlockSize != 0 {
					return errors.CreateInvalidArgumentError(
						"baseDiskSize should be divisible by baseDiskBlockSize, baseDiskSize=%v, baseDiskBlockSize=%v",
						baseDiskSize,
						baseDiskBlockSize,
					)
				}

				baseDiskBlockCount = baseDiskSize / baseDiskBlockSize
			}

			return client.Create(ctx, nbs.CreateDiskParams{
				ID:              baseDisk.DiskId,
				BlocksCount:     baseDiskBlockCount,
				BlockSize:       baseDiskBlockSize,
				Kind:            types.DiskKind_DISK_KIND_SSD,
				CloudID:         t.cloudID,
				FolderID:        t.folderID,
				PartitionsCount: 1, // Should be deleted after NBS-1896.
				IsSystem:        true,
			})
		},
		func(stop *bool) (err error) {
			var taskID string

			request := t.state.Request
			zoneID := request.BaseDisk.ZoneId

			if request.SrcDisk != nil {
				taskID, err = t.scheduler.ScheduleZonalTask(
					headers.SetIncomingIdempotencyKey(ctx, execCtx.GetTaskID()),
					"dataplane.TransferFromDiskToDisk",
					"",
					zoneID,
					&dataplane_protos.TransferFromDiskToDiskRequest{
						SrcDisk:             request.SrcDisk,
						SrcDiskCheckpointId: request.SrcDiskCheckpointId,
						DstDisk:             request.BaseDisk,
					},
					t.cloudID,
					t.folderID,
				)
				if err != nil {
					return err
				}
			} else {
				image, err := t.resourceStorage.GetImageMeta(
					ctx,
					request.SrcImageId,
				)
				if err != nil {
					return err
				}

				// Old images without metadata we consider as not dataplane.
				if image != nil && image.UseDataplaneTasks {
					taskID, err = t.scheduler.ScheduleZonalTask(
						headers.SetIncomingIdempotencyKey(
							ctx,
							execCtx.GetTaskID(),
						),
						"dataplane.TransferFromSnapshotToDisk",
						"",
						zoneID,
						&dataplane_protos.TransferFromSnapshotToDiskRequest{
							SrcSnapshotId: request.SrcImageId,
							DstDisk:       request.BaseDisk,
						},
						t.cloudID,
						t.folderID,
					)
				} else if request.UseDataplaneTasksForLegacySnapshots {
					taskID, err = t.scheduler.ScheduleZonalTask(
						headers.SetIncomingIdempotencyKey(
							ctx,
							execCtx.GetTaskID(),
						),
						"dataplane.TransferFromLegacySnapshotToDisk",
						"",
						zoneID,
						&dataplane_protos.TransferFromSnapshotToDiskRequest{
							SrcSnapshotId: request.SrcImageId,
							DstDisk:       request.BaseDisk,
						},
						t.cloudID,
						t.folderID,
					)
				} else {
					taskID, err = t.transferService.TransferFromImageToDisk(
						headers.SetIncomingIdempotencyKey(
							ctx,
							execCtx.GetTaskID(),
						),
						&transfer_protos.TransferFromImageToDiskRequest{
							SrcImageId:        request.SrcImageId,
							Dst:               request.BaseDisk,
							OperationCloudId:  t.cloudID,
							OperationFolderId: t.folderID,
						},
					)
				}
				if err != nil {
					return err
				}
			}

			_, err = t.scheduler.WaitTaskAsync(ctx, execCtx, taskID)
			return err
		},
		func(stop *bool) error {
			checkpointID := t.state.Request.BaseDiskCheckpointId
			baseDisk := t.state.Request.BaseDisk

			client, err := t.nbsFactory.GetClient(ctx, baseDisk.ZoneId)
			if err != nil {
				return err
			}

			return client.CreateCheckpoint(ctx, baseDisk.DiskId, checkpointID)
		},
		func(stop *bool) error {
			baseDisk := t.state.Request.BaseDisk
			imageID := t.state.Request.SrcImageId

			return t.storage.BaseDiskCreated(ctx, storage.BaseDisk{
				ID:      baseDisk.DiskId,
				ImageID: imageID,
				ZoneID:  baseDisk.ZoneId,
			})
		}},
		func(step uint32) error {
			t.state.Step = step
			return execCtx.SaveState(ctx)
		})
}

func (t *createBaseDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	baseDisk := t.state.Request.BaseDisk
	imageID := t.state.Request.SrcImageId

	return t.storage.BaseDiskCreationFailed(ctx, storage.BaseDisk{
		ID:      baseDisk.DiskId,
		ImageID: imageID,
		ZoneID:  baseDisk.ZoneId,
	})
}

func (t *createBaseDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *createBaseDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
