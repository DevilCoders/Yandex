package steps_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestLockAcquire(t *testing.T) {
	type testCase struct {
		name          string
		prepareLocker func(locker *mocks.MockLocker)
		expected      steps.RunResult
	}
	lockID := "cms-qwe-test-shard"
	fqdn := "fqdn1"
	taskID := "qwe"

	testCases := []testCase{
		{
			name: "lock instance",
			prepareLocker: func(locker *mocks.MockLocker) {
				locker.EXPECT().LockCluster(gomock.Any(), fqdn, taskID, lockcluster.InstanceCMS).Return(
					&lockcluster.State{LockID: lockID, IsTaken: true}, nil)
			},
			expected: steps.RunResult{
				Description: "acquired lock \"cms-qwe-test-shard\"",
				Error:       nil,
				IsDone:      true,
			},
		},
		{
			name: "already locked",
			prepareLocker: func(locker *mocks.MockLocker) {
				locker.EXPECT().LockCluster(gomock.Any(), fqdn, taskID, lockcluster.InstanceCMS).Return(
					&lockcluster.State{LockID: lockID}, lockcluster.NewError(
						lockcluster.NotAcquiredConflicts,
						"could not acquire lock \"cms-qwe-test-shard\" because of conflicts: fqdn1 locked by other-lock",
					)).Times(2)
			},
			expected: steps.RunResult{
				Description: "will try again: could not acquire lock \"cms-qwe-test-shard\" because of conflicts: fqdn1 locked by other-lock",
				IsDone:      false,
				Error:       lockcluster.NotAcquiredConflicts,
			},
		},
		{
			name: "not found in metadb",
			prepareLocker: func(locker *mocks.MockLocker) {
				locker.EXPECT().LockCluster(gomock.Any(), fqdn, taskID, lockcluster.InstanceCMS).Return(
					nil, lockcluster.UnmanagedHost).Times(2)
			},
			expected: steps.RunResult{
				Description: "host is unmanaged",
				Error:       nil,
				IsDone:      true,
			},
		},
		{
			name: "can not create",
			prepareLocker: func(locker *mocks.MockLocker) {
				locker.EXPECT().LockCluster(gomock.Any(), fqdn, taskID, lockcluster.InstanceCMS).Return(
					&lockcluster.State{LockID: lockID}, xerrors.New("some error")).Times(2)
			},
			expected: steps.RunResult{
				Description: "could not create lock \"cms-qwe-test-shard\"",
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
			step := steps.NewLockAcquire(l)
			state := &models.OperationState{}
			state.SetFQDN("fqdn1").SetLockState(models.DefaultLockState())
			stepCtx := opcontext.NewStepContext(models.ManagementInstanceOperation{
				ID:         "qwe",
				InstanceID: "fqdn1",
				State:      state,
			})

			testStep(t, ctx, stepCtx, step, tc.expected)
			testStep(t, ctx, stepCtx, step, tc.expected)
		})
	}
}
