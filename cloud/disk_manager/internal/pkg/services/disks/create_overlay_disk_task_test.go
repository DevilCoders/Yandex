package disks

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
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

func createExecutionContextMock() *tasks_mocks.ExecutionContextMock {
	execCtx := tasks_mocks.CreateExecutionContextMock()
	execCtx.On("GetTaskID").Return("toplevel_task_id")
	return execCtx
}

////////////////////////////////////////////////////////////////////////////////

func TestCreateOverlayDiskTask(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	scheduler := tasks_mocks.CreateSchedulerMock()
	poolService := pools_mocks.CreateServiceMock()
	nbsFactory := nbs_mocks.CreateFactoryMock()
	nbsClient := nbs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &createOverlayDiskTask{
		storage:     storage,
		scheduler:   scheduler,
		poolService: poolService,
		nbsFactory:  nbsFactory,
	}
	err := task.Init(ctx, &protos.CreateOverlayDiskRequest{
		SrcImageId: "image",
		Params: &protos.CreateDiskParams{
			BlocksCount: 123,
			Disk: &types.Disk{
				ZoneId: "zone",
				DiskId: "disk",
			},
			BlockSize: 4096,
			Kind:      types.DiskKind_DISK_KIND_SSD,
			CloudId:   "cloud",
			FolderId:  "folder",
		},
	})
	assert.NoError(t, err)

	// TODO: Improve this expectations.
	storage.On("CreateDisk", ctx, mock.Anything).Return(&resources.DiskMeta{
		ID: "disk",
	}, nil)
	storage.On("DiskCreated", ctx, mock.Anything).Return(nil)

	poolService.On(
		"AcquireBaseDisk",
		headers.SetIncomingIdempotencyKey(ctx, "toplevel_task_id_acquire"),
		&pools_protos.AcquireBaseDiskRequest{
			SrcImageId: "image",
			OverlayDisk: &types.Disk{
				ZoneId: "zone",
				DiskId: "disk",
			},
			OverlayDiskKind: types.DiskKind_DISK_KIND_SSD,
			OverlayDiskSize: 123 * 4096,
		}).Return("acquire", nil)
	scheduler.On("WaitTaskAsync", ctx, execCtx, "acquire").Return(
		&pools_protos.AcquireBaseDiskResponse{
			BaseDiskId:           "base",
			BaseDiskCheckpointId: "checkpoint",
		},
		nil,
	)

	nbsFactory.On("GetClient", ctx, "zone").Return(nbsClient, nil)
	nbsClient.On("Create", ctx, nbs.CreateDiskParams{
		ID:                   "disk",
		BaseDiskID:           "base",
		BaseDiskCheckpointID: "checkpoint",
		BlocksCount:          123,
		BlockSize:            4096,
		Kind:                 types.DiskKind_DISK_KIND_SSD,
		CloudID:              "cloud",
		FolderID:             "folder",
	}).Return(nil)

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, storage, scheduler, poolService, nbsFactory, nbsClient, execCtx)
	assert.NoError(t, err)
}

func TestCreateOverlayDiskTaskFailureWhenAcquireReturnsEmptyBaseDiskId(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	scheduler := tasks_mocks.CreateSchedulerMock()
	poolService := pools_mocks.CreateServiceMock()
	nbsFactory := nbs_mocks.CreateFactoryMock()
	nbsClient := nbs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &createOverlayDiskTask{
		storage:     storage,
		scheduler:   scheduler,
		poolService: poolService,
		nbsFactory:  nbsFactory,
	}
	err := task.Init(ctx, &protos.CreateOverlayDiskRequest{
		SrcImageId: "image",
		Params: &protos.CreateDiskParams{
			BlocksCount: 123,
			Disk: &types.Disk{
				ZoneId: "zone",
				DiskId: "disk",
			},
			BlockSize: 4096,
			Kind:      types.DiskKind_DISK_KIND_SSD,
			CloudId:   "cloud",
			FolderId:  "folder",
		},
	})
	assert.NoError(t, err)

	// TODO: Improve this expectation.
	storage.On("CreateDisk", ctx, mock.Anything).Return(&resources.DiskMeta{}, nil)

	nbsFactory.On("GetClient", ctx, "zone").Return(nbsClient, nil)

	poolService.On(
		"AcquireBaseDisk",
		headers.SetIncomingIdempotencyKey(ctx, "toplevel_task_id_acquire"),
		&pools_protos.AcquireBaseDiskRequest{
			SrcImageId: "image",
			OverlayDisk: &types.Disk{
				ZoneId: "zone",
				DiskId: "disk",
			},
			OverlayDiskKind: types.DiskKind_DISK_KIND_SSD,
			OverlayDiskSize: 123 * 4096,
		}).Return("acquire", nil)
	scheduler.On("WaitTaskAsync", ctx, execCtx, "acquire").Return(
		&pools_protos.AcquireBaseDiskResponse{
			BaseDiskId:           "",
			BaseDiskCheckpointId: "checkpoint",
		},
		nil,
	)

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, storage, scheduler, poolService, nbsFactory, nbsClient, execCtx)
	assert.Error(t, err)
}

