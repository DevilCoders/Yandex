package steps_test

import (
	"context"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func testStep(
	t *testing.T,
	ctx context.Context,
	stepCtx *opcontext.OperationContext,
	step steps.InstanceStep,
	expected steps.RunResult,
) {
	logger, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))
	result := steps.RunStep(ctx, stepCtx, logger, step)
	require.Equal(t, expected.Description, result.Description)
	if expected.Error != nil {
		require.Equal(t, expected.Error.Error(), result.Error.Error())
	} else {
		require.NoError(t, result.Error)
	}
	require.Equal(t, expected.IsDone, result.IsDone)
}
