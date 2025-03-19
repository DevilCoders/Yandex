package pools

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	pools_storage_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage/mocks"
	tasks_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/mocks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

type TestAction uint32

const (
	TestActionRunTask    TestAction = iota
	TestActionCancelTask TestAction = iota
)

func checkReleaseBaseDiskTaskShouldReleaseBaseDisk(t *testing.T, action TestAction) {
	ctx := createContext()
	s := pools_storage_mocks.CreateStorageMock()
	execCtx := tasks_mocks.CreateExecutionContextMock()

	task := &releaseBaseDiskTask{
		storage: s,
	}

	disk := &types.Disk{ZoneId: "zone", DiskId: "disk"}
	err := task.Init(ctx, &protos.ReleaseBaseDiskRequest{OverlayDisk: disk})
	assert.NoError(t, err)

	baseDisk := storage.BaseDisk{
		ID:      "baseDisk",
		ImageID: "image",
		ZoneID:  "zone",
	}
	s.On("ReleaseBaseDiskSlot", ctx, disk).Return(baseDisk, nil)

	if action == TestActionRunTask {
		err = task.Run(ctx, execCtx)
	} else {
		err = task.Cancel(ctx, execCtx)
	}
	mock.AssertExpectationsForObjects(t, s, execCtx)
	assert.NoError(t, err)
}

func TestRunningReleaseBaseDiskTaskShouldReleaseBaseDisk(t *testing.T) {
	checkReleaseBaseDiskTaskShouldReleaseBaseDisk(t, TestActionRunTask)
}

func TestCancellingReleaseBaseDiskTaskShouldReleaseBaseDisk(t *testing.T) {
	checkReleaseBaseDiskTaskShouldReleaseBaseDisk(t, TestActionCancelTask)
}
