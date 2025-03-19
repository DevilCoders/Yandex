package steps_test

import (
	"context"
	"strconv"
	"strings"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

func TestWhipPrimaries(t *testing.T) {
	ctx := context.Background()
	ctrl := gomock.NewController(t)
	fqdn := "container_fqdn"
	dom0 := "dom0_fqdn"
	OpID := "123"
	instanceID := strings.Join([]string{dom0, fqdn}, ":")

	defer ctrl.Finish()

	t.Run("No containers on dom0", func(t *testing.T) {
		opsMetaLog := models.NewOpsMetaLog()
		insCtx := steps.NewEmptyInstructionCtx()

		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Fqnds: []string{dom0}},
			D: models.AutomaticDecision{OpsLog: &opsMetaLog},
		})
		cmsInstance := mocks.NewMockInstanceClient(ctrl)

		allKnownContainers := []*api.Instance{}

		cmsInstance.EXPECT().ResolveInstancesByDom0(ctx, []string{dom0}).Times(1).Return(&api.InstanceListResponce{
			Instance: allKnownContainers,
		}, nil)

		step := steps.NewCmsInstanceWhipPrimariesRequestStep(cmsInstance)
		result := step.RunStep(ctx, &insCtx)
		require.Equal(t, nil, result.Error)
	})
	t.Run("Successful operation after first iteration", func(t *testing.T) {
		opsMetaLog := models.NewOpsMetaLog()
		insCtx := steps.NewEmptyInstructionCtx()

		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Fqnds: []string{dom0}},
			D: models.AutomaticDecision{OpsLog: &opsMetaLog},
		})
		cmsInstance := mocks.NewMockInstanceClient(ctrl)

		allKnownContainers := []*api.Instance{
			{
				Dom0: dom0,
				Fqdn: fqdn,
			},
		}

		idem, err := idempotence.New()
		require.NoError(t, err)
		ctx = idempotence.WithOutgoing(ctx, idem)

		cmsInstance.EXPECT().ResolveInstancesByDom0(ctx, []string{dom0}).Times(1).Return(&api.InstanceListResponce{
			Instance: allKnownContainers,
		}, nil)

		cmsInstance.EXPECT().CreateWhipPrimaryOperation(ctx, instanceID, "").Return(&api.InstanceOperation{
			Id:     OpID,
			Status: api.InstanceOperation_OK,
		}, nil)

		cmsInstance.EXPECT().GetInstanceOperation(ctx, OpID).Return(&api.InstanceOperation{
			Id:     OpID,
			Status: api.InstanceOperation_OK,
		}, nil)

		step := steps.NewCmsInstanceWhipPrimariesRequestStep(cmsInstance)
		result := step.RunStep(ctx, &insCtx)
		require.Equal(t, nil, result.Error)
	})
	t.Run("Successful operation after a few iteration", func(t *testing.T) {
		opsMetaLog := models.NewOpsMetaLog()
		insCtx := steps.NewEmptyInstructionCtx()

		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Fqnds: []string{dom0}},
			D: models.AutomaticDecision{OpsLog: &opsMetaLog},
		})
		cmsInstance := mocks.NewMockInstanceClient(ctrl)

		iteration := 10
		allKnownContainers := []*api.Instance{}

		for idx := 0; idx < iteration; idx++ {
			idem, err := idempotence.New()
			require.NoError(t, err)
			ctx = idempotence.WithOutgoing(ctx, idem)

			allKnownContainers = append(allKnownContainers, &api.Instance{Dom0: dom0, Fqdn: strconv.Itoa(idx)})
			instanceID := strings.Join([]string{dom0, strconv.Itoa(idx)}, ":")
			cmsInstance.EXPECT().CreateWhipPrimaryOperation(ctx, instanceID, "").Return(&api.InstanceOperation{
				Id:     strconv.Itoa(idx),
				Status: api.InstanceOperation_PROCESSING,
			}, nil)
		}

		step := steps.NewCmsInstanceWhipPrimariesRequestStep(cmsInstance)

		for idx := 0; idx < iteration; idx++ {
			cmsInstance.EXPECT().ResolveInstancesByDom0(ctx, []string{dom0}).Return(&api.InstanceListResponce{
				Instance: allKnownContainers,
			}, nil)

			status := api.InstanceOperation_PROCESSING
			for j := 0; j < iteration; j++ {
				if j > iteration-1-idx {
					status = api.InstanceOperation_OK
				}
				cmsInstance.EXPECT().GetInstanceOperation(ctx, strconv.Itoa(j)).Return(&api.InstanceOperation{
					Id:     strconv.Itoa(j),
					Status: status,
				}, nil)
			}

			result := step.RunStep(ctx, &insCtx)
			require.Equal(t, nil, result.Error)

			allKnownContainers = append(allKnownContainers[:0], allKnownContainers[1:]...)
		}
	})
	t.Run("WhipPrimary request failed", func(t *testing.T) {
		opsMetaLog := models.NewOpsMetaLog()
		insCtx := steps.NewEmptyInstructionCtx()

		error := semerr.NotFound("ERROR")
		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Fqnds: []string{dom0}},
			D: models.AutomaticDecision{OpsLog: &opsMetaLog},
		})
		cmsInstance := mocks.NewMockInstanceClient(ctrl)

		allKnownContainers := []*api.Instance{
			{
				Dom0: dom0,
				Fqdn: fqdn,
			},
		}

		idem, err := idempotence.New()
		require.NoError(t, err)
		ctx = idempotence.WithOutgoing(ctx, idem)

		cmsInstance.EXPECT().ResolveInstancesByDom0(ctx, []string{dom0}).Return(&api.InstanceListResponce{
			Instance: allKnownContainers,
		}, nil)

		cmsInstance.EXPECT().CreateWhipPrimaryOperation(ctx, instanceID, "").Return(&api.InstanceOperation{
			Id:     OpID,
			Status: api.InstanceOperation_OK,
		}, error)
		step := steps.NewCmsInstanceWhipPrimariesRequestStep(cmsInstance)
		result := step.RunStep(ctx, &insCtx)
		require.Equal(t, error, result.Error)
	})
	t.Run("WhipPrimary request return ERROR", func(t *testing.T) {
		opsMetaLog := models.NewOpsMetaLog()
		insCtx := steps.NewEmptyInstructionCtx()

		insCtx.SetActualRD(&types.RequestDecisionTuple{
			R: models.ManagementRequest{Fqnds: []string{dom0}},
			D: models.AutomaticDecision{OpsLog: &opsMetaLog},
		})
		cmsInstance := mocks.NewMockInstanceClient(ctrl)

		allKnownContainers := []*api.Instance{
			{
				Dom0: dom0,
				Fqdn: fqdn,
			},
		}

		idem, err := idempotence.New()
		require.NoError(t, err)
		ctx = idempotence.WithOutgoing(ctx, idem)

		cmsInstance.EXPECT().ResolveInstancesByDom0(ctx, []string{dom0}).Times(1).Return(&api.InstanceListResponce{
			Instance: allKnownContainers,
		}, nil)

		cmsInstance.EXPECT().CreateWhipPrimaryOperation(ctx, instanceID, "").Times(1).Return(&api.InstanceOperation{
			Id:     OpID,
			Status: api.InstanceOperation_PROCESSING,
		}, nil)

		cmsInstance.EXPECT().GetInstanceOperation(ctx, OpID).Times(1).Return(&api.InstanceOperation{
			Id:     OpID,
			Status: api.InstanceOperation_ERROR,
		}, nil)

		step := steps.NewCmsInstanceWhipPrimariesRequestStep(cmsInstance)
		result := step.RunStep(ctx, &insCtx)
		require.Equal(t, nil, result.Error)
	})
}
