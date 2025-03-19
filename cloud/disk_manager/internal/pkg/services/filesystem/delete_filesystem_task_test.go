package filesystem

import (
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	storage_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources/mocks"
	"context"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"

	nfs_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nfs/mocks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/filesystem/protos"
)

////////////////////////////////////////////////////////////////////////////////

func TestDeleteFilesystemTaskRun(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	nfsFactory := nfs_mocks.CreateFactoryMock()
	nfsClient := nfs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &deleteFilesystemTask{
		storage: storage,
		factory: nfsFactory,
	}

	filesystem := &protos.FilesystemId{ZoneId: "zone", FilesystemId: "filesystem"}
	err := task.Init(ctx, &protos.DeleteFilesystemRequest{Filesystem: filesystem})
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

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, nfsFactory, nfsClient, execCtx)
	assert.NoError(t, err)
}

func TestDeleteFilesystemTaskCancel(t *testing.T) {
	ctx := context.Background()
	storage := storage_mocks.CreateStorageMock()
	nfsFactory := nfs_mocks.CreateFactoryMock()
	nfsClient := nfs_mocks.CreateClientMock()
	execCtx := createExecutionContextMock()

	task := &deleteFilesystemTask{
		storage: storage,
		factory: nfsFactory,
	}

	filesystem := &protos.FilesystemId{ZoneId: "zone", FilesystemId: "filesystem"}
	err := task.Init(ctx, &protos.DeleteFilesystemRequest{Filesystem: filesystem})
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
