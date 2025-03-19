package steps_test

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/mwswitch"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/mocks"
	healthtypes "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

func TestCheckIsMaster(t *testing.T) {
	fqdn := "fqdn1"
	type testCase struct {
		name           string
		expected       []steps.RunResult
		prepareFactory func(health *mocks.MockMDBHealthClient)
		isCompute      bool
		finishWorkflow bool
	}
	testCases := []testCase{
		{
			name: "master, continue",
			expected: []steps.RunResult{
				{
					IsDone:      true,
					Description: "host is master for service \"qwerty\"",
				},
				{
					IsDone:      true,
					Description: "host is master for service \"qwerty\"",
				},
			},
			prepareFactory: func(health *mocks.MockMDBHealthClient) {
				health.EXPECT().GetHostsHealth(gomock.Any(), []string{fqdn}).Return(
					[]healthtypes.HostHealth{healthtypes.NewHostHealth(
						"asd",
						fqdn,
						[]healthtypes.ServiceHealth{
							healthtypes.NewServiceHealth(
								"qwerty",
								time.Now(),
								healthtypes.ServiceStatusAlive,
								healthtypes.ServiceRoleMaster,
								healthtypes.ServiceReplicaTypeUnknown,
								"",
								0,
								nil,
							),
						},
					)},
					nil,
				)

				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), []string{fqdn}).Return(
					map[string]healthtypes.HostNeighboursInfo{
						fqdn: {
							HACluster:      true,
							HAShard:        false,
							SameRolesTotal: 2,
						},
					}, nil)
			},
			isCompute:      false,
			finishWorkflow: false,
		},
		{
			name: "master, continue, finish doesn't matter",
			expected: []steps.RunResult{
				{
					IsDone:      true,
					Description: "host is master for service \"qwerty\"",
				},
				{
					IsDone:      true,
					Description: "host is master for service \"qwerty\"",
				},
			},
			prepareFactory: func(health *mocks.MockMDBHealthClient) {
				health.EXPECT().GetHostsHealth(gomock.Any(), []string{fqdn}).Return(
					[]healthtypes.HostHealth{healthtypes.NewHostHealth(
						"asd",
						fqdn,
						[]healthtypes.ServiceHealth{
							healthtypes.NewServiceHealth(
								"qwerty",
								time.Now(),
								healthtypes.ServiceStatusAlive,
								healthtypes.ServiceRoleMaster,
								healthtypes.ServiceReplicaTypeUnknown,
								"",
								0,
								nil,
							),
						},
					)},
					nil,
				)

				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), []string{fqdn}).Return(
					map[string]healthtypes.HostNeighboursInfo{
						fqdn: {
							HACluster:      true,
							HAShard:        false,
							SameRolesTotal: 2,
						},
					}, nil)
			},
			isCompute:      false,
			finishWorkflow: true,
		},
		{
			name: "master in compute, wait",
			expected: []steps.RunResult{
				{
					IsDone: false,
					Description: "Host is master for service \"qwerty\" and we are running in compute environment. " +
						"MDB-16289 is not yet supported for this service, so you (the DUTY) have to notify customers by contacting support, see example ticket YCLOUD-4346.",
				},
				{
					IsDone: false,
					Description: "Host is master for service \"qwerty\" and we are running in compute environment. " +
						"MDB-16289 is not yet supported for this service, so you (the DUTY) have to notify customers by contacting support, see example ticket YCLOUD-4346.",
				},
			},
			prepareFactory: func(health *mocks.MockMDBHealthClient) {
				health.EXPECT().GetHostsHealth(gomock.Any(), []string{fqdn}).Return(
					[]healthtypes.HostHealth{healthtypes.NewHostHealth(
						"asd",
						fqdn,
						[]healthtypes.ServiceHealth{
							healthtypes.NewServiceHealth(
								"qwerty",
								time.Now(),
								healthtypes.ServiceStatusAlive,
								healthtypes.ServiceRoleMaster,
								healthtypes.ServiceReplicaTypeUnknown,
								"",
								0,
								nil,
							),
						},
					)},
					nil,
				).Times(2)

				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), []string{fqdn}).Return(
					map[string]healthtypes.HostNeighboursInfo{
						fqdn: {
							HACluster:      true,
							HAShard:        false,
							SameRolesTotal: 2,
						},
					}, nil).Times(2)
			},
			isCompute:      true,
			finishWorkflow: false,
		},
		{
			name: "master in compute, wait, finish doesn't matter",
			expected: []steps.RunResult{
				{
					IsDone: false,
					Description: "Host is master for service \"qwerty\" and we are running in compute environment. " +
						"MDB-16289 is not yet supported for this service, so you (the DUTY) have to notify customers by contacting support, see example ticket YCLOUD-4346.",
				},
				{
					IsDone: false,
					Description: "Host is master for service \"qwerty\" and we are running in compute environment. " +
						"MDB-16289 is not yet supported for this service, so you (the DUTY) have to notify customers by contacting support, see example ticket YCLOUD-4346.",
				},
			},
			prepareFactory: func(health *mocks.MockMDBHealthClient) {
				health.EXPECT().GetHostsHealth(gomock.Any(), []string{fqdn}).Return(
					[]healthtypes.HostHealth{healthtypes.NewHostHealth(
						"asd",
						fqdn,
						[]healthtypes.ServiceHealth{
							healthtypes.NewServiceHealth(
								"qwerty",
								time.Now(),
								healthtypes.ServiceStatusAlive,
								healthtypes.ServiceRoleMaster,
								healthtypes.ServiceReplicaTypeUnknown,
								"",
								0,
								nil,
							),
						},
					)},
					nil,
				).Times(2)

				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), []string{fqdn}).Return(
					map[string]healthtypes.HostNeighboursInfo{
						fqdn: {
							HACluster:      true,
							HAShard:        false,
							SameRolesTotal: 2,
						},
					}, nil).Times(2)
			},
			isCompute:      true,
			finishWorkflow: true,
		},
		{
			name: "not HA, continue",
			expected: []steps.RunResult{
				{
					IsDone:      true,
					Description: "is not HA",
				},
				{
					IsDone:      true,
					Description: "is not HA",
				},
			},
			prepareFactory: func(health *mocks.MockMDBHealthClient) {
				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), []string{fqdn}).Return(
					map[string]healthtypes.HostNeighboursInfo{
						fqdn: {
							SameRolesTotal: 0,
						},
					}, nil)
			},
			finishWorkflow: false,
		},
		{
			name: "not HA, finish",
			expected: []steps.RunResult{
				{
					IsDone:         true,
					Description:    "is not HA",
					FinishWorkflow: true,
				},
				{
					IsDone:         true,
					Description:    "is not HA",
					FinishWorkflow: true,
				},
			},
			prepareFactory: func(health *mocks.MockMDBHealthClient) {
				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), []string{fqdn}).Return(
					map[string]healthtypes.HostNeighboursInfo{
						fqdn: {
							SameRolesTotal: 0,
						},
					}, nil)
			},
			finishWorkflow: true,
		},
		{
			name: "not master, continue",
			expected: []steps.RunResult{
				{
					IsDone:      true,
					Description: "host is not master",
				},
				{
					IsDone:      true,
					Description: "host is not master",
				},
			},
			prepareFactory: func(health *mocks.MockMDBHealthClient) {
				health.EXPECT().GetHostsHealth(gomock.Any(), []string{fqdn}).Return(
					[]healthtypes.HostHealth{healthtypes.NewHostHealth(
						"asd",
						fqdn,
						[]healthtypes.ServiceHealth{
							healthtypes.NewServiceHealth(
								"qwerty",
								time.Now(),
								healthtypes.ServiceStatusAlive,
								healthtypes.ServiceRoleReplica,
								healthtypes.ServiceReplicaTypeUnknown,
								"",
								0,
								nil,
							),
						},
					)},
					nil,
				)

				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), []string{fqdn}).Return(
					map[string]healthtypes.HostNeighboursInfo{
						fqdn: {
							HACluster:      true,
							HAShard:        false,
							SameRolesTotal: 2,
						},
					}, nil)
			},
			finishWorkflow: false,
		},
		{
			name: "not master, finish",
			expected: []steps.RunResult{
				{
					IsDone:         true,
					Description:    "host is not master",
					FinishWorkflow: true,
				},
				{
					IsDone:         true,
					Description:    "host is not master",
					FinishWorkflow: true,
				},
			},
			prepareFactory: func(health *mocks.MockMDBHealthClient) {
				health.EXPECT().GetHostsHealth(gomock.Any(), []string{fqdn}).Return(
					[]healthtypes.HostHealth{healthtypes.NewHostHealth(
						"asd",
						fqdn,
						[]healthtypes.ServiceHealth{
							healthtypes.NewServiceHealth(
								"qwerty",
								time.Now(),
								healthtypes.ServiceStatusAlive,
								healthtypes.ServiceRoleReplica,
								healthtypes.ServiceReplicaTypeUnknown,
								"",
								0,
								nil,
							),
						},
					)},
					nil,
				)

				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), []string{fqdn}).Return(
					map[string]healthtypes.HostNeighboursInfo{
						fqdn: {
							HACluster:      true,
							HAShard:        false,
							SameRolesTotal: 2,
						},
					}, nil)
			},
			finishWorkflow: true,
		},
		{
			name: "unmanaged host",
			expected: []steps.RunResult{
				{
					IsDone:      true,
					Description: "health knows nothing about host",
				},
				{
					IsDone:      true,
					Description: "health knows nothing about host",
				},
			},
			prepareFactory: func(health *mocks.MockMDBHealthClient) {
				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), []string{fqdn}).Return(nil, nil).Times(2)
			},
			finishWorkflow: true,
		},
		{
			name: "master in compute, wait, than not master, continue",
			expected: []steps.RunResult{
				{
					IsDone: false,
					Description: "Host is master for service \"qwerty\" and we are running in compute environment. " +
						"MDB-16289 is not yet supported for this service, so you (the DUTY) have to notify customers by contacting support, see example ticket YCLOUD-4346.",
				},
				{
					IsDone: false,
					Description: "Host is master for service \"qwerty\" and we are running in compute environment. " +
						"MDB-16289 is not yet supported for this service, so you (the DUTY) have to notify customers by contacting support, see example ticket YCLOUD-4346.",
				},
				{
					IsDone:      true,
					Description: "host is not master",
				},
			},
			prepareFactory: func(health *mocks.MockMDBHealthClient) {
				health.EXPECT().GetHostsHealth(gomock.Any(), []string{fqdn}).Return(
					[]healthtypes.HostHealth{healthtypes.NewHostHealth(
						"asd",
						fqdn,
						[]healthtypes.ServiceHealth{
							healthtypes.NewServiceHealth(
								"qwerty",
								time.Now(),
								healthtypes.ServiceStatusAlive,
								healthtypes.ServiceRoleMaster,
								healthtypes.ServiceReplicaTypeUnknown,
								"",
								0,
								nil,
							),
						},
					)},
					nil,
				).Times(2)
				health.EXPECT().GetHostsHealth(gomock.Any(), []string{fqdn}).Return(
					[]healthtypes.HostHealth{healthtypes.NewHostHealth(
						"asd",
						fqdn,
						[]healthtypes.ServiceHealth{
							healthtypes.NewServiceHealth(
								"qwerty",
								time.Now(),
								healthtypes.ServiceStatusAlive,
								healthtypes.ServiceRoleReplica,
								healthtypes.ServiceReplicaTypeUnknown,
								"",
								0,
								nil,
							),
						},
					)},
					nil,
				)

				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), []string{fqdn}).Return(
					map[string]healthtypes.HostNeighboursInfo{
						fqdn: {
							HACluster:      true,
							HAShard:        false,
							SameRolesTotal: 2,
						},
					}, nil).Times(3)
			},
			isCompute: true,
		},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			health := mocks.NewMockMDBHealthClient(ctrl)
			tc.prepareFactory(health)

			step := steps.NewCheckIsMaster(tc.isCompute, health, tc.finishWorkflow, mwswitch.EnabledMWConfig{})
			opCtx := opcontext.NewStepContext(models.ManagementInstanceOperation{
				ID:         "qwe",
				InstanceID: fqdn,
				State:      models.DefaultOperationState(),
			}).SetFQDN(fqdn)

			for _, expected := range tc.expected {
				testStep(t, ctx, opCtx, step, expected)
			}
		})
	}
}
