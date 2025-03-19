package steps_test

import (
	"context"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

func TestApproveManual(t *testing.T) {
	ctx := context.Background()

	t.Run("manual approved", func(t *testing.T) {
		step := steps.NewApproveManualRequestStep().(*steps.ApproveManualRequestStep)

		insCtx := steps.NewEmptyInstructionCtx()

		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Type: "manual"},
		})

		runRes := step.RunStep(ctx, &insCtx)

		require.Equal(t, steps.AfterStepApprove, runRes.Action)
		require.Equal(t,
			"approved",
			runRes.ForHuman)
		require.NoError(t, runRes.Error)
	})
	t.Run("not manual disallowed", func(t *testing.T) {
		step := steps.NewApproveManualRequestStep().(*steps.ApproveManualRequestStep)

		insCtx := steps.NewEmptyInstructionCtx()

		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Type: "not-manual"},
		})

		runRes := step.RunStep(ctx, &insCtx)

		require.Equal(t, steps.AfterStepContinue, runRes.Action)
		require.Equal(t,
			"request type is 'not-manual'",
			runRes.ForHuman)
		require.NoError(t, runRes.Error)
	})
}
