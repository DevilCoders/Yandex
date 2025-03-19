package steps_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	shipments "a.yandex-team.ru/cloud/mdb/cms/api/internal/shipments/provider"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	deployapimocks "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/mocks"
	models2 "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	metadbmocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
)

func TestWhipMasterStep(t *testing.T) {
	fqdn := "fqdn1"
	type testCase struct {
		name           string
		expected       []steps.RunResult
		prepareFactory func(deploy *deployapimocks.MockClient, mDB *metadbmocks.MockMetaDB)
		stateFactory   func() *models.OperationState
	}

	testCases := []testCase{
		{
			name: "simple shipment workflow",
			expected: []steps.RunResult{
				{
					Description: "created shipment id 42",
					Error:       nil,
					IsDone:      false,
				},
				{
					Description: "shipment 42 is not finished yet",
					Error:       nil,
					IsDone:      false,
				},
				{
					Description: "shipment 42 has been finished successfully",
					Error:       nil,
					IsDone:      true,
				},
			},
			prepareFactory: func(deploy *deployapimocks.MockClient, mDB *metadbmocks.MockMetaDB) {
				deploy.EXPECT().CreateShipment(
					gomock.Any(),
					[]string{fqdn},
					gomock.Any(),
					gomock.Any(),
					gomock.Any(),
					gomock.Any(),
				).Return(models2.Shipment{
					ID:     42,
					FQDNs:  []string{fqdn},
					Status: models2.ShipmentStatusInProgress,
				}, nil)

				deploy.EXPECT().GetShipment(gomock.Any(), models2.ShipmentID(42)).Return(models2.Shipment{
					ID:     42,
					FQDNs:  []string{fqdn},
					Status: models2.ShipmentStatusInProgress,
				}, nil)

				deploy.EXPECT().GetShipment(gomock.Any(), models2.ShipmentID(42)).Return(models2.Shipment{
					ID:     42,
					FQDNs:  []string{fqdn},
					Status: models2.ShipmentStatusDone,
				}, nil)

				mDB.EXPECT().Begin(gomock.Any(), gomock.Any()).DoAndReturn(
					func(ctx context.Context, _ interface{}) (context.Context, error) {
						return ctx, nil
					})

				mDB.EXPECT().Rollback(gomock.Any()).Return(nil)
				mDB.EXPECT().Commit(gomock.Any()).Return(nil)

				mDB.EXPECT().GetHostByFQDN(gomock.Any(), fqdn).Return(
					metadb.Host{
						FQDN:         fqdn,
						SubClusterID: "asd",
					}, nil,
				)

				mDB.EXPECT().GetHostsBySubcid(gomock.Any(), "asd").Return(
					[]metadb.Host{{
						FQDN: fqdn,
					}}, nil,
				)
			},
			stateFactory: func() *models.OperationState {
				return models.DefaultOperationState().SetFQDN(fqdn)
			},
		},
		{
			name: "is not master",
			expected: []steps.RunResult{
				{
					Description: "already checked",
					IsDone:      true,
				},
				{
					Description: "already checked",
					IsDone:      true,
				},
			},
			prepareFactory: func(_ *deployapimocks.MockClient, _ *metadbmocks.MockMetaDB) {},
			stateFactory: func() *models.OperationState {
				return models.DefaultOperationState().SetCheckIsMasterStepState(&models.CheckIsMasterStepState{
					IsNotMaster: true,
					Reason:      "already checked",
				})
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			deploy := deployapimocks.NewMockClient(ctrl)
			mDB := metadbmocks.NewMockMetaDB(ctrl)
			tc.prepareFactory(deploy, mDB)
			provider := shipments.NewShipmentProvider(deploy, mDB)

			step := steps.NewWhipMaster(provider)
			stepCtx := opcontext.NewStepContext(models.ManagementInstanceOperation{
				ID:         "qwe",
				InstanceID: fqdn,
				State:      tc.stateFactory(),
			})

			for _, expected := range tc.expected {
				testStep(t, ctx, stepCtx, step, expected)
			}
		})
	}
}
