package pools

import (
	"testing"
	"time"

	"github.com/golang/protobuf/ptypes/empty"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	pools_storage_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage/mocks"
	tasks_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/mocks"
)

////////////////////////////////////////////////////////////////////////////////

func TestOptimizeBaseDisksTask(t *testing.T) {
	ctx := createContext()
	scheduler := tasks_mocks.CreateSchedulerMock()
	s := pools_storage_mocks.CreateStorageMock()
	execCtx := tasks_mocks.CreateExecutionContextMock()

	execCtx.On("GetTaskID").Return("1")
	execCtx.On("SaveState", mock.Anything).Return(nil)

	now := time.Now()
	yesterday := now.AddDate(0, 0, -1)

	s.On("GetReadyPoolInfos", ctx).Return([]storage.PoolInfo{
		{
			ImageID:       "image1",
			ZoneID:        "zone1",
			FreeUnits:     111,
			AcquiredUnits: 20,
			Capacity:      100,
			ImageSize:     0,
			CreatedAt:     yesterday,
		},
		{
			ImageID:       "image2",
			ZoneID:        "zone2",
			FreeUnits:     111,
			AcquiredUnits: 2,
			Capacity:      100,
			ImageSize:     0,
			CreatedAt:     yesterday,
		},
		{
			ImageID:       "image3",
			ZoneID:        "zone3",
			FreeUnits:     111,
			AcquiredUnits: 10,
			Capacity:      100,
			ImageSize:     0,
			CreatedAt:     yesterday,
		},
		{
			ImageID:       "image4",
			ZoneID:        "zone4",
			FreeUnits:     111,
			AcquiredUnits: 20,
			Capacity:      100,
			ImageSize:     111,
			CreatedAt:     yesterday,
		},
		{
			ImageID:       "image5",
			ZoneID:        "zone5",
			FreeUnits:     111,
			AcquiredUnits: 20,
			Capacity:      100,
			ImageSize:     111,
			CreatedAt:     now,
		},
	}, nil)

	scheduler.On(
		"ScheduleTask",
		mock.Anything,
		"pools.ConfigurePool",
		"",
		&protos.ConfigurePoolRequest{
			ImageId:      "image2",
			ZoneId:       "zone2",
			Capacity:     100,
			UseImageSize: true,
		},
		"",
		"",
	).Return("task2", nil)

	scheduler.On(
		"ScheduleTask",
		mock.Anything,
		"pools.ConfigurePool",
		"",
		&protos.ConfigurePoolRequest{
			ImageId:      "image4",
			ZoneId:       "zone4",
			Capacity:     100,
			UseImageSize: false,
		},
		"",
		"",
	).Return("task4", nil)

	scheduler.On(
		"WaitTaskAsync",
		mock.Anything,
		execCtx,
		"task2",
	).Return(nil, nil)

	scheduler.On(
		"WaitTaskAsync",
		mock.Anything,
		execCtx,
		"task4",
	).Return(nil, nil)

	scheduler.On(
		"ScheduleTask",
		mock.Anything,
		"pools.RetireBaseDisks",
		"",
		&protos.RetireBaseDisksRequest{
			ImageId:          "image2",
			ZoneId:           "zone2",
			UseBaseDiskAsSrc: true,
		},
		"",
		"",
	).Return("task2_1", nil)

	scheduler.On(
		"ScheduleTask",
		mock.Anything,
		"pools.RetireBaseDisks",
		"",
		&protos.RetireBaseDisksRequest{
			ImageId:          "image4",
			ZoneId:           "zone4",
			UseBaseDiskAsSrc: true,
		},
		"",
		"",
	).Return("task4_1", nil)

	scheduler.On(
		"WaitTaskAsync",
		mock.Anything,
		execCtx,
		"task2_1",
	).Return(nil, nil)

	scheduler.On(
		"WaitTaskAsync",
		mock.Anything,
		execCtx,
		"task4_1",
	).Return(nil, nil)

	minPoolAge, err := time.ParseDuration("12h")
	assert.NoError(t, err)

	task := &optimizeBaseDisksTask{
		scheduler:                               scheduler,
		storage:                                 s,
		convertToImageSizedBaseDisksThreshold:   5,
		convertToDefaultSizedBaseDisksThreshold: 15,
		minPoolAge:                              minPoolAge,
	}

	err = task.Init(ctx, &empty.Empty{})
	assert.NoError(t, err)

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, s, execCtx)
	assert.NoError(t, err)
}
