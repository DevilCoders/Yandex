package steps_test

import (
	"context"
	"strings"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	mocks3 "a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	types2 "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	cms_models "a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	mocks2 "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

type HealthTestCaseExp struct {
	msg string
	act steps.AfterStepAction
}

type HealthTestCaseIn struct {
	clusters      []cms_models.Instance
	healthInfo    map[string]types.HostNeighboursInfo
	jugglerHealth healthiness.HealthCheckResult
}

type HealthTestCase struct {
	name string
	in   HealthTestCaseIn
	exp  HealthTestCaseExp
}

func TestWaitForHealthy(t *testing.T) {
	spaceLimit := 1230200329
	ctx := context.Background()
	thisDom0 := "this-dom0"
	var tcs = []HealthTestCase{
		{in: HealthTestCaseIn{
			clusters: []cms_models.Instance{{
				DBMClusterName: "this-cluster",
				FQDN:           "this-fqdn",
			}},
			healthInfo: map[string]types.HostNeighboursInfo{},
			jugglerHealth: healthiness.HealthCheckResult{
				Unknown: []healthiness.FQDNCheck{{
					Instance: cms_models.Instance{
						DBMClusterName: "this-cluster",
						FQDN:           "this-fqdn",
					},
					HintIs: "just because",
				}},
			},
		}, exp: HealthTestCaseExp{
			msg: strings.Join([]string{
				"knows nothing about 1:",
				"'this-fqdn' of 'this-cluster', reason: \"just because\"",
			}, "\n"),
			act: steps.AfterStepEscalate,
		}, name: "escalate when unknown in juggler and health"},
		{in: HealthTestCaseIn{
			clusters: []cms_models.Instance{{
				DBMClusterName: "this-cluster",
				FQDN:           "this-fqdn",
			}},
			healthInfo: map[string]types.HostNeighboursInfo{},
			jugglerHealth: healthiness.HealthCheckResult{
				GiveAway: []healthiness.FQDNCheck{{
					Instance: cms_models.Instance{
						DBMClusterName: "this-cluster",
						FQDN:           "this-fqdn",
					},
				}},
			},
		}, exp: HealthTestCaseExp{
			msg: strings.Join([]string{
				"1 ok",
			}, "\n"),
			act: steps.AfterStepContinue,
		}, name: "unknown in health, but known in juggler"},
		{in: HealthTestCaseIn{
			clusters: []cms_models.Instance{{
				DBMClusterName: "this-cluster",
				FQDN:           "this-fqdn",
				Volumes:        []dbm.Volume{{SpaceLimit: spaceLimit}},
			}},
			healthInfo: map[string]types.HostNeighboursInfo{
				"this-fqdn": {
					Cid:         "this-cid",
					Sid:         "this-sid",
					Env:         "this-env",
					SameRolesTS: time.Now().Add(-20 * time.Minute),
					HACluster:   true,
				}},
		}, exp: HealthTestCaseExp{
			msg: strings.Join([]string{
				"outdated info about 1:",
				"stale for 20m0s 'this-fqdn' of 'this-cluster' HA: true (cluster), roles: UNKNOWN, giving away this node will leave 0 healthy nodes of 1 total, space limit 1.2 GB",
			}, "\n"),
			act: steps.AfterStepWait,
		}, name: "wait when stale"},
		{in: HealthTestCaseIn{
			clusters: []cms_models.Instance{{
				DBMClusterName: "this-cluster",
				FQDN:           "this-fqdn",
			}},
			healthInfo: map[string]types.HostNeighboursInfo{
				"this-fqdn": {
					Cid:            "this-cid",
					Sid:            "this-sid",
					Env:            "qa",
					SameRolesTS:    time.Now().Add(-1 * time.Second),
					SameRolesTotal: 2,
					SameRolesAlive: 1,
					Roles:          []string{"role-name"},
					HACluster:      true,
					HAShard:        true,
				}},
		}, exp: HealthTestCaseExp{
			msg: strings.Join([]string{
				"1 ok",
			}, "\n"),
			act: steps.AfterStepContinue,
		}, name: "ok for non prod"},
		{in: HealthTestCaseIn{
			clusters: []cms_models.Instance{{
				DBMClusterName: "this-cluster",
				FQDN:           "this-fqdn",
				Volumes:        []dbm.Volume{{SpaceLimit: spaceLimit}},
			}},
			healthInfo: map[string]types.HostNeighboursInfo{
				"this-fqdn": {
					Cid:            "this-cid",
					Sid:            "this-sid",
					Env:            "prod",
					SameRolesTS:    time.Now().Add(-1 * time.Second),
					SameRolesTotal: 2,
					SameRolesAlive: 1,
					Roles:          []string{"role-name"},
					HACluster:      true,
					HAShard:        true,
				}},
		}, exp: HealthTestCaseExp{
			msg: strings.Join([]string{
				"1 would degrade now:",
				"'this-fqdn' of 'this-cluster' HA: true (cluster+shard), roles: role-name, giving away this node will leave 1 healthy nodes of 3 total, space limit 1.2 GB",
			}, "\n"),
			act: steps.AfterStepEscalate,
		}, name: "escalate when same roles not enough"},
		{in: HealthTestCaseIn{
			clusters: []cms_models.Instance{{
				DBMClusterName: "this-cluster",
				FQDN:           "this-fqdn",
			}},
			healthInfo: map[string]types.HostNeighboursInfo{
				"this-fqdn": {
					Cid:            "this-cid",
					Sid:            "this-sid",
					Env:            "prod",
					SameRolesTS:    time.Now().Add(-1 * time.Second),
					SameRolesTotal: 1,
					SameRolesAlive: 1,
					Roles:          []string{"role-name"},
					HACluster:      false,
					HAShard:        false,
				}},
		}, exp: HealthTestCaseExp{
			msg: strings.Join([]string{
				"1 ok",
			}, "\n"),
			act: steps.AfterStepContinue,
		}, name: "ok when HA is not satisfied"},
		{in: HealthTestCaseIn{
			clusters: []cms_models.Instance{{
				DBMClusterName: "this-cluster",
				FQDN:           "this-fqdn",
			}},
			healthInfo: map[string]types.HostNeighboursInfo{
				"this-fqdn": {
					Cid:            "this-cid",
					Sid:            "this-sid",
					Env:            "prod",
					SameRolesTS:    time.Unix(-62135596800, 0),
					SameRolesTotal: 1,
					SameRolesAlive: 1,
					Roles:          []string{"role-name"},
					HACluster:      false,
					HAShard:        false,
				}},
		}, exp: HealthTestCaseExp{
			msg: strings.Join([]string{
				"1 ok",
			}, "\n"),
			act: steps.AfterStepContinue,
		}, name: "do not wait if not HA and invalid sameRoleTS"},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			dom0d := mocks3.NewMockDom0Discovery(ctrl)
			health := mocks2.NewMockMDBHealthClient(ctrl)
			jglr := mocks.NewMockHealthiness(ctrl)
			jglr.EXPECT().ByInstances(gomock.Any(), gomock.Any()).Return(tc.in.jugglerHealth, nil).AnyTimes()

			step := steps.NewWaitAllHealthyStep(dom0d, health, jglr).(*steps.WaitAllHealthyStep)

			dom0d.EXPECT().Dom0Instances(gomock.Any(), gomock.Any()).Times(1).Return(dom0discovery.DiscoveryResult{
				WellKnown: tc.in.clusters,
			}, nil)
			health.EXPECT().GetHostNeighboursInfo(gomock.Any(), gomock.Any()).Return(tc.in.healthInfo, nil)

			insCtx := steps.NewEmptyInstructionCtx()
			insCtx.SetActualRD(&types2.RequestDecisionTuple{
				R: models.ManagementRequest{Fqnds: []string{thisDom0}},
			})

			runRes := step.RunStep(ctx, &insCtx)

			require.NotNil(t, runRes)
			require.Equal(t, tc.exp.act, runRes.Action)
			require.Equal(t, tc.exp.msg, runRes.ForHuman)
			require.NoError(t, runRes.Error)
		})
	}
}

