package steps_test

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployapimocks "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/mocks"
	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
)

const (
	expTextOnTimeout = `This is unrecoverable till MDB-9691 and/or MDB-9634.
Please check why errors happened. Then click "OK -> AT-WALLE" button to continue.

0 failed:
1 timeout:container.test
0 success:`

	expTextOnSkipTimeout = `0 success:
1 ignored because skip timeouts: container.test`
)

type DeployShipmentTest struct {
}

func (sm DeployShipmentTest) GetShipments() opmetas.ShipmentsInfo {
	return opmetas.ShipmentsInfo{
		"dom0.test": []deploymodels.ShipmentID{deploymodels.ShipmentID(1)},
	}
}

func (sm DeployShipmentTest) SetShipment(string, deploymodels.ShipmentID) {
}

func getDapi(ctrl *gomock.Controller) *deployapimocks.MockClient {
	dapi := deployapimocks.NewMockClient(ctrl)
	dapi.EXPECT().GetMinionMaster(gomock.Any(), gomock.Any()).Return(deployapi.MinionMaster{
		MasterFQDN: "salt-master.test",
	}, nil)
	return dapi
}

func TestShipments(t *testing.T) {
	cfg := shipments.AwaitShipmentConfig{}
	rd := types.RequestDecisionTuple{
		R: models.ManagementRequest{ID: 132, Status: models.StatusInProcess},
		D: models.AutomaticDecision{},
	}
	cmds := []deploymodels.CommandDef{{
		Type:    "cmd.run",
		Args:    []string{"echo 1"},
		Timeout: encodingutil.FromDuration(time.Minute * time.Duration(cfg.Timeout)),
	}}
	ctx := context.Background()
	unusedMetaContainer := DeployShipmentTest{}

	t.Run("create", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		dapi := getDapi(ctrl)
		dapi.EXPECT().CreateShipment(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(deploymodels.Shipment{
			ID: 1,
		}, nil)
		dwrapper := shipments.NewDeployWrapperFromCfg(cmds, dapi, cfg)
		rr := steps.CreateShipment(ctx, []string{"container.test"}, unusedMetaContainer, dwrapper)
		require.Equal(t, `created shipments
successfully: 1`, rr.ForHuman)
	})

	t.Run("wait on timeout-ed success", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		dapi := getDapi(ctrl)
		dapi.EXPECT().GetShipment(gomock.Any(), gomock.Any()).Return(deploymodels.Shipment{
			ID:     1,
			FQDNs:  []string{"container.test"},
			Status: deploymodels.ShipmentStatusTimeout,
		}, nil)
		dwrapper := shipments.NewDeployWrapperFromCfg(cmds, dapi, cfg)
		rr := steps.WaitShipment(ctx, DeployShipmentTest{}, dwrapper, &rd)
		require.Equal(t, expTextOnTimeout, rr.ForHuman)
		require.Equal(t, steps.AfterStepWait, rr.Action)
	})

	t.Run("continue on skipped timeout", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		dapi := getDapi(ctrl)
		dapi.EXPECT().GetShipment(gomock.Any(), gomock.Any()).Return(deploymodels.Shipment{
			ID:     1,
			FQDNs:  []string{"container.test"},
			Status: deploymodels.ShipmentStatusTimeout,
		}, nil)
		dwrapper := shipments.NewDeployWrapper(cmds, dapi, cfg.StopOnErrCount, cfg.MaxParallelRuns, cfg.Timeout, shipments.WithSkipTimeouts())
		rr := steps.WaitShipment(ctx, DeployShipmentTest{}, dwrapper, &rd)
		require.Equal(t, expTextOnSkipTimeout, rr.ForHuman)
		require.Equal(t, steps.AfterStepContinue, rr.Action)
	})

	t.Run("wait in progress", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		dapi := getDapi(ctrl)
		dapi.EXPECT().GetShipment(gomock.Any(), gomock.Any()).Return(deploymodels.Shipment{
			ID:     1,
			FQDNs:  []string{"container.test"},
			Status: deploymodels.ShipmentStatusInProgress,
		}, nil)
		dwrapper := shipments.NewDeployWrapperFromCfg(cmds, dapi, cfg)
		rr := steps.WaitShipment(ctx, DeployShipmentTest{}, dwrapper, &rd)
		require.Equal(t, steps.AfterStepWait, rr.Action)
		require.Equal(t, `0 success:
1 in progress:container.test`, rr.ForHuman)
	})

	t.Run("not registered in deploy", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		dapi := deployapimocks.NewMockClient(ctrl)
		dapi.EXPECT().GetMinionMaster(gomock.Any(), gomock.Any()).Return(
			deployapi.MinionMaster{},
			deployapi.ErrNotFound)
		dwrapper := shipments.NewDeployWrapperFromCfg(cmds, dapi, cfg)
		rr := steps.CreateShipment(ctx, []string{"container.test"}, unusedMetaContainer, dwrapper)
		require.Equal(t, `created shipments
successfully: 0
not in deploy: container.test`, rr.ForHuman)
	})
}
