package steps_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestLockRelease(t *testing.T) {
	type testCase struct {
		name          string
		prepareLocker func(locker *mocks.MockLocker)
		expected      steps.RunResult
	}

	lockID := "cms-qwe-test-shard"
	lockState := models.DefaultLockState()
	lockState.LockID = lockID

	testCases := []testCase{
		{
			name: "unlock instance",
			prepareLocker: func(locker *mocks.MockLocker) {
				locker.EXPECT().ReleaseCluster(gomock.Any(), lockState).Return(nil)
			},
			expected: steps.RunResult{
				Description: "released lock \"cms-qwe-test-shard\"",
				Error:       nil,
				IsDone:      true,
			},
		},
		{
			name: "can not release",
			prepareLocker: func(locker *mocks.MockLocker) {
				locker.EXPECT().ReleaseCluster(gomock.Any(), lockState).Return(xerrors.New("some error"))
			},
			expected: steps.RunResult{
				Description: "failed to release lock \"cms-qwe-test-shard\"",
				IsDone:      false,
				Error:       xerrors.New("some error"),
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			l := mocks.NewMockLocker(ctrl)
			tc.prepareLocker(l)
			step := steps.NewLockRelease(l)
			state := &models.OperationState{}
			state.SetFQDN("fqdn1").SetLockState(lockState)
			stepCtx := opcontext.NewStepContext(models.ManagementInstanceOperation{
				ID:         "qwe",
				InstanceID: "fqdn1",
				State:      state,
			})

			testStep(t, ctx, stepCtx, step, tc.expected)
		})
	}
}
