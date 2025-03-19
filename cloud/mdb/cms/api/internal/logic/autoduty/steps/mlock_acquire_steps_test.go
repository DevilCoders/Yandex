package steps_test

import (
	"context"
	"fmt"
	"strconv"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	dom0discoverymocks "a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	intmodels "a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
	mlockmocks "a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient/mocks"
)

func TestAcquireOrWaitDom0(t *testing.T) {
	const holderName = "test-holder"
	type input struct {
		reason  string
		lockID  string
		objects []string
		holder  string
		prepare func(locker *mlockmocks.MockLocker)
	}
	type output struct {
		action     steps.AfterStepAction
		forHuman   string
		afterSteps []steps.DecisionStep
		err        func(err error) bool
	}
	type testCase struct {
		name   string
		input  input
		expect output
	}
	tcs := []testCase{
		{
			name: "lock cannot be created",
			input: input{
				"reboot for 10 containers",
				"test-lock-id",
				[]string{},
				holderName,
				func(locker *mlockmocks.MockLocker) {
					locker.EXPECT().
						CreateLock(gomock.Any(), "test-lock-id", holderName, []string{}, gomock.Any()).
						Return(fmt.Errorf("test-error-reason"))
				}},
			expect: output{
				action:     steps.AfterStepWait,
				forHuman:   "could not create lock \"test-lock-id\"",
				afterSteps: []steps.DecisionStep(nil),
				err: func(err error) bool {
					return err.Error() == "test-error-reason"
				},
			},
		},
		{
			name: "lock is taken by someone else",
			input: input{
				"reboot for 2 containers",
				"test-lock-id",
				[]string{"man1.db.y.net", "sas1.db.y.net"},
				holderName,
				func(locker *mlockmocks.MockLocker) {
					locker.EXPECT().
						CreateLock(
							gomock.Any(),
							"test-lock-id",
							holderName,
							[]string{"man1.db.y.net", "sas1.db.y.net"},
							gomock.Any(),
						).Return(
						nil,
					)
					locker.EXPECT().GetLockStatus(
						gomock.Any(),
						"test-lock-id",
					).Return(
						mlockclient.LockStatus{
							Acquired:  false,
							Conflicts: []mlockclient.Conflict{{Object: "sas1.db.y.net", LockIDs: []string{"other-lock"}}},
						}, nil,
					)
				}},
			expect: output{
				action:     steps.AfterStepWait,
				forHuman:   "will try again. Could not acquire lock \"test-lock-id\" because of conflicts: sas1.db.y.net locked by other-lock",
				afterSteps: []steps.DecisionStep(nil),
			},
		},
		{
			name: "happy path lock taken",
			input: input{
				"reboot for 2 containers",
				"test-lock-id",
				[]string{"man1.db.y.net", "sas1.db.y.net"},
				holderName,
				func(locker *mlockmocks.MockLocker) {
					locker.EXPECT().
						CreateLock(
							gomock.Any(),
							"test-lock-id",
							holderName,
							[]string{"man1.db.y.net", "sas1.db.y.net"},
							gomock.Any(),
						).Return(
						nil,
					)
					locker.EXPECT().GetLockStatus(
						gomock.Any(),
						"test-lock-id",
					).Return(
						mlockclient.LockStatus{
							Acquired: true,
						}, nil,
					)
				}},
			expect: output{
				action:     steps.AfterStepContinue,
				forHuman:   "acquired lock \"test-lock-id\"",
				afterSteps: []steps.DecisionStep(nil),
			},
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			input := tc.input
			expect := tc.expect
			ctx := context.Background()
			locker := mlockmocks.NewMockLocker(ctrl)
			input.prepare(locker)
			result := steps.AcquireOrWaitDom0(ctx, locker, input.lockID, input.objects, input.reason, input.holder)
			if expect.err != nil {
				require.Error(t, result.Error)
				require.True(t, expect.err(result.Error))
			} else {
				require.NoError(t, result.Error)
			}
			require.Equal(t, expect.forHuman, result.ForHuman)
			require.Equal(t, expect.action, result.Action)
			require.Equal(t, expect.afterSteps, result.AfterMeSteps)
		})
	}
}

