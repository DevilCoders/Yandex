package steps_test

import (
	"context"
	"testing"
	"time"

	"github.com/jonboulle/clockwork"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

func TestPeriodic(t *testing.T) {
	ctx := context.Background()
	insCtx := steps.NewEmptyInstructionCtx()

	opsMetaLog := &models.OpsMetaLog{}

	insCtx.SetActualRD(&types.RequestDecisionTuple{
		R: models.ManagementRequest{Fqnds: []string{dom0fqdn}},
		D: models.AutomaticDecision{OpsLog: opsMetaLog},
	})

	toRun := steps.ListOfSteps(steps.NewApproveManualRequestStep())
	now := time.Date(2022, 06, 01, 12, 00, 00, 00, time.UTC)
	clock := clockwork.NewFakeClockAt(now)
	step := steps.NewPeriodic(toRun, time.Hour, steps.PeriodicWithClock(clock))

	res := step.RunStep(ctx, &insCtx)
	require.NoError(t, res.Error)
	require.Equal(t, steps.AfterStepContinue, res.Action)
	require.Equal(t, toRun(), res.AfterMeSteps)
	require.Equal(t, "time to run", res.ForHuman)
	require.Equal(t, now, opsMetaLog.PeriodicState.LastRun.Time)

	res = step.RunStep(ctx, &insCtx)
	require.NoError(t, res.Error)
	require.Equal(t, steps.AfterStepContinue, res.Action)
	require.Equal(t, "it's not time yet, last run was at 2022-06-01T12:00:00Z", res.ForHuman)
	require.Empty(t, res.AfterMeSteps)
	require.Equal(t, now, opsMetaLog.PeriodicState.LastRun.Time)

	newNow := now.Add(2 * time.Hour)
	clock = clockwork.NewFakeClockAt(newNow)
	step = steps.NewPeriodic(toRun, time.Hour, steps.PeriodicWithClock(clock))
	res = step.RunStep(ctx, &insCtx)
	require.NoError(t, res.Error)
	require.Equal(t, steps.AfterStepContinue, res.Action)
	require.Equal(t, toRun(), res.AfterMeSteps)
	require.Equal(t, "time to run", res.ForHuman)
	require.Equal(t, newNow, opsMetaLog.PeriodicState.LastRun.Time)
}
