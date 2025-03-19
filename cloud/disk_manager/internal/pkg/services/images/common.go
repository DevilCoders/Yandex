package images

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	dataplane_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools"
	pools_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

func deleteImage(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
	scheduler tasks.Scheduler,
	storage resources.Storage,
	snapshotClient snapshot.Client,
	poolService pools.Service,
	imageID string,
	operationCloudID string,
	operationFolderID string,
	snapshotHeartbeatTimeout time.Duration,
	snapshotPingPeriod time.Duration,
	snapshotBackoffTimeout time.Duration,
) error {

	selfTaskID := execCtx.GetTaskID()

	taskID, err := poolService.ImageDeleting(
		headers.SetIncomingIdempotencyKey(ctx, selfTaskID+"_image_deleting"),
		&pools_protos.ImageDeletingRequest{
			ImageId: imageID,
		},
	)
	if err != nil {
		return err
	}

	_, err = scheduler.WaitTaskAsync(ctx, execCtx, taskID)
	if err != nil {
		return err
	}

	imageMeta, err := storage.DeleteImage(
		ctx,
		imageID,
		selfTaskID,
		time.Now(),
	)
	if err != nil {
		return err
	}

	if imageMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", imageID),
		}
	}

	// Hack for NBS-2225.
	if imageMeta.DeleteTaskID != selfTaskID {
		return scheduler.WaitTaskEnded(ctx, imageMeta.DeleteTaskID)
	}

	if imageMeta.UseDataplaneTasks {
		taskID, err := scheduler.ScheduleTask(
			headers.SetIncomingIdempotencyKey(ctx, selfTaskID+"_delete"),
			"dataplane.DeleteSnapshot",
			"",
			&dataplane_protos.DeleteSnapshotRequest{
				SnapshotId: imageID,
			},
			operationCloudID,
			operationFolderID,
		)
		if err != nil {
			return err
		}

		_, err = scheduler.WaitTaskAsync(ctx, execCtx, taskID)
		if err != nil {
			return err
		}
	} else {
		err = snapshotClient.DeleteImage(
			ctx,
			snapshot.DeleteImageState{
				ImageID:           imageID,
				OperationCloudID:  operationCloudID,
				OperationFolderID: operationFolderID,
				State: snapshot.TaskState{
					TaskID:           selfTaskID + "_delete",
					HeartbeatTimeout: snapshotHeartbeatTimeout,
					PingPeriod:       snapshotPingPeriod,
					BackoffTimeout:   snapshotBackoffTimeout,
				},
			},
			func(offset int64, progress float64) error {
				return execCtx.SaveState(ctx)
			},
		)
		if err != nil {
			return err
		}
	}

	return storage.ImageDeleted(ctx, imageID, time.Now())
}

////////////////////////////////////////////////////////////////////////////////

func configureImagePools(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
	scheduler tasks.Scheduler,
	poolService pools.Service,
	dstImageID string,
	diskPools []*types.DiskPool,
) error {

	configurePoolTasks := make([]string, 0)

	for _, c := range diskPools {
		idempotencyKey := execCtx.GetTaskID() + "_" + dstImageID + "_" + c.ZoneId

		taskID, err := poolService.ConfigurePool(
			headers.SetIncomingIdempotencyKey(ctx, idempotencyKey),
			&pools_protos.ConfigurePoolRequest{
				ImageId:  dstImageID,
				ZoneId:   c.ZoneId,
				Capacity: c.Capacity,
			},
		)
		if err != nil {
			return err
		}

		configurePoolTasks = append(configurePoolTasks, taskID)
	}

	for _, taskID := range configurePoolTasks {
		_, err := scheduler.WaitTaskAsync(ctx, execCtx, taskID)
		if err != nil {
			return err
		}
	}

	return nil
}
