package steps_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/dbm/mocks"
)

func TestDisallowNewHosts(t *testing.T) {
	ctx := context.Background()

	t.Run("switch to false when already false", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		dbmClient := mocks.NewMockClient(ctrl)
		step := steps.NewAllowNewHostsStep(dbmClient, false).(*steps.AllowNewHostsStep)
		dbmClient.EXPECT().AreNewHostsAllowed(gomock.Any(), gomock.Any()).Times(1).Return(dbm.NewHostsInfo{
			SetBy:           "not-a-cms-robot",
			NewHostsAllowed: false,
		}, nil)

		insCtx := steps.NewEmptyInstructionCtx()

		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Fqnds: []string{"any-fqdn"}},
		})

		runRes := step.RunStep(ctx, &insCtx)

		require.NoError(t, runRes.Error)
		require.Equal(t, steps.AfterStepContinue, runRes.Action)
		require.Equal(t,
			"allow_new_hosts==false already, but set by 'not-a-cms-robot'. Anyway, I continue",
			runRes.ForHuman)
	})

	t.Run("switch to true leaves false when previously set to false NOT by CMS", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		dbmClient := mocks.NewMockClient(ctrl)
		step := steps.NewAllowNewHostsStep(dbmClient, true).(*steps.AllowNewHostsStep)
		dbmClient.EXPECT().AreNewHostsAllowed(gomock.Any(), gomock.Any()).Times(1).Return(dbm.NewHostsInfo{
			SetBy:           "not-a-cms-robot",
			NewHostsAllowed: false,
		}, nil)

		insCtx := steps.NewEmptyInstructionCtx()

		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Fqnds: []string{"any-fqdn"}},
		})

		runRes := step.RunStep(ctx, &insCtx)

		require.NoError(t, runRes.Error)
		require.Equal(t, steps.AfterStepContinue, runRes.Action)
		require.Equal(t,
			"left allow_new_hosts==false, because it was explicitly set not by me, but by 'not-a-cms-robot'. New hosts will not be placed on this DOM0",
			runRes.ForHuman)
	})

	t.Run("switch to true happy path", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		dbmClient := mocks.NewMockClient(ctrl)
		step := steps.NewAllowNewHostsStep(dbmClient, true).(*steps.AllowNewHostsStep)
		dbmClient.EXPECT().AreNewHostsAllowed(gomock.Any(), gomock.Any()).Times(1).Return(dbm.NewHostsInfo{
			SetBy:           "robot-mdb-cms-porto",
			NewHostsAllowed: false,
		}, nil)
		dbmClient.EXPECT().UpdateNewHostsAllowed(gomock.Any(), gomock.Any(), gomock.Any()).Times(1).Return(nil)

		insCtx := steps.NewEmptyInstructionCtx()

		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Fqnds: []string{"any-fqdn"}},
		})

		runRes := step.RunStep(ctx, &insCtx)

		require.NoError(t, runRes.Error)
		require.Equal(t, steps.AfterStepContinue, runRes.Action)
		require.Equal(t,
			"set to true",
			runRes.ForHuman)
	})
}
