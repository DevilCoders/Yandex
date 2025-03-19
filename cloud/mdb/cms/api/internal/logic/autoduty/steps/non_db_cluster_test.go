package steps_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/conductor/mocks"
)

type PGIntTestCaseExp struct {
	msg string
	act steps.AfterStepAction
}

type PGIntTestCase struct {
	name string
	in   int
	exp  PGIntTestCaseExp
}

func TestNonDBClusterStep(t *testing.T) {
	ctx := context.Background()

	t.Run("escalate if not in group", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		cnCl := mocks.NewMockClient(ctrl)
		step := steps.NewNonDatabaseClusterStep(cnCl, 0, "any").(*steps.NonDatabaseClusterStep)
		cnCl.EXPECT().GroupToHosts(gomock.Any(), gomock.Any(), gomock.Any()).Return([]string{
			"other-fqdn",
		}, nil)

		insCtx := steps.NewEmptyInstructionCtx()
		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Fqnds: []string{"any-fqdn"}},
		})

		runRes := step.RunStep(ctx, &insCtx)

		require.Equal(t, steps.AfterStepEscalate, runRes.Action)
		require.Equal(t,
			"don't know about this host, try look at https://c.yandex-team.ru/api/hosts2groups/any-fqdn",
			runRes.ForHuman)
		require.NoError(t, runRes.Error)
	})

	tcs := []PGIntTestCase{
		{name: "give if free slots", in: 2, exp: PGIntTestCaseExp{
			msg: "approve, 1 given in group 'any' and allowed to give 2 max",
			act: steps.AfterStepApprove,
		}},
		{name: "wait if no free slots", in: 1, exp: PGIntTestCaseExp{
			msg: "1 given to Wall-e from group 'any' (should be <= 1): given-fqdn",
			act: steps.AfterStepWait,
		}},
	}

	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			givenFqdn := "given-fqdn"
			currFqdn := "any-fqdn"
			cnCl := mocks.NewMockClient(ctrl)
			step := steps.NewNonDatabaseClusterStep(cnCl, tc.in, "any").(*steps.NonDatabaseClusterStep)
			cnCl.EXPECT().GroupToHosts(gomock.Any(), gomock.Any(), gomock.Any()).Return([]string{
				givenFqdn, currFqdn,
			}, nil)

			insCtx := steps.NewInstructionCtx([]models.ManagementRequest{
				{Fqnds: []string{givenFqdn}, Status: models.StatusOK},
			})
			insCtx.SetActualRD(&types.RequestDecisionTuple{
				R: models.ManagementRequest{Fqnds: []string{currFqdn}},
			})

			runRes := step.RunStep(ctx, &insCtx)

			require.Equal(t, tc.exp.act, runRes.Action)
			require.Equal(t,
				tc.exp.msg,
				runRes.ForHuman)
			require.NoError(t, runRes.Error)
		})
	}
}
