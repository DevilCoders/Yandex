package snapshots

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/common"
	dataplane_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/performance"
	performance_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/performance/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/snapshots/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type createSnapshotFromDiskTask struct {
	performanceConfig        *performance_config.PerformanceConfig
	scheduler                tasks.Scheduler
	storage                  resources.Storage
	nbsFactory               nbs.Factory
	snapshotFactory          snapshot.Factory
	snapshotPingPeriod       time.Duration
	snapshotHeartbeatTimeout time.Duration
	snapshotBackoffTimeout   time.Duration
	state                    *protos.CreateSnapshotFromDiskTaskState
}

func (t *createSnapshotFromDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.CreateSnapshotFromDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.CreateSnapshotFromDiskTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *createSnapshotFromDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *createSnapshotFromDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CreateSnapshotFromDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *createSnapshotFromDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request
	disk := request.SrcDisk
	selfTaskID := execCtx.GetTaskID()
	checkpointID := request.DstSnapshotId

	nbsClient, err := t.nbsFactory.GetClient(ctx, disk.ZoneId)
	if err != nil {
		return err
	}

	snapshotClient, err := t.snapshotFactory.CreateClientFromZone(
		ctx,
		disk.ZoneId,
	)
	if err != nil {
		return err
	}
	defer snapshotClient.Close()

	snapshotMeta, err := t.storage.CreateSnapshot(ctx, resources.SnapshotMeta{
		ID:                request.DstSnapshotId,
		FolderID:          request.FolderId,
		Disk:              disk,
		CheckpointID:      checkpointID,
		CreateRequest:     request,
		CreateTaskID:      selfTaskID,
		CreatingAt:        time.Now(),
		CreatedBy:         "", // TODO: Extract CreatedBy from execCtx
		Incremental:       request.Incremental,
		UseDataplaneTasks: request.UseDataplaneTasks,
	})
	if err != nil {
		return err
	}

	if snapshotMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", request.DstSnapshotId),
		}
	}

	err = nbsClient.CreateCheckpoint(ctx, disk.DiskId, checkpointID)
	if err != nil {
		return err
	}

	if snapshotMeta.UseDataplaneTasks {
		baseSnapshotID := snapshotMeta.BaseSnapshotID
		baseCheckpointID := snapshotMeta.BaseCheckpointID

		if len(snapshotMeta.BaseSnapshotID) != 0 {
			// Lock base snapshot to prevent deletion.
			locked, err := t.storage.LockSnapshot(
				ctx,
				snapshotMeta.BaseSnapshotID,
				selfTaskID,
			)
			if err != nil {
				return err
			}

			if locked {
				logging.Debug(
					ctx,
					"Successfully locked snapshot with id=%v",
					snapshotMeta.BaseSnapshotID,
				)
			} else {
				logging.Info(
					ctx,
					"Snapshot with id=%v can't be locked",
					snapshotMeta.BaseSnapshotID,
				)

				// Should perform full snapshot of disk.
				baseSnapshotID = ""
				baseCheckpointID = ""
			}
		}

		taskID, err := t.scheduler.ScheduleZonalTask(
			headers.SetIncomingIdempotencyKey(ctx, selfTaskID+"_run"),
			"dataplane.CreateSnapshotFromDisk",
			"",
			disk.ZoneId,
			&dataplane_protos.CreateSnapshotFromDiskRequest{
				SrcDisk:                 disk,
				SrcDiskBaseCheckpointId: baseCheckpointID,
				SrcDiskCheckpointId:     checkpointID,
				BaseSnapshotId:          baseSnapshotID,
				DstSnapshotId:           request.DstSnapshotId,
				UseS3:                   request.UseS3,
			},
			request.OperationCloudId,
			request.OperationFolderId,
		)
		if err != nil {
			return err
		}

		t.state.DataplaneTaskID = taskID

		response, err := t.scheduler.WaitTaskAsync(ctx, execCtx, taskID)
		if err != nil {
			return err
		}

		typedResponse, ok := response.(*dataplane_protos.CreateSnapshotFromDiskResponse)
		if !ok {
			return fmt.Errorf("invalid create snapshot response type: %v", typedResponse)
		}

		// TODO: estimate should be applied before resource creation, not after.
		execCtx.SetEstimate(performance.Estimate(
			typedResponse.SnapshotStorageSize,
			t.performanceConfig.GetCreateSnapshotFromDiskBandwidthMiBs(),
		))

		t.state.SnapshotSize = int64(typedResponse.SnapshotSize)
		t.state.SnapshotStorageSize = int64(typedResponse.SnapshotStorageSize)
	} else {
		info, err := snapshotClient.CreateSnapshotFromDisk(
			ctx,
			snapshot.CreateSnapshotFromDiskState{
				SrcDisk:                 disk,
				SrcDiskCheckpointID:     checkpointID,
				SrcDiskBaseCheckpointID: snapshotMeta.BaseCheckpointID,
				DstSnapshotID:           request.DstSnapshotId,
				DstBaseSnapshotID:       snapshotMeta.BaseSnapshotID,
				FolderID:                request.FolderId,
				OperationCloudID:        request.OperationCloudId,
				OperationFolderID:       request.OperationFolderId,
				State: snapshot.TaskState{
					TaskID:           selfTaskID + "_run",
					Offset:           t.state.Offset,
					HeartbeatTimeout: t.snapshotHeartbeatTimeout,
					PingPeriod:       t.snapshotPingPeriod,
					BackoffTimeout:   t.snapshotBackoffTimeout,
				},
			},
			func(offset int64, progress float64) error {
				common.Assert(
					t.state.Offset <= offset,
					fmt.Sprintf(
						"offset should not run backward: %v > %v, taskID=%v",
						offset,
						t.state.Offset,
						selfTaskID,
					),
				)

				t.state.Offset = offset
				t.state.Progress = progress
				return execCtx.SaveState(ctx)
			},
		)
		if err != nil {
			return err
		}

		t.state.SnapshotSize = info.Size
		t.state.SnapshotStorageSize = info.StorageSize
		t.state.Progress = 1
	}

	err = t.storage.SnapshotCreated(
		ctx,
		request.DstSnapshotId,
		time.Now(),
		uint64(t.state.SnapshotSize),
		uint64(t.state.SnapshotStorageSize),
	)
	if err != nil {
		return err
	}

	if snapshotMeta.UseDataplaneTasks && len(snapshotMeta.BaseSnapshotID) != 0 {
		err := t.storage.UnlockSnapshot(
			ctx,
			snapshotMeta.BaseSnapshotID,
			selfTaskID,
		)
		if err != nil {
			return err
		}
	}

	if request.Incremental {
		err := nbsClient.DeleteCheckpointData(ctx, disk.DiskId, checkpointID)
		if err != nil {
			return err
		}

		if len(snapshotMeta.BaseCheckpointID) == 0 {
			return nil
		}

		return nbsClient.DeleteCheckpoint(
			ctx,
			disk.DiskId,
			snapshotMeta.BaseCheckpointID,
		)
	}

	return nbsClient.DeleteCheckpoint(ctx, disk.DiskId, checkpointID)
}

