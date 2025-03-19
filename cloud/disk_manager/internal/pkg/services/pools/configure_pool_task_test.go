package pools

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	resources_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources/mocks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	pools_storage_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage/mocks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	tasks_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/mocks"
)

////////////////////////////////////////////////////////////////////////////////

func TestConfigurePoolTaskImageNotReady(t *testing.T) {
	ctx := createContext()
	resource := resources_mocks.CreateStorageMock()
	s := pools_storage_mocks.CreateStorageMock()
	execCtx := tasks_mocks.CreateExecutionContextMock()

	task := &configurePoolTask{
		storage:         s,
		snapshotFactory: nil,
		resourceStorage: resource,
	}

	zone := "zone"
	imageID := t.Name()
	capacity := uint32(1)

	err := task.Init(ctx, &protos.ConfigurePoolRequest{
		ZoneId:       zone,
		ImageId:      imageID,
		Capacity:     capacity,
		UseImageSize: false,
	})
	require.NoError(t, err)

	resource.On(
		"GetImageMeta",
		ctx,
		imageID,
	).Once().Return(&resources.ImageMeta{
		ID:                imageID,
		UseDataplaneTasks: true,
	}, nil)

	resource.On(
		"CheckImageReady",
		ctx,
		imageID,
	).Once().Return(&resources.ImageMeta{}, &errors.NonRetriableError{
		Err: fmt.Errorf("image with id=%v is not ready", imageID),
	})

	err = task.Run(ctx, execCtx)
	mock.AssertExpectationsForObjects(t, s, execCtx)
	require.Error(t, err)
	require.True(t, errors.Is(err, &errors.NonRetriableError{}))
	require.Contains(t, err.Error(), "is not ready")
}
