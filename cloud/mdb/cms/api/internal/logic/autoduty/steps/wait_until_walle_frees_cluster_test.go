package steps_test

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	mocks3 "a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	cms_models "a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

func TestWaitUntilWalleFreesCluster(t *testing.T) {
	ctx := context.Background()

	t.Run("continue if clusters are free and no other requests", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		localFQDN := "local-fqdn"
		localCluster := "local-cluster"
		dom0d := mocks3.NewMockDom0Discovery(ctrl)
		dom0d.EXPECT().Dom0Instances(gomock.Any(), localFQDN).Return(
			dom0discovery.DiscoveryResult{
				WellKnown: []cms_models.Instance{{DBMClusterName: localCluster}},
			}, nil,
		)

		step := steps.NewWaitUntilWalleFreesCluster(dom0d).(*steps.WaitUntilWalleFreesCluster)

		insCtx := steps.NewEmptyInstructionCtx()

		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Fqnds: []string{localFQDN}},
		})

		runRes := step.RunStep(ctx, &insCtx)

		require.NotNil(t, runRes)
		require.Equal(t, steps.AfterStepContinue, runRes.Action)
		require.Equal(t, "wall-e does not currently perform any operations on any host of these 1 clusters: local-cluster", runRes.ForHuman)
		require.NoError(t, runRes.Error)
	})
}

func TestLearnIfClusterBusy(t *testing.T) {
	ctx := context.Background()

	strNodeSupaCluster := "fqdn on supa-clusta"
	strSupaClusta := "supa-clusta"
	strNodeFromOtherCluster := "fqdn on other cluster"
	newStep := func(ctrl *gomock.Controller) *steps.WaitUntilWalleFreesCluster {
		dom0d := mocks3.NewMockDom0Discovery(ctrl)
		dom0d.EXPECT().Dom0Instances(gomock.Any(), strNodeSupaCluster).Return(
			dom0discovery.DiscoveryResult{
				WellKnown: []cms_models.Instance{{DBMClusterName: strSupaClusta}},
			}, nil,
		).AnyTimes()
		dom0d.EXPECT().Dom0Instances(gomock.Any(), strNodeFromOtherCluster).Return(
			dom0discovery.DiscoveryResult{
				WellKnown: []cms_models.Instance{{DBMClusterName: "some other cluster"}},
			}, nil,
		).AnyTimes()
		return steps.NewWaitUntilWalleFreesCluster(dom0d).(*steps.WaitUntilWalleFreesCluster)
	}
	t.Run("cluster is given away 10 min ago", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		step := newStep(ctrl)
		runres := step.WaitIfAnyClusterBusy(ctx, []string{strSupaClusta}, models.ManagementRequest{
			Fqnds:      []string{strNodeSupaCluster},
			Status:     models.StatusOK,
			ResolvedAt: time.Now().Add(time.Duration(-10) * time.Minute),
		})
		require.NotNil(t, runres)
		require.Equal(t, steps.AfterStepWait, runres.Action)
		require.Equal(t,
			"wait until Wall-e returns cluster 'supa-clusta'. We gave it 10m0s ago.",
			runres.ForHuman)

	})
	t.Run("cluster is free from walle", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		step := newStep(ctrl)

		runres := step.WaitIfAnyClusterBusy(ctx, []string{strSupaClusta}, models.ManagementRequest{
			Fqnds:  []string{strNodeFromOtherCluster},
			Status: models.StatusOK,
		})
		require.Nil(t, runres)
	})
}
