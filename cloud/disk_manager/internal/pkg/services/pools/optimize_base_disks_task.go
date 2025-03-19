package pools

import (
	"context"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type optimizeBaseDisksTask struct {
	scheduler                               tasks.Scheduler
	storage                                 storage.Storage
	convertToImageSizedBaseDisksThreshold   uint64
	convertToDefaultSizedBaseDisksThreshold uint64
	minPoolAge                              time.Duration
	state                                   *protos.OptimizeBaseDisksTaskState
}

func (t *optimizeBaseDisksTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	t.state = &protos.OptimizeBaseDisksTaskState{}
	return nil
}

func (t *optimizeBaseDisksTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *optimizeBaseDisksTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.OptimizeBaseDisksTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *optimizeBaseDisksTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	t1 := t.convertToDefaultSizedBaseDisksThreshold
	t2 := t.convertToImageSizedBaseDisksThreshold

	if t1 == 0 && t2 == 0 {
		return nil
	}

	if len(t.state.Requests) == 0 {
		poolInfos, err := t.storage.GetReadyPoolInfos(ctx)
		if err != nil {
			return err
		}

		now := time.Now()

		for _, poolInfo := range poolInfos {
			dateThreshold := poolInfo.CreatedAt.Add(t.minPoolAge)
			if now.Before(dateThreshold) {
				continue
			}

			useImageSize := poolInfo.ImageSize > 0
			newUseImageSize := useImageSize

			if useImageSize && poolInfo.AcquiredUnits > t1 {
				newUseImageSize = false
			} else if !useImageSize && poolInfo.AcquiredUnits < t2 {
				newUseImageSize = true
			}

			if useImageSize == newUseImageSize {
				continue
			}

			t.state.Requests = append(
				t.state.Requests,
				&protos.ConfigurePoolRequest{
					ZoneId:       poolInfo.ZoneID,
					ImageId:      poolInfo.ImageID,
					Capacity:     poolInfo.Capacity,
					UseImageSize: newUseImageSize,
				},
			)
		}

		err = execCtx.SaveState(ctx)
		if err != nil {
			return err
		}
	}

	var configureTaskIDs []string

	for _, request := range t.state.Requests {
		taskID, err := t.scheduler.ScheduleTask(
			headers.SetIncomingIdempotencyKey(
				ctx,
				"configure_pool_"+execCtx.GetTaskID()+
					":"+request.GetZoneId()+":"+request.GetImageId(),
			),
			"pools.ConfigurePool",
			"",
			request,
			"",
			"",
		)
		if err != nil {
			return err
		}

		configureTaskIDs = append(configureTaskIDs, taskID)
	}

	for i, taskID := range configureTaskIDs {
		_, err := t.scheduler.WaitTaskAsync(
			ctx,
			execCtx,
			taskID,
		)
		if err != nil {
			if !errors.CanRetry(err) {
				continue
			}

			return err
		}

		request := t.state.Requests[i]

		taskID, err = t.scheduler.ScheduleTask(
			headers.SetIncomingIdempotencyKey(
				ctx,
				"retire_base_disks_"+execCtx.GetTaskID()+
					":"+request.GetZoneId()+":"+request.GetImageId(),
			),
			"pools.RetireBaseDisks",
			"",
			&protos.RetireBaseDisksRequest{
				ZoneId:           request.GetZoneId(),
				ImageId:          request.GetImageId(),
				UseBaseDiskAsSrc: true,
			},
			"",
			"",
		)
		if err != nil {
			return err
		}

		_, err = t.scheduler.WaitTaskAsync(ctx, execCtx, taskID)
		if err != nil {
			return err
		}
	}

	return nil
}

func (t *optimizeBaseDisksTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *optimizeBaseDisksTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *optimizeBaseDisksTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
