package pools

import (
	"context"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type scheduleBaseDisksTask struct {
	scheduler                           tasks.Scheduler
	storage                             storage.Storage
	useDataplaneTasksForLegacySnapshots bool
}

func (t *scheduleBaseDisksTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	return nil
}

func (t *scheduleBaseDisksTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(&empty.Empty{})
}

func (t *scheduleBaseDisksTask) Load(ctx context.Context, state []byte) error {
	return nil
}

func (t *scheduleBaseDisksTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	baseDisks, err := t.storage.TakeBaseDisksToSchedule(ctx)
	if err != nil {
		return err
	}

	for i := 0; i < len(baseDisks); i++ {
		taskID, err := t.scheduler.ScheduleTask(
			headers.SetIncomingIdempotencyKey(
				ctx,
				"_base_disk_id_"+baseDisks[i].ID,
			),
			"pools.CreateBaseDisk",
			"",
			&protos.CreateBaseDiskRequest{
				SrcImageId:          baseDisks[i].ImageID,
				SrcDisk:             baseDisks[i].SrcDisk,
				SrcDiskCheckpointId: baseDisks[i].SrcDiskCheckpointID,
				BaseDisk: &types.Disk{
					ZoneId: baseDisks[i].ZoneID,
					DiskId: baseDisks[i].ID,
				},
				BaseDiskCheckpointId:                baseDisks[i].CheckpointID,
				BaseDiskSize:                        baseDisks[i].Size,
				UseDataplaneTasksForLegacySnapshots: t.useDataplaneTasksForLegacySnapshots,
			},
			"",
			"",
		)
		if err != nil {
			return err
		}

		baseDisks[i].CreateTaskID = taskID
	}

	return t.storage.BaseDisksScheduled(ctx, baseDisks)
}

func (t *scheduleBaseDisksTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *scheduleBaseDisksTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *scheduleBaseDisksTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