func (t *createSnapshotFromDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request
	disk := request.SrcDisk

	nbsClient, err := t.nbsFactory.GetClient(ctx, disk.ZoneId)
	if err != nil {
		return err
	}

	snapshotClient, err := t.snapshotFactory.CreateClientFromZone(
		ctx,
		disk.ZoneId,
	)
	if err != nil {
		return err
	}
	defer snapshotClient.Close()

	// NOTE: we use snapshot id as checkpoint id.
	checkpointID := request.DstSnapshotId

	// NBS-1873: Should always delete checkpoint.
	err = nbsClient.DeleteCheckpoint(ctx, disk.DiskId, checkpointID)
	if err != nil {
		return err
	}

	selfTaskID := execCtx.GetTaskID()

	snapshotMeta, err := t.storage.DeleteSnapshot(
		ctx,
		request.DstSnapshotId,
		selfTaskID,
		time.Now(),
	)
	if err != nil {
		return err
	}

	if snapshotMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", request.DstSnapshotId),
		}
	}

	// NBS-3192.
	if snapshotMeta.UseDataplaneTasks && len(snapshotMeta.BaseSnapshotID) != 0 {
		err := t.storage.UnlockSnapshot(
			ctx,
			snapshotMeta.BaseSnapshotID,
			selfTaskID,
		)
		if err != nil {
			return err
		}
	}

	// Hack for NBS-2225.
	if snapshotMeta.DeleteTaskID != selfTaskID {
		return t.scheduler.WaitTaskEnded(ctx, snapshotMeta.DeleteTaskID)
	}

	if snapshotMeta.UseDataplaneTasks {
		taskID, err := t.scheduler.ScheduleTask(
			headers.SetIncomingIdempotencyKey(ctx, selfTaskID+"_cancel"),
			"dataplane.DeleteSnapshot",
			"",
			&dataplane_protos.DeleteSnapshotRequest{
				SnapshotId: request.DstSnapshotId,
			},
			request.OperationCloudId,
			request.OperationFolderId,
		)
		if err != nil {
			return err
		}

		_, err = t.scheduler.WaitTaskAsync(ctx, execCtx, taskID)
		if err != nil {
			return err
		}
	} else {
		err = snapshotClient.DeleteSnapshot(
			ctx,
			snapshot.DeleteSnapshotState{
				SnapshotID:        request.DstSnapshotId,
				OperationCloudID:  request.OperationCloudId,
				OperationFolderID: request.OperationFolderId,
				State: snapshot.TaskState{
					TaskID:           selfTaskID + "_cancel",
					HeartbeatTimeout: t.snapshotHeartbeatTimeout,
					PingPeriod:       t.snapshotPingPeriod,
					BackoffTimeout:   t.snapshotBackoffTimeout,
				},
			},
			func(offset int64, progress float64) error {
				return nil
			},
		)
		if err != nil {
			return err
		}
	}

	return t.storage.SnapshotDeleted(ctx, request.DstSnapshotId, time.Now())
}

func (t *createSnapshotFromDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	metadata := &disk_manager.CreateSnapshotMetadata{}

	if len(t.state.DataplaneTaskID) != 0 {
		message, err := t.scheduler.GetTaskMetadata(
			ctx,
			t.state.DataplaneTaskID,
		)
		if err != nil {
			return nil, err
		}

		createMetadata, ok := message.(*dataplane_protos.CreateSnapshotFromDiskMetadata)
		if ok {
			metadata.Progress = createMetadata.Progress
		}
	} else {
		metadata.Progress = t.state.Progress
	}

	return metadata, nil
}

func (t *createSnapshotFromDiskTask) GetResponse() proto.Message {
	return &disk_manager.CreateSnapshotResponse{
		Size:        t.state.SnapshotSize,
		StorageSize: t.state.SnapshotStorageSize,
	}
}
