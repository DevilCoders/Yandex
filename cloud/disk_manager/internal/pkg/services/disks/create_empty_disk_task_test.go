package disks

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	nbs_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs/mocks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	storage_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources/mocks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

func TestCreateEmptyDiskTask(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	nbsFactory := nbs_mocks.CreateFactoryMock()
	nbsClient := nbs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &createEmptyDiskTask{
		storage:    storage,
		nbsFactory: nbsFactory,
	}
	err := task.Init(ctx, &protos.CreateDiskParams{
		BlocksCount: 123,
		Disk: &types.Disk{
			ZoneId: "zone",
			DiskId: "disk",
		},
		BlockSize: 456,
		Kind:      types.DiskKind_DISK_KIND_SSD,
		CloudId:   "cloud",
		FolderId:  "folder",
	})
	assert.NoError(t, err)

	// TODO: Improve this expectations.
	storage.On("CreateDisk", ctx, mock.Anything).Return(&resources.DiskMeta{
		ID: "disk",
	}, nil)
	storage.On("DiskCreated", ctx, mock.Anything).Return(nil)

	nbsFactory.On("GetClient", ctx, "zone").Return(nbsClient, nil)
	nbsClient.On("Create", ctx, nbs.CreateDiskParams{
		ID:          "disk",
		BlocksCount: 123,
		BlockSize:   456,
		Kind:        types.DiskKind_DISK_KIND_SSD,
		CloudID:     "cloud",
		FolderID:    "folder",
	}).Return(nil)

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, storage, nbsFactory, nbsClient, execCtx)
	assert.NoError(t, err)
}

func TestCreateEmptyDiskTaskFailure(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	nbsFactory := nbs_mocks.CreateFactoryMock()
	nbsClient := nbs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &createEmptyDiskTask{
		storage:    storage,
		nbsFactory: nbsFactory,
	}
	err := task.Init(ctx, &protos.CreateDiskParams{
		BlocksCount: 123,
		Disk: &types.Disk{
			ZoneId: "zone",
			DiskId: "disk",
		},
		BlockSize: 456,
		Kind:      types.DiskKind_DISK_KIND_SSD,
		CloudId:   "cloud",
		FolderId:  "folder",
	})
	assert.NoError(t, err)

	// TODO: Improve this expectation.
	storage.On("CreateDisk", ctx, mock.Anything).Return(&resources.DiskMeta{}, nil)

	nbsFactory.On("GetClient", ctx, "zone").Return(nbsClient, nil)
	nbsClient.On("Create", ctx, nbs.CreateDiskParams{
		ID:          "disk",
		BlocksCount: 123,
		BlockSize:   456,
		Kind:        types.DiskKind_DISK_KIND_SSD,
		CloudID:     "cloud",
		FolderID:    "folder",
	}).Return(assert.AnError)

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, storage, nbsFactory, nbsClient, execCtx)
	assert.Equal(t, err, assert.AnError)
}

func TestCancelCreateEmptyDiskTask(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	nbsFactory := nbs_mocks.CreateFactoryMock()
	nbsClient := nbs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &createEmptyDiskTask{
		storage:    storage,
		nbsFactory: nbsFactory,
	}
	err := task.Init(ctx, &protos.CreateDiskParams{
		BlocksCount: 123,
		Disk: &types.Disk{
			ZoneId: "zone",
			DiskId: "disk",
		},
		BlockSize: 456,
		Kind:      types.DiskKind_DISK_KIND_SSD,
		CloudId:   "cloud",
		FolderId:  "folder",
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

	err = task.Cancel(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, storage, nbsFactory, nbsClient, execCtx)
	assert.NoError(t, err)
}

func TestCancelCreateEmptyDiskTaskFailure(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	nbsFactory := nbs_mocks.CreateFactoryMock()
	nbsClient := nbs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &createEmptyDiskTask{
		storage:    storage,
		nbsFactory: nbsFactory,
	}
	err := task.Init(ctx, &protos.CreateDiskParams{
		BlocksCount: 123,
		Disk: &types.Disk{
			ZoneId: "zone",
			DiskId: "disk",
		},
		BlockSize: 456,
		Kind:      types.DiskKind_DISK_KIND_SSD,
		CloudId:   "cloud",
		FolderId:  "folder",
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

	nbsFactory.On("GetClient", ctx, "zone").Return(nbsClient, nil)
	nbsClient.On("Delete", ctx, "disk").Return(assert.AnError)

	err = task.Cancel(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, storage, nbsFactory, nbsClient, execCtx)
	assert.Equal(t, err, assert.AnError)
}
