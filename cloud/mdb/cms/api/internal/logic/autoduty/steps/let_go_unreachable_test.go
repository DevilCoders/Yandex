package steps_test

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

type testReachabilityChecker struct {
	result juggler.FQDNGroupByJugglerCheck
	err    error
}

func (rc *testReachabilityChecker) Check(_ context.Context, _ []string, _ []string, _ time.Time) (juggler.FQDNGroupByJugglerCheck, error) {
	return rc.result, rc.err
}

func TestUnreachableStep(t *testing.T) {
	const (
		dom0fqdn           = "man1.db.yandex.net"
		unreachableComment = "wall-e thinks it's unreachable"
	)
	now := time.Date(2020, 12, 10, 10, 45, 0, 0, time.UTC)

	type input struct {
		reachabilityChecker testReachabilityChecker
		waitFor             time.Duration
		action              steps.AfterStepAction
		walleComment        string
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
	tcs := []testCase{{
		name: "unreachable in text, unreachable in juggler",
		expect: expect{
			action:   steps.AfterStepContinue,
			forHuman: "yes, unreachable in juggler",
		},
		input: input{
			reachabilityChecker: testReachabilityChecker{
				result: juggler.FQDNGroupByJugglerCheck{
					NotOK: []string{dom0fqdn},
				},
			},
			waitFor:      time.Hour,
			action:       steps.AfterStepContinue,
			walleComment: unreachableComment,
		},
	}, {
		name: "reachable in text, unreachable in juggler",
		expect: expect{
			action:   steps.AfterStepContinue,
			forHuman: "yes, unreachable in juggler",
		},
		input: input{
			reachabilityChecker: testReachabilityChecker{
				result: juggler.FQDNGroupByJugglerCheck{
					NotOK: []string{dom0fqdn},
				},
			},
			waitFor:      time.Hour,
			action:       steps.AfterStepContinue,
			walleComment: "reachable",
		},
	}, {
		name: "unreachable in text, reachable in juggler",
		expect: expect{
			action:   steps.AfterStepContinue,
			forHuman: "waited 1h0m0s",
		},
		input: input{
			reachabilityChecker: testReachabilityChecker{
				result: juggler.FQDNGroupByJugglerCheck{
					OK: []string{dom0fqdn},
				},
			},
			waitFor:      time.Hour,
			action:       steps.AfterStepContinue,
			walleComment: unreachableComment,
		},
	}, {
		name: "reachable in text, reachable in juggler",
		expect: expect{
			action:   steps.AfterStepContinue,
			forHuman: "no",
		},
		input: input{
			reachabilityChecker: testReachabilityChecker{
				result: juggler.FQDNGroupByJugglerCheck{
					OK: []string{dom0fqdn},
				},
			},
			waitFor:      time.Hour,
			action:       steps.AfterStepContinue,
			walleComment: "reachable",
		},
	}, {
		name: "unreachable in text, error in juggler",
		expect: expect{
			action:   steps.AfterStepWait,
			forHuman: "error in juggler, will try again",
			err: func(err error) bool {
				return semerr.IsUnavailable(err)
			},
		},
		input: input{
			reachabilityChecker: testReachabilityChecker{
				err: semerr.Unavailable("unavailable"),
			},
			waitFor: time.Hour,
			action:  steps.AfterStepContinue,
		},
	}}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			step := steps.NewCustomActOnUnreachableStep(tc.input.waitFor, &tc.input.reachabilityChecker, tc.input.action)
			ctx := context.Background()
			insCtx := steps.NewEmptyInstructionCtx()

			insCtx.SetActualRD(&types.RequestDecisionTuple{
				R: models.ManagementRequest{
					Fqnds:     []string{dom0fqdn},
					CreatedAt: now.Add(-time.Second),
					Comment:   tc.input.walleComment,
				},
			})
			result := step.RunStep(ctx, &insCtx)
			if tc.expect.err != nil {
				require.True(t, tc.expect.err(result.Error))
			} else {
				require.NoError(t, result.Error)
			}
			require.Equal(t, tc.expect.forHuman, result.ForHuman)
			require.Equal(t, tc.expect.action, result.Action)
			require.Equal(t, tc.expect.afterSteps, result.AfterMeSteps)
		})
	}

}

func TestWaitTillTimeElapses(t *testing.T) {
	now := time.Date(2020, 12, 10, 10, 45, 0, 0, time.UTC)
	t.Run("happy paths", func(t *testing.T) {
		res := steps.WaitTillTimeElapses(now, now.Add(-time.Minute), time.Minute*5, steps.AfterStepApprove)
		require.Equal(t, "will wait till 2020-12-10 10:49:00 +0000 UTC", res.ForHuman)
		require.Equal(t, steps.AfterStepWait, res.Action)

		res = steps.WaitTillTimeElapses(now, now.Add(-time.Minute*6), time.Minute*5, steps.AfterStepApprove)
		require.Equal(t, "waited 5m0s", res.ForHuman)
		require.Equal(t, steps.AfterStepApprove, res.Action)
	})
}
