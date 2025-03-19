package steps_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	swmodels "a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	deployapi2 "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployapi "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/mocks"
	models2 "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestUnregisterStep(t *testing.T) {
	ctx := context.Background()
	t.Run("happy path", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		dapi := deployapi.NewMockClient(ctrl)
		dapi.EXPECT().GetMinionMaster(gomock.Any(), gomock.Any()).Return(
			deployapi2.MinionMaster{},
			nil)
		dapi.EXPECT().UnregisterMinion(gomock.Any(), gomock.Any()).Return(models2.Minion{}, nil)
		step := steps.NewUnregisterStep(dapi)

		insCtx := steps.NewEmptyInstructionCtx()
		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Name: swmodels.ManagementRequestActionRedeploy, Fqnds: []string{"any-fqdn"}},
		})

		res := step.RunStep(ctx, &insCtx)

		require.Equal(t, steps.AfterStepContinue, res.Action)
		require.Equal(t, "ok", res.ForHuman)
	})
	t.Run("not exists", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		dapi := deployapi.NewMockClient(ctrl)
		dapi.EXPECT().GetMinionMaster(gomock.Any(), gomock.Any()).Return(
			deployapi2.MinionMaster{},
			deployapi2.ErrNotFound.Wrap(xerrors.New("for tests")))
		step := steps.NewUnregisterStep(dapi)

		insCtx := steps.NewEmptyInstructionCtx()
		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Name: swmodels.ManagementRequestActionRedeploy, Fqnds: []string{"any-fqdn"}},
		})

		res := step.RunStep(ctx, &insCtx)

		require.Equal(t, steps.AfterStepContinue, res.Action)
		require.Equal(t, "not found in deploy", res.ForHuman)
	})
	t.Run("deploy api error", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		dapi := deployapi.NewMockClient(ctrl)
		dapi.EXPECT().GetMinionMaster(gomock.Any(), gomock.Any()).Return(
			deployapi2.MinionMaster{},
			nil)
		dapi.EXPECT().UnregisterMinion(gomock.Any(), gomock.Any()).Return(models2.Minion{}, xerrors.New("error-for-tests"))
		step := steps.NewUnregisterStep(dapi)

		insCtx := steps.NewEmptyInstructionCtx()
		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Name: swmodels.ManagementRequestActionRedeploy, Fqnds: []string{"any-fqdn"}},
		})

		res := step.RunStep(ctx, &insCtx)

		require.Equal(t, steps.AfterStepWait, res.Action)
		require.Equal(t, "deploy api failed", res.ForHuman)
		require.Error(t, res.Error)
		require.Equal(t, res.Error.Error(), "error-for-tests")
	})
	t.Run("wrong action", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		dapi := deployapi.NewMockClient(ctrl)
		step := steps.NewUnregisterStep(dapi)

		insCtx := steps.NewEmptyInstructionCtx()
		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Name: "reboot", Fqnds: []string{"any-fqdn"}},
		})

		res := step.RunStep(ctx, &insCtx)

		require.Equal(t, steps.AfterStepContinue, res.Action)
		require.Equal(t, "not redeploy action, step skipped", res.ForHuman)
	})
}
