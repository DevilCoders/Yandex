package steps_test

import (
	"context"
	"fmt"
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
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	mlockmocks "a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient/mocks"
)

func TestUnlockOrWait(t *testing.T) {
	type input struct {
		lockID  string
		prepare func(locker *mlockmocks.MockLocker)
	}
	type expect struct {
		action     steps.AfterStepAction
		forHuman   string
		afterSteps []steps.DecisionStep
		err        func(err error) bool
	}
	type testCase struct {
		name   string
		input  input
		expect expect
	}
	testCases := []testCase{
		{
			"lock not found",
			input{
				lockID: "test-lock",
				prepare: func(locker *mlockmocks.MockLocker) {
					locker.EXPECT().ReleaseLock(gomock.Any(), "test-lock").AnyTimes().Return(
						semerr.NotFound("not found"))
				},
			},
			expect{
				action:     steps.AfterStepContinue,
				forHuman:   "lock 'test-lock' is not taken",
				afterSteps: []steps.DecisionStep(nil),
			},
		},
		{
			"released with error",
			input{
				lockID: "test-lock",
				prepare: func(locker *mlockmocks.MockLocker) {
					locker.EXPECT().ReleaseLock(gomock.Any(), "test-lock").AnyTimes().Return(
						fmt.Errorf("unkown test error"))
				},
			},
			expect{
				action:     steps.AfterStepWait,
				forHuman:   "failed to release lock 'test-lock'",
				afterSteps: []steps.DecisionStep(nil),
				err: func(err error) bool {
					return err.Error() == "unkown test error"
				},
			},
		},
		{
			"happy path released lock",
			input{
				lockID: "test-lock",
				prepare: func(locker *mlockmocks.MockLocker) {
					locker.EXPECT().ReleaseLock(gomock.Any(), "test-lock").AnyTimes().Return(
						nil)
				},
			},
			expect{
				action:     steps.AfterStepContinue,
				forHuman:   "released lock 'test-lock'",
				afterSteps: []steps.DecisionStep(nil),
			},
		},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			input := tc.input
			expect := tc.expect
			ctx := context.Background()
			locker := mlockmocks.NewMockLocker(ctrl)
			input.prepare(locker)
			result := steps.UnlockOrWaitDom0(ctx, locker, input.lockID)
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

func TestUnlockClusterNodesStep(t *testing.T) {
	dom0fqdn := "dom0fqdn"
	decisionID := int64(42)
	fqdn1 := "fqdn1"
	fqdn2 := "fqdn2"
	fqdn3 := "fqdn3"
	lockID1 := "cms-42-cluster1"
	lockID2 := "cms-42-cluster2"
	type iteration struct {
		prepare  func(l *mocks.MockLocker, locks map[string]*lockcluster.State)
		validate func(result steps.RunResult, t *testing.T)
	}
	tcs := []struct {
		name       string
		iterations []iteration
	}{
		{
			name: "success",
			iterations: []iteration{
				{
					prepare: func(l *mocks.MockLocker, locks map[string]*lockcluster.State) {
						l.EXPECT().ReleaseCluster(gomock.Any(), locks[fqdn1]).Return(nil)
						l.EXPECT().ReleaseCluster(gomock.Any(), locks[fqdn2]).Return(nil)
					},
					validate: func(result steps.RunResult, t *testing.T) {
						require.NoError(t, result.Error)
						require.Equal(t, steps.AfterStepContinue, result.Action)
						require.Equal(t, "released locks: cms-42-cluster1, cms-42-cluster2", result.ForHuman)
					},
				},
			},
		},
		{
			name: "unavailable",
			iterations: []iteration{
				{
					prepare: func(l *mocks.MockLocker, locks map[string]*lockcluster.State) {
						l.EXPECT().ReleaseCluster(gomock.Any(), locks[fqdn1]).Return(semerr.Unavailable("the error"))
					},
					validate: func(result steps.RunResult, t *testing.T) {
						require.Error(t, result.Error)
						require.True(t, semerr.IsUnavailable(result.Error))
						require.Equal(t, steps.AfterStepWait, result.Action)
						require.Equal(t, "could not release lock for \"fqdn1\"."+
							"\nReleased locks: ",
							result.ForHuman,
						)
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
					WellKnown: []intmodels.Instance{{FQDN: fqdn1}, {FQDN: fqdn2}, {FQDN: fqdn3}},
					Unknown:   []string{"unknown"},
				}, nil).MaxTimes(2).MinTimes(1)

			ops := models.OpsMetaLog{LocksState: &opmetas.LocksStateMeta{Locks: map[string]*lockcluster.State{
				fqdn1: {LockID: lockID1},
				fqdn2: {LockID: lockID2},
			}}}

			step := steps.NewUnlockClusterNodesStep(l, d)
			execCtx := steps.NewEmptyInstructionCtx()
			execCtx.SetActualRD(&types.RequestDecisionTuple{
				D: models.AutomaticDecision{ID: decisionID, OpsLog: &ops},
				R: models.ManagementRequest{Fqnds: []string{dom0fqdn}},
			})

			for _, iter := range tc.iterations {
				t.Run("", func(t *testing.T) {
					iter.prepare(l, ops.LocksState.Locks)
					result := step.RunStep(ctx, &execCtx)
					iter.validate(result, t)
				})
			}
		})
	}
}