func Test_overrideResult(t *testing.T) {
	type args struct {
		source    healthiness.HealthCheckResult
		overrides healthiness.HealthCheckResult
	}
	iExisting := healthiness.FQDNCheck{Instance: cms_models.Instance{FQDN: "test-exists"}}
	iNewFound := healthiness.FQDNCheck{Instance: cms_models.Instance{FQDN: "test-found"}}
	tests := []struct {
		name string
		args args
		want healthiness.HealthCheckResult
	}{
		{
			name: "Stale",
			args: args{
				source: healthiness.HealthCheckResult{
					Stale: []healthiness.FQDNCheck{iExisting},
				},
				overrides: healthiness.HealthCheckResult{
					Stale: []healthiness.FQDNCheck{iNewFound, iExisting},
				},
			},
			want: healthiness.HealthCheckResult{
				Stale: []healthiness.FQDNCheck{iExisting, iNewFound},
			},
		},
		{
			name: "GiveAway",
			args: args{
				source: healthiness.HealthCheckResult{
					GiveAway: []healthiness.FQDNCheck{iExisting},
				},
				overrides: healthiness.HealthCheckResult{
					GiveAway: []healthiness.FQDNCheck{iNewFound, iExisting},
				},
			},
			want: healthiness.HealthCheckResult{
				GiveAway: []healthiness.FQDNCheck{iExisting, iNewFound},
			},
		},
		{
			name: "Ignored",
			args: args{
				source: healthiness.HealthCheckResult{
					Ignored: []healthiness.FQDNCheck{iExisting},
				},
				overrides: healthiness.HealthCheckResult{
					Ignored: []healthiness.FQDNCheck{iNewFound, iExisting},
				},
			},
			want: healthiness.HealthCheckResult{
				Ignored: []healthiness.FQDNCheck{iExisting, iNewFound},
			},
		},
		{
			name: "WouldDegrade",
			args: args{
				source: healthiness.HealthCheckResult{
					WouldDegrade: []healthiness.FQDNCheck{iExisting},
				},
				overrides: healthiness.HealthCheckResult{
					WouldDegrade: []healthiness.FQDNCheck{iNewFound, iExisting},
				},
			},
			want: healthiness.HealthCheckResult{
				WouldDegrade: []healthiness.FQDNCheck{iExisting, iNewFound},
			},
		},
		{
			name: "Unknown",
			args: args{
				source: healthiness.HealthCheckResult{
					Unknown: []healthiness.FQDNCheck{iExisting},
				},
				overrides: healthiness.HealthCheckResult{
					Unknown: []healthiness.FQDNCheck{iNewFound},
				},
			},
			want: healthiness.HealthCheckResult{
				Unknown: []healthiness.FQDNCheck{iNewFound},
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got := steps.OverrideResult(tt.args.source, tt.args.overrides)
			require.Equal(t, tt.want, got)
		})
	}
}
