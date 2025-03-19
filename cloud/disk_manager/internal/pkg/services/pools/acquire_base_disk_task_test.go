package pools

import (
	"context"
	"testing"

	"github.com/golang/protobuf/ptypes/empty"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	pools_storage_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage/mocks"
	tasks_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/mocks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

func createContext() context.Context {
	return logging.SetLogger(
		context.Background(),
		logging.CreateStderrLogger(logging.DebugLevel),
	)
}

////////////////////////////////////////////////////////////////////////////////

func TestAcquireBaseDiskTask(t *testing.T) {
	ctx := createContext()
	scheduler := tasks_mocks.CreateSchedulerMock()
	s := pools_storage_mocks.CreateStorageMock()
	execCtx := tasks_mocks.CreateExecutionContextMock()

	execCtx.On("SaveState", ctx).Return(nil)

	task := &acquireBaseDiskTask{
		scheduler: scheduler,
		storage:   s,
	}

	overlayDisk := &types.Disk{ZoneId: "zone", DiskId: "disk"}
	err := task.Init(ctx, &protos.AcquireBaseDiskRequest{
		SrcImageId:  "image",
		OverlayDisk: overlayDisk,
	})
	assert.NoError(t, err)

	s.On(
		"AcquireBaseDiskSlot",
		ctx,
		"image",
		storage.Slot{OverlayDisk: overlayDisk},
	).Return(storage.BaseDisk{
		ID:           "baseDisk",
		ImageID:      "image",
		ZoneID:       "zone",
		CheckpointID: "checkpoint",
		CreateTaskID: "acquire",
		Ready:        true,
	}, nil)

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, s, execCtx)
	assert.NoError(t, err)
}

func TestAcquireBaseDiskTaskShouldWaitForBaseDiskCreated(t *testing.T) {
	ctx := createContext()
	scheduler := tasks_mocks.CreateSchedulerMock()
	s := pools_storage_mocks.CreateStorageMock()
	execCtx := tasks_mocks.CreateExecutionContextMock()

	execCtx.On("SaveState", ctx).Return(nil)

	task := &acquireBaseDiskTask{
		scheduler: scheduler,
		storage:   s,
	}

	overlayDisk := &types.Disk{ZoneId: "zone", DiskId: "disk"}
	err := task.Init(ctx, &protos.AcquireBaseDiskRequest{
		SrcImageId:  "image",
		OverlayDisk: overlayDisk,
	})
	assert.NoError(t, err)

	s.On(
		"AcquireBaseDiskSlot",
		ctx,
		"image",
		storage.Slot{OverlayDisk: overlayDisk},
	).Return(storage.BaseDisk{
		ID:           "baseDisk",
		ImageID:      "image",
		ZoneID:       "zone",
		CheckpointID: "checkpoint",
		CreateTaskID: "otherAcquire",
		Ready:        false,
	}, nil)
	scheduler.On("WaitTaskAsync", ctx, execCtx, "otherAcquire").Return(&empty.Empty{}, nil)

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, s, execCtx)
	assert.NoError(t, err)
}

func TestCancelAcquireBaseDiskTaskShouldReleaseBaseDisk(t *testing.T) {
	ctx := createContext()
	scheduler := tasks_mocks.CreateSchedulerMock()
	s := pools_storage_mocks.CreateStorageMock()
	execCtx := tasks_mocks.CreateExecutionContextMock()

	task := &acquireBaseDiskTask{
		scheduler: scheduler,
		storage:   s,
	}

	disk := &types.Disk{ZoneId: "zone", DiskId: "disk"}
	err := task.Init(ctx, &protos.AcquireBaseDiskRequest{
		SrcImageId:  "image",
		OverlayDisk: disk,
	})
	assert.NoError(t, err)

	baseDisk := storage.BaseDisk{
		ID:           "baseDisk",
		ImageID:      "image",
		ZoneID:       "zone",
		CheckpointID: "checkpoint",
		CreateTaskID: "acquire",
		Ready:        false,
	}
	s.On("ReleaseBaseDiskSlot", ctx, disk).Return(baseDisk, nil)

	err = task.Cancel(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, s, execCtx)
	assert.NoError(t, err)
}