func TestCreateOverlayDiskTaskFailureWhenAcquireReturnsEmptyBaseDiskCheckpointId(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	scheduler := tasks_mocks.CreateSchedulerMock()
	poolService := pools_mocks.CreateServiceMock()
	nbsFactory := nbs_mocks.CreateFactoryMock()
	nbsClient := nbs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &createOverlayDiskTask{
		storage:     storage,
		scheduler:   scheduler,
		poolService: poolService,
		nbsFactory:  nbsFactory,
	}
	err := task.Init(ctx, &protos.CreateOverlayDiskRequest{
		SrcImageId: "image",
		Params: &protos.CreateDiskParams{
			BlocksCount: 123,
			Disk: &types.Disk{
				ZoneId: "zone",
				DiskId: "disk",
			},
			BlockSize: 4096,
			Kind:      types.DiskKind_DISK_KIND_SSD,
			CloudId:   "cloud",
			FolderId:  "folder",
		},
	})
	assert.NoError(t, err)

	// TODO: Improve this expectation.
	storage.On("CreateDisk", ctx, mock.Anything).Return(&resources.DiskMeta{}, nil)

	nbsFactory.On("GetClient", ctx, "zone").Return(nbsClient, nil)

	poolService.On(
		"AcquireBaseDisk",
		headers.SetIncomingIdempotencyKey(ctx, "toplevel_task_id_acquire"),
		&pools_protos.AcquireBaseDiskRequest{
			SrcImageId: "image",
			OverlayDisk: &types.Disk{
				ZoneId: "zone",
				DiskId: "disk",
			},
			OverlayDiskKind: types.DiskKind_DISK_KIND_SSD,
			OverlayDiskSize: 123 * 4096,
		}).Return("acquire", nil)
	scheduler.On("WaitTaskAsync", ctx, execCtx, "acquire").Return(
		&pools_protos.AcquireBaseDiskResponse{
			BaseDiskId:           "base",
			BaseDiskCheckpointId: "",
		},
		nil,
	)

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, storage, scheduler, poolService, nbsFactory, nbsClient, execCtx)
	assert.Error(t, err)
}

func TestCancelCreateOverlayDiskTask(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	scheduler := tasks_mocks.CreateSchedulerMock()
	poolService := pools_mocks.CreateServiceMock()
	nbsFactory := nbs_mocks.CreateFactoryMock()
	nbsClient := nbs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &createOverlayDiskTask{
		storage:     storage,
		scheduler:   scheduler,
		poolService: poolService,
		nbsFactory:  nbsFactory,
	}
	err := task.Init(ctx, &protos.CreateOverlayDiskRequest{
		SrcImageId: "image",
		Params: &protos.CreateDiskParams{
			BlocksCount: 123,
			Disk: &types.Disk{
				ZoneId: "zone",
				DiskId: "disk",
			},
			BlockSize: 4096,
			Kind:      types.DiskKind_DISK_KIND_SSD,
			CloudId:   "cloud",
			FolderId:  "folder",
		},
	})
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

	poolService.On(
		"ReleaseBaseDisk",
		headers.SetIncomingIdempotencyKey(ctx, "toplevel_task_id_release"),
		&pools_protos.ReleaseBaseDiskRequest{
			OverlayDisk: &types.Disk{
				ZoneId: "zone",
				DiskId: "disk",
			},
		}).Return("release", nil)
	scheduler.On("WaitTaskAsync", ctx, execCtx, "release").Return(nil, nil)

	err = task.Cancel(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, storage, scheduler, poolService, nbsFactory, nbsClient, execCtx)
	assert.NoError(t, err)
}
