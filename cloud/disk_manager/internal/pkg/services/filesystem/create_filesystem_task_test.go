package filesystem

import (
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	storage_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources/mocks"
	"context"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nfs"
	nfs_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nfs/mocks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/filesystem/protos"
	tasks_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/mocks"
)

////////////////////////////////////////////////////////////////////////////////

func createExecutionContextMock() *tasks_mocks.ExecutionContextMock {
	execCtx := tasks_mocks.CreateExecutionContextMock()
	execCtx.On("GetTaskID").Return("toplevel_task_id")
	return execCtx
}

////////////////////////////////////////////////////////////////////////////////

func TestCreateFilesystemTask(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	nfsFactory := nfs_mocks.CreateFactoryMock()
	nfsClient := nfs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &createFilesystemTask{
		storage: storage,
		factory: nfsFactory,
	}

	err := task.Init(ctx, &protos.CreateFilesystemRequest{
		Filesystem: &protos.FilesystemId{
			ZoneId:       "zone",
			FilesystemId: "filesystem",
		},
		CloudId:     "cloud",
		FolderId:    "folder",
		BlockSize:   456,
		BlocksCount: 123,
	})
	assert.NoError(t, err)

	storage.On("CreateFilesystem", ctx, mock.Anything).Return(&resources.FilesystemMeta{
		ID: "filesystem",
	}, nil)
	storage.On("FilesystemCreated", ctx, mock.Anything).Return(nil)

	nfsFactory.On("CreateClient", ctx, "zone").Return(nfsClient, nil)
	nfsClient.On("Create", ctx, "filesystem", nfs.CreateFilesystemParams{
		CloudID:     "cloud",
		FolderID:    "folder",
		BlockSize:   456,
		BlocksCount: 123,
	}).Return(nil)

	nfsClient.On("Close").Return(nil)

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, nfsFactory, nfsClient, execCtx)
	assert.NoError(t, err)
}

func TestCreateFilesystemTaskFailure(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	nfsFactory := nfs_mocks.CreateFactoryMock()
	nfsClient := nfs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &createFilesystemTask{
		storage: storage,
		factory: nfsFactory,
	}
	err := task.Init(ctx, &protos.CreateFilesystemRequest{
		Filesystem: &protos.FilesystemId{
			ZoneId:       "zone",
			FilesystemId: "filesystem",
		},
		CloudId:     "cloud",
		FolderId:    "folder",
		BlockSize:   456,
		BlocksCount: 123,
	})
	assert.NoError(t, err)

	storage.On("CreateFilesystem", ctx, mock.Anything).Return(&resources.FilesystemMeta{
		ID: "filesystem",
	}, nil)
	storage.On("FilesystemCreated", ctx, mock.Anything).Return(nil)
	storage.On(
		"DeleteFilesystem",
		ctx,
		"filesystem",
		"toplevel_task_id",
		mock.Anything,
	).Return(&resources.FilesystemMeta{
		DeleteTaskID: "toplevel_task_id",
	}, nil)
	storage.On("FilesystemDeleted", ctx, "filesystem", mock.Anything).Return(nil)

	nfsFactory.On("CreateClient", ctx, "zone").Return(nfsClient, nil)
	nfsClient.On("Create", ctx, "filesystem", nfs.CreateFilesystemParams{
		CloudID:     "cloud",
		FolderID:    "folder",
		BlockSize:   456,
		BlocksCount: 123,
	}).Return(assert.AnError)

	nfsClient.On("Close").Return(nil)

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, nfsFactory, nfsClient, execCtx)
	assert.Equal(t, err, assert.AnError)
}

func TestCancelCreateFilesystemTask(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	nfsFactory := nfs_mocks.CreateFactoryMock()
	nfsClient := nfs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &createFilesystemTask{
		storage: storage,
		factory: nfsFactory,
	}
	err := task.Init(ctx, &protos.CreateFilesystemRequest{
		Filesystem: &protos.FilesystemId{
			ZoneId:       "zone",
			FilesystemId: "filesystem",
		},
		CloudId:     "cloud",
		FolderId:    "folder",
		BlockSize:   456,
		BlocksCount: 123,
	})
	assert.NoError(t, err)

	storage.On(
		"DeleteFilesystem",
		ctx,
		"filesystem",
		"toplevel_task_id",
		mock.Anything,
	).Return(&resources.FilesystemMeta{
		DeleteTaskID: "toplevel_task_id",
	}, nil)
	storage.On("FilesystemDeleted", ctx, "filesystem", mock.Anything).Return(nil)

	nfsFactory.On("CreateClient", ctx, "zone").Return(nfsClient, nil)
	nfsClient.On("Delete", ctx, "filesystem").Return(nil)
	nfsClient.On("Close").Return(nil)

	err = task.Cancel(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, nfsFactory, nfsClient, execCtx)
	assert.NoError(t, err)
}
