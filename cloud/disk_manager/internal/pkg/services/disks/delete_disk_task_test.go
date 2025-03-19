package disks

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"

	nbs_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs/mocks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	storage_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources/mocks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks/protos"
	pools_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/mocks"
	pools_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	tasks_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/mocks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

func TestDeleteDiskTaskRun(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	scheduler := tasks_mocks.CreateSchedulerMock()
	nbsFactory := nbs_mocks.CreateFactoryMock()
	nbsClient := nbs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &deleteDiskTask{
		storage:    storage,
		scheduler:  scheduler,
		nbsFactory: nbsFactory,
	}

	disk := &types.Disk{ZoneId: "zone", DiskId: "disk"}
	err := task.Init(ctx, &protos.DeleteDiskRequest{Disk: disk})
	assert.NoError(t, err)

	storage.On(
		"DeleteDisk",
		ctx,
		"disk",
		"toplevel_task_id",
		mock.Anything,
	).Return(&resources.DiskMeta{
		DeleteTaskID: "toplevel_task_id",
	}, nil)
	storage.On("DiskDeleted", ctx, "disk", mock.Anything).Return(nil)

	nbsFactory.On("GetClient", ctx, "zone").Return(nbsClient, nil)
	nbsClient.On("Delete", ctx, "disk").Return(nil)

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, storage, scheduler, nbsFactory, nbsClient, execCtx)
	assert.NoError(t, err)
}

func TestDeleteDiskTaskRunWithDiskCreatedFromImage(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	scheduler := tasks_mocks.CreateSchedulerMock()
	poolService := pools_mocks.CreateServiceMock()
	nbsFactory := nbs_mocks.CreateFactoryMock()
	nbsClient := nbs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &deleteDiskTask{
		storage:     storage,
		scheduler:   scheduler,
		poolService: poolService,
		nbsFactory:  nbsFactory,
	}

	disk := &types.Disk{ZoneId: "zone", DiskId: "disk"}
	err := task.Init(ctx, &protos.DeleteDiskRequest{Disk: disk})
	assert.NoError(t, err)

	storage.On(
		"DeleteDisk",
		ctx,
		"disk",
		"toplevel_task_id",
		mock.Anything,
	).Return(&resources.DiskMeta{
		SrcImageID:   "image",
		DeleteTaskID: "toplevel_task_id",
	}, nil)
	storage.On("DiskDeleted", ctx, "disk", mock.Anything).Return(nil)

	nbsFactory.On("GetClient", ctx, "zone").Return(nbsClient, nil)
	nbsClient.On("Delete", ctx, "disk").Return(nil)

	poolService.On(
		"ReleaseBaseDisk",
		headers.SetIncomingIdempotencyKey(ctx, "toplevel_task_id"),
		&pools_protos.ReleaseBaseDiskRequest{
			OverlayDisk: disk,
		}).Return("release", nil)
	scheduler.On("WaitTaskAsync", ctx, execCtx, "release").Return(nil, nil)

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, storage, scheduler, poolService, nbsFactory, nbsClient, execCtx)
	assert.NoError(t, err)
}

func TestDeleteDiskTaskCancel(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	scheduler := tasks_mocks.CreateSchedulerMock()
	poolService := pools_mocks.CreateServiceMock()
	nbsFactory := nbs_mocks.CreateFactoryMock()
	nbsClient := nbs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &deleteDiskTask{
		storage:     storage,
		scheduler:   scheduler,
		poolService: poolService,
		nbsFactory:  nbsFactory,
	}

	disk := &types.Disk{ZoneId: "zone", DiskId: "disk"}
	err := task.Init(ctx, &protos.DeleteDiskRequest{Disk: disk})
	assert.NoError(t, err)

	storage.On(
		"DeleteDisk",
		ctx,
		"disk",
		"toplevel_task_id",
		mock.Anything,
	).Return(&resources.DiskMeta{
		SrcImageID:   "image",
		DeleteTaskID: "toplevel_task_id",
	}, nil)
	storage.On("DiskDeleted", ctx, "disk", mock.Anything).Return(nil)

	nbsFactory.On("GetClient", ctx, "zone").Return(nbsClient, nil)
	nbsClient.On("Delete", ctx, "disk").Return(nil)

	poolService.On(
		"ReleaseBaseDisk",
		headers.SetIncomingIdempotencyKey(ctx, "toplevel_task_id"),
		&pools_protos.ReleaseBaseDiskRequest{
			OverlayDisk: disk,
		}).Return("release", nil)
	scheduler.On("WaitTaskAsync", ctx, execCtx, "release").Return(nil, nil)

	err = task.Cancel(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, storage, scheduler, poolService, nbsFactory, nbsClient, execCtx)
	assert.NoError(t, err)
}
