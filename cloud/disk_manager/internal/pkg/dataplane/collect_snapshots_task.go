package dataplane

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type collectSnapshotsTask struct {
	scheduler                       tasks.Scheduler
	storage                         storage.Storage
	snapshotCollectionTimeout       time.Duration
	snapshotCollectionInflightLimit int
	state                           *protos.CollectSnapshotsTaskState
}

func (t *collectSnapshotsTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	t.state = &protos.CollectSnapshotsTaskState{}
	return nil
}

func (t *collectSnapshotsTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *collectSnapshotsTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CollectSnapshotsTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *collectSnapshotsTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	deletingBefore := time.Now().Add(-t.snapshotCollectionTimeout)

	if len(t.state.Snapshots) == 0 {
		var err error

		t.state.Snapshots, err = t.storage.GetSnapshotsToDelete(
			ctx,
			deletingBefore,
			t.snapshotCollectionInflightLimit,
		)
		if err != nil {
			return err
		}

		err = execCtx.SaveState(ctx)
		if err != nil {
			return err
		}
	}

	var tasks []string

	for _, snapshot := range t.state.Snapshots {
		idempotencyKey := fmt.Sprintf(
			"%v_%v",
			execCtx.GetTaskID(),
			snapshot.SnapshotId,
		)

		taskID, err := t.scheduler.ScheduleTask(
			headers.SetIncomingIdempotencyKey(ctx, idempotencyKey),
			"dataplane.DeleteSnapshotData",
			"",
			&protos.DeleteSnapshotDataRequest{
				SnapshotId: snapshot.SnapshotId,
			},
			"",
			"",
		)
		if err != nil {
			return err
		}

		tasks = append(tasks, taskID)
	}

	for _, taskID := range tasks {
		_, err := t.scheduler.WaitTaskAsync(ctx, execCtx, taskID)
		if err != nil {
			return err
		}
	}

	return t.storage.ClearDeletingSnapshots(ctx, t.state.Snapshots)
}

func (t *collectSnapshotsTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *collectSnapshotsTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *collectSnapshotsTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