func TestLockClusterNodesStep(t *testing.T) {
	dom0fqdn := "dom0fqdn"
	decisionID := int64(42)
	decStr := strconv.FormatInt(decisionID, 10)
	fqdn1 := "fqdn1"
	fqdn2 := "fqdn2"
	lockID1 := "cms-42-cluster1"
	lockID2 := "cms-42-cluster2"
	type iteration struct {
		prepare  func(l *mocks.MockLocker)
		validate func(result steps.RunResult, state models.OpsMetaLog, t *testing.T)
	}
	tcs := []struct {
		name       string
		iterations []iteration
	}{
		{
			name: "success",
			iterations: []iteration{
				{
					prepare: func(l *mocks.MockLocker) {
						l.EXPECT().LockCluster(gomock.Any(), fqdn1, decStr, lockcluster.WalleCMS).Return(&lockcluster.State{LockID: lockID1, IsTaken: true}, nil)
						l.EXPECT().LockCluster(gomock.Any(), fqdn2, decStr, lockcluster.WalleCMS).Return(&lockcluster.State{LockID: lockID2, IsTaken: true}, nil)
					},
					validate: func(result steps.RunResult, state models.OpsMetaLog, t *testing.T) {
						require.NoError(t, result.Error)
						require.Equal(t, steps.AfterStepContinue, result.Action)
						require.Equal(t, "acquired locks: cms-42-cluster1, cms-42-cluster2", result.ForHuman)
						require.NotNil(t, state.LocksState)
						require.Equal(t, map[string]*lockcluster.State{
							fqdn1: {LockID: lockID1, IsTaken: true},
							fqdn2: {LockID: lockID2, IsTaken: true},
						}, state.LocksState.Locks)
					},
				},
				{
					prepare: func(l *mocks.MockLocker) {},
					validate: func(result steps.RunResult, state models.OpsMetaLog, t *testing.T) {
						require.NoError(t, result.Error)
						require.Equal(t, steps.AfterStepContinue, result.Action)
						require.Equal(t, "acquired locks: cms-42-cluster1, cms-42-cluster2", result.ForHuman)
						require.NotNil(t, state.LocksState)
						require.Equal(t, map[string]*lockcluster.State{
							fqdn1: {LockID: lockID1, IsTaken: true},
							fqdn2: {LockID: lockID2, IsTaken: true},
						}, state.LocksState.Locks)
					},
				},
			},
		},
		{
			name: "one conflict",
			iterations: []iteration{
				{
					prepare: func(l *mocks.MockLocker) {
						l.EXPECT().LockCluster(gomock.Any(), fqdn1, decStr, lockcluster.WalleCMS).Return(&lockcluster.State{LockID: lockID1, IsTaken: true}, nil)
						l.EXPECT().LockCluster(gomock.Any(), fqdn2, decStr, lockcluster.WalleCMS).Return(
							&lockcluster.State{LockID: lockID2}, lockcluster.NewError(
								lockcluster.NotAcquiredConflicts,
								"could not acquire lock \"cms-42-cluster2\" because of conflicts: fqdn2 locked by anotherlock",
							))
					},
					validate: func(result steps.RunResult, state models.OpsMetaLog, t *testing.T) {
						require.NoError(t, result.Error)
						require.Equal(t, steps.AfterStepWait, result.Action)
						require.Equal(t, "will try again later."+
							"\nCreated locks: cms-42-cluster1."+
							"\nConflicts:"+
							"\ncould not acquire lock \"cms-42-cluster2\" because of conflicts: fqdn2 locked by anotherlock",
							result.ForHuman,
						)
						require.NotNil(t, state.LocksState)
						require.Equal(t, map[string]*lockcluster.State{
							fqdn1: {LockID: lockID1, IsTaken: true},
							fqdn2: {LockID: lockID2},
						}, state.LocksState.Locks)
					},
				},
				{
					prepare: func(l *mocks.MockLocker) {
						l.EXPECT().LockCluster(gomock.Any(), fqdn1, decStr, lockcluster.WalleCMS).Return(&lockcluster.State{LockID: lockID1, IsTaken: true}, nil)
						l.EXPECT().LockCluster(gomock.Any(), fqdn2, decStr, lockcluster.WalleCMS).Return(
							&lockcluster.State{LockID: lockID2}, lockcluster.NewError(
								lockcluster.NotAcquiredConflicts,
								"could not acquire lock \"cms-42-cluster2\" because of conflicts: fqdn2 locked by anotherlock",
							))
					},
					validate: func(result steps.RunResult, state models.OpsMetaLog, t *testing.T) {
						require.NoError(t, result.Error)
						require.Equal(t, steps.AfterStepWait, result.Action)
						require.Equal(t, "will try again later."+
							"\nCreated locks: cms-42-cluster1."+
							"\nConflicts:"+
							"\ncould not acquire lock \"cms-42-cluster2\" because of conflicts: fqdn2 locked by anotherlock",
							result.ForHuman,
						)
						require.NotNil(t, state.LocksState)
						require.Equal(t, map[string]*lockcluster.State{
							fqdn1: {LockID: lockID1, IsTaken: true},
							fqdn2: {LockID: lockID2},
						}, state.LocksState.Locks)
					},
				},
			},
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			l := mocks.NewMockLocker(ctrl)
			d := dom0discoverymocks.NewMockDom0Discovery(ctrl)
			d.EXPECT().Dom0Instances(gomock.Any(), dom0fqdn).Return(
				dom0discovery.DiscoveryResult{
					WellKnown: []intmodels.Instance{{FQDN: fqdn1}, {FQDN: fqdn2}},
					Unknown:   []string{"unknown"},
				}, nil).MaxTimes(2)

			step := steps.NewLockContainersStep(l, d)
			opsMetaLog := models.NewOpsMetaLog()
			execCtx := steps.NewEmptyInstructionCtx()
			execCtx.SetActualRD(&types.RequestDecisionTuple{
				D: models.AutomaticDecision{ID: decisionID, OpsLog: &opsMetaLog},
				R: models.ManagementRequest{Fqnds: []string{dom0fqdn}},
			})

			for _, iter := range tc.iterations {
				t.Run("", func(t *testing.T) {
					iter.prepare(l)
					result := step.RunStep(ctx, &execCtx)
					iter.validate(result, opsMetaLog, t)
				})
			}
		})
	}
}
