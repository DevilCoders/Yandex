package snapshots

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	dataplane_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/snapshots/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type deleteSnapshotTask struct {
	scheduler                tasks.Scheduler
	storage                  resources.Storage
	nbsFactory               nbs.Factory
	snapshotFactory          snapshot.Factory
	snapshotPingPeriod       time.Duration
	snapshotHeartbeatTimeout time.Duration
	snapshotBackoffTimeout   time.Duration
	state                    *protos.DeleteSnapshotTaskState
}

func (t *deleteSnapshotTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.DeleteSnapshotRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.DeleteSnapshotTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *deleteSnapshotTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *deleteSnapshotTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.DeleteSnapshotTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *deleteSnapshotTask) deleteSnapshot(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	snapshotClient, err := t.snapshotFactory.CreateClient(ctx)
	if err != nil {
		return err
	}
	defer snapshotClient.Close()

	request := t.state.Request
	selfTaskID := execCtx.GetTaskID()

	snapshotMeta, err := t.storage.DeleteSnapshot(
		ctx,
		request.SnapshotId,
		selfTaskID,
		time.Now(),
	)
	if err != nil {
		return err
	}

	if snapshotMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", request.SnapshotId),
		}
	}

	// TODO: Cancel inflight snapshot creation.

	if snapshotMeta.Incremental {
		nbsClient, err := t.nbsFactory.GetClient(ctx, snapshotMeta.Disk.ZoneId)
		if err != nil {
			return err
		}

		err = nbsClient.DeleteCheckpoint(
			ctx,
			snapshotMeta.Disk.DiskId,
			snapshotMeta.CheckpointID,
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
			headers.SetIncomingIdempotencyKey(ctx, selfTaskID),
			"dataplane.DeleteSnapshot",
			"",
			&dataplane_protos.DeleteSnapshotRequest{
				SnapshotId: request.SnapshotId,
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
				SnapshotID:        request.SnapshotId,
				OperationCloudID:  request.OperationCloudId,
				OperationFolderID: request.OperationFolderId,
				State: snapshot.TaskState{
					TaskID:           selfTaskID,
					HeartbeatTimeout: t.snapshotHeartbeatTimeout,
					PingPeriod:       t.snapshotPingPeriod,
					BackoffTimeout:   t.snapshotBackoffTimeout,
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

	return t.storage.SnapshotDeleted(ctx, request.SnapshotId, time.Now())
}

func (t *deleteSnapshotTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	err := t.deleteSnapshot(ctx, execCtx)
	return errors.MakeRetriable(err, true /* ignoreRetryLimit */)
}

func (t *deleteSnapshotTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.deleteSnapshot(ctx, execCtx)
}

func (t *deleteSnapshotTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &disk_manager.DeleteSnapshotMetadata{
		SnapshotId: t.state.Request.SnapshotId,
	}, nil
}

func (t *deleteSnapshotTask) GetResponse() proto.Message {

	// TODO: Fill response with data.
	return &disk_manager.DeleteSnapshotResponse{}
}
