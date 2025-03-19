package steps_test

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	metadbmocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/testutil"
	healthmock "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

func TestWaitForHealthyStep(t *testing.T) {
	fqdn := "fqdn1"
	clusterID := "cid1"
	type testCase struct {
		name     string
		ni       map[string]types.HostNeighboursInfo
		expected []steps.RunResult
		status   metadb.ClusterStatus

		healthQueryCount int
		metadbQueryCount int
	}

	testCases := []testCase{
		{
			name: "ok",
			ni: map[string]types.HostNeighboursInfo{fqdn: {
				SameRolesTS: time.Now(),
			}},
			expected: []steps.RunResult{
				{
					Description: "all neighbours are healthy",
					IsDone:      true,
				},
				{
					Description: "health has been checked already",
					IsDone:      true,
				},
			},
			status:           metadb.ClusterStatusRunning,
			healthQueryCount: 1,
			metadbQueryCount: 1,
		},
		{
			name: "stale",
			ni: map[string]types.HostNeighboursInfo{fqdn: {
				SameRolesTS: time.Now().Add(-time.Hour),
				HACluster:   true,
			}},
			expected: []steps.RunResult{
				{
					Description: "outdated info about:\nstale for 1h0m0s HA: true (cluster), roles: UNKNOWN, giving away this node will leave 0 healthy nodes of 1 total, space limit 0 B",
					IsDone:      false,
				},
				{
					Description: "outdated info about:\nstale for 1h0m0s HA: true (cluster), roles: UNKNOWN, giving away this node will leave 0 healthy nodes of 1 total, space limit 0 B",
					IsDone:      false,
				},
			},
			status:           metadb.ClusterStatusRunning,
			healthQueryCount: 2,
			metadbQueryCount: 2,
		},
		{
			name: "degraded",
			ni: map[string]types.HostNeighboursInfo{fqdn: {
				Env:            "prod",
				Roles:          []string{"test role"},
				HACluster:      true,
				SameRolesTotal: 2,
				SameRolesAlive: 1,
				SameRolesTS:    time.Now(),
			}},
			expected: []steps.RunResult{
				{
					Description: "would degrade now:\nHA: true (cluster), roles: test role, giving away this node will leave 1 healthy nodes of 3 total, space limit 0 B",
					IsDone:      false,
				},
				{
					Description: "would degrade now:\nHA: true (cluster), roles: test role, giving away this node will leave 1 healthy nodes of 3 total, space limit 0 B",
					IsDone:      false,
				},
			},
			status:           metadb.ClusterStatusRunning,
			healthQueryCount: 2,
			metadbQueryCount: 2,
		},
		{
			name: "ignore unknown hosts",
			ni:   map[string]types.HostNeighboursInfo{},
			expected: []steps.RunResult{
				{
					Description: "health knows nothing about fqdn1",
					IsDone:      true,
				},
				{
					Description: "health has been checked already",
					IsDone:      true,
				},
			},
			status:           metadb.ClusterStatusRunning,
			healthQueryCount: 1,
			metadbQueryCount: 1,
		},
		{
			name: "stopped cluster",
			ni:   map[string]types.HostNeighboursInfo{},
			expected: []steps.RunResult{
				{
					Description: "cluster is STOPPED, do not need to check health",
					IsDone:      true,
				},
				{
					Description: "health has been checked already",
					IsDone:      true,
				},
			},
			status:           metadb.ClusterStatusStopped,
			healthQueryCount: 0,
			metadbQueryCount: 1,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			health := healthmock.NewMockMDBHealthClient(ctrl)
			health.EXPECT().GetHostNeighboursInfo(gomock.Any(), gomock.Any()).Return(tc.ni, nil).Times(tc.healthQueryCount)

			meta := metadbmocks.NewMockMetaDB(ctrl)
			txCtx, matcher := testutil.MatchContext(t, ctx)
			meta.EXPECT().Begin(gomock.Any(), sqlutil.Alive).Return(txCtx, nil).Times(tc.metadbQueryCount)
			meta.EXPECT().GetHostByFQDN(matcher, fqdn).Return(metadb.Host{ClusterID: clusterID}, nil).Times(tc.metadbQueryCount)
			meta.EXPECT().ClusterInfo(matcher, clusterID).Return(metadb.ClusterInfo{Status: tc.status}, nil).Times(tc.metadbQueryCount)
			meta.EXPECT().Rollback(matcher).Return(nil).Times(tc.metadbQueryCount)

			step := steps.NewWaitForHealthy(health, meta)
			stepCtx := opcontext.NewStepContext(models.ManagementInstanceOperation{
				ID:         "qwe",
				InstanceID: fqdn,
				State:      models.DefaultOperationState(),
			}).SetFQDN(fqdn)

			for _, expected := range tc.expected {
				testStep(t, ctx, stepCtx, step, expected)
			}
		})
	}
}
