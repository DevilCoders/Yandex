package pools

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type retireBaseDiskTask struct {
	scheduler  tasks.Scheduler
	storage    storage.Storage
	nbsFactory nbs.Factory
	state      *protos.RetireBaseDiskTaskState
}

func (t *retireBaseDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.RetireBaseDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.RetireBaseDiskTaskState{
		Request: typedRequest,
	}
	return nil
}

func (t *retireBaseDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *retireBaseDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.RetireBaseDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *retireBaseDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request
	baseDiskID := request.BaseDiskId
	selfTaskID := execCtx.GetTaskID()

	if request.SrcDisk != nil {
		client, err := t.nbsFactory.GetClient(ctx, request.SrcDisk.ZoneId)
		if err != nil {
			return err
		}

		err = client.GetCheckpointSize(
			ctx,
			func(blockIndex uint64, checkpointSize uint64) error {
				t.state.SrcDiskMilestoneBlockIndex = blockIndex
				t.state.SrcDiskMilestoneCheckpointSize = checkpointSize
				return execCtx.SaveState(ctx)
			},
			request.SrcDisk.DiskId,
			request.SrcDiskCheckpointId,
			t.state.SrcDiskMilestoneBlockIndex,
			t.state.SrcDiskMilestoneCheckpointSize,
		)
		if err != nil {
			if nbs.IsDiskNotFoundError(err) {
				// Should be idempotent.
				return nil
			}

			return err
		}
	}

	rebaseInfos, err := t.storage.RetireBaseDisk(
		ctx,
		baseDiskID,
		request.SrcDisk,
		request.SrcDiskCheckpointId,
		t.state.SrcDiskMilestoneCheckpointSize,
	)
	if err != nil {
		return err
	}

	rebaseTasks := make([]string, 0)

	for _, info := range rebaseInfos {
		idempotencyKey := selfTaskID
		idempotencyKey += "_" + info.OverlayDisk.DiskId
		idempotencyKey += "_" + info.TargetBaseDiskID

		taskID, err := t.scheduler.ScheduleTask(
			headers.SetIncomingIdempotencyKey(ctx, idempotencyKey),
			"pools.RebaseOverlayDisk",
			fmt.Sprintf(
				"Rebase overlay disk while retiring base disk with id=%v",
				baseDiskID,
			),
			&protos.RebaseOverlayDiskRequest{
				OverlayDisk:      info.OverlayDisk,
				BaseDiskId:       info.BaseDiskID,
				TargetBaseDiskId: info.TargetBaseDiskID,
				SlotGeneration:   info.SlotGeneration,
			},
			"",
			"",
		)
		if err != nil {
			return err
		}

		rebaseTasks = append(rebaseTasks, taskID)
	}

	for _, taskID := range rebaseTasks {
		err := t.scheduler.WaitTaskEnded(ctx, taskID)
		if err != nil {
			return err
		}
	}

	retired, err := t.storage.IsBaseDiskRetired(ctx, baseDiskID)
	if err != nil {
		return err
	}

	if !retired {
		// NBS-3316: loop until base disk is retired.
		return &errors.InterruptExecutionError{}
	}

	return nil
}

func (t *retireBaseDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *retireBaseDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *retireBaseDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
