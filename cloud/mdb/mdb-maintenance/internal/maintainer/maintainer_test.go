package maintainer

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	resmanagermock "a.yandex-team.ru/cloud/mdb/internal/compute/resmanager/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/holidays/workaholic"
	metadbmock "a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/stat"
	"a.yandex-team.ru/library/go/core/log/nop"
	metricsnop "a.yandex-team.ru/library/go/core/metrics/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestCollectUserFromCloud(t *testing.T) {
	ctrl := gomock.NewController(t)
	metaDB := metadbmock.NewMockMetaDB(ctrl)
	mStat := stat.MaintenanceStat{
		ConfigCount:                metricsnop.Counter{},
		ConfigErrorCount:           metricsnop.Counter{},
		MaintenanceDuration:        metricsnop.Timer{},
		ConfigPlanClusterCount:     metricsnop.CounterVec{},
		ConfigSelectedClusterCount: metricsnop.CounterVec{},
	}
	resClient := resmanagermock.NewMockClient(ctrl)
	cfg := DefaultConfig()
	m := New(nil, &nop.Logger{}, metaDB, cfg, mStat, nil, resClient, &workaholic.Calendar{})

	testCases := []struct {
		bindings      []resmanager.AccessBinding
		bindingErr    error
		collectFailed bool
		users         []string
	}{
		{[]resmanager.AccessBinding{
			{RoleID: "mdb.admin", Subject: resmanager.Subject{ID: "usr1", Type: "userAccount"}},
		}, nil, false, []string{"usr1"}},
		{[]resmanager.AccessBinding{
			{RoleID: "mdb.viewer", Subject: resmanager.Subject{ID: "usr1", Type: "userAccount"}},
		}, nil, false, nil},
		{[]resmanager.AccessBinding{
			{RoleID: "mdb.admin", Subject: resmanager.Subject{ID: "usr1", Type: "serviceAccount"}},
		}, nil, false, nil},
		{[]resmanager.AccessBinding{
			{RoleID: "mdb.admin", Subject: resmanager.Subject{ID: "usr1", Type: "serviceAccount"}},
			{RoleID: "mdb.viewer", Subject: resmanager.Subject{ID: "usr2", Type: "serviceAccount"}},
			{RoleID: "mdb.admin", Subject: resmanager.Subject{ID: "usr3", Type: "userAccount"}},
		}, nil, false, []string{"usr3"}},
		{nil, xerrors.Errorf("unknown"), true, nil},
	}

	for i, tc := range testCases {
		cid := fmt.Sprintf("cid%d", i)
		t.Run(cid, func(t *testing.T) {
			resClient.EXPECT().ListAccessBindings(gomock.Any(), cid, true).Return(tc.bindings, tc.bindingErr)
			users, err := m.collectUserFromCloud(context.Background(), cid)
			if tc.collectFailed {
				require.Error(t, err)
			} else {
				require.NoError(t, err)
			}
			require.Equal(t, tc.users, users)
		})
	}
}

func TestMaintainer_toRestore(t *testing.T) {
	type testCase struct {
		name         string
		minDays      int
		maxDelayDays int
		expectDays   int
	}
	tests := []testCase{
		{
			name:         "replan rejected task considers min days and max days",
			minDays:      3,
			maxDelayDays: 2,
			expectDays:   2,
		},
		{
			name:         "replan rejected task considers min days",
			minDays:      3,
			maxDelayDays: 3,
			expectDays:   3,
		},
		{
			name:         "replan rejected task considers min days when max days is a lot",
			minDays:      3,
			maxDelayDays: 4,
			expectDays:   3,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			now, err := time.Parse(time.RubyDate, "Mon Oct 18 15:04:05 +0300 2021")
			require.NoError(t, err, "invalid now date")
			m := &Maintainer{
				L: &nop.Logger{},
				now: func() time.Time {
					return now
				},
				Cal: &workaholic.Calendar{},
			}
			clusters := []models.Cluster{{
				ID:       "test",
				Settings: models.MaintenanceSettings{}, // empty - any day
			}}
			existedTasksByClusterID := map[string]models.MaintenanceTask{
				"test": {
					ClusterID: "test",
					Status:    models.MaintenanceTaskRejected,
					MaxDelay:  now.Add(time.Duration(tt.maxDelayDays) * 24 * time.Hour),
				},
			}

			actions, err := m.toRestore(context.Background(), clusters, existedTasksByClusterID, tt.minDays, 0)

			assert.NoError(t, err)
			require.Len(t, actions, 1, "should contain one action")
			require.Equal(t, actionRetry, actions[0].typ, "rejected task should be retried")
			require.WithinDuration(t, now.Add(24*time.Hour*time.Duration(tt.expectDays)), actions[0].planTS, time.Hour, "should replan to expected time")
		})
	}

}

func TestMaintainer_limitToCreate(t *testing.T) {
	type args struct {
		toCreate           []MaintainAction
		newlyPlannedLimit  uint64
		newlyPlannedClouds int
	}
	tests := []struct {
		name string
		args args
		want map[string]int //map[cloudID]length_of_actions
	}{
		{
			name: "1 action, limit 1",
			args: args{
				toCreate: []MaintainAction{
					{cluster: models.Cluster{CloudExtID: "cloud1", ID: "cluster1"}},
				},
				newlyPlannedLimit:  1,
				newlyPlannedClouds: 1,
			},
			want: map[string]int{
				"cloud1": 1,
			},
		},
		{
			name: "1 cloud, 2 actions, limit 1",
			args: args{
				toCreate: []MaintainAction{
					{cluster: models.Cluster{CloudExtID: "cloud1", ID: "cluster1"}},
					{cluster: models.Cluster{CloudExtID: "cloud1", ID: "cluster2"}},
				},
				newlyPlannedLimit:  1,
				newlyPlannedClouds: 1,
			},
			want: map[string]int{
				"cloud1": 2,
			},
		},
		{
			name: "2 clouds, cloud limit 1, actions unlimited",
			args: args{
				toCreate: []MaintainAction{
					{cluster: models.Cluster{CloudExtID: "cloud1", ID: "cluster1"}},
					{cluster: models.Cluster{CloudExtID: "cloud2", ID: "cluster2"}},
				},
				newlyPlannedLimit:  1000,
				newlyPlannedClouds: 1,
			},
			want: map[string]int{
				"cloud1": 1, // should be always first, because we want to process clouds deterministically – in order
			},
		},
		{
			name: "2 clouds, no cloud limit, actions limited",
			args: args{
				toCreate: []MaintainAction{
					{cluster: models.Cluster{CloudExtID: "cloud1", ID: "cluster1"}},
					{cluster: models.Cluster{CloudExtID: "cloud2", ID: "cluster2"}},
				},
				newlyPlannedLimit:  1,
				newlyPlannedClouds: 2,
			},
			want: map[string]int{
				"cloud1": 1,
			},
		},
		{
			name: "2 clouds, no cloud limit, actions limited",
			args: args{
				toCreate: []MaintainAction{
					{cluster: models.Cluster{CloudExtID: "cloud1", ID: "cluster1"}},
					{cluster: models.Cluster{CloudExtID: "cloud2", ID: "cluster2"}},
					{cluster: models.Cluster{CloudExtID: "cloud2", ID: "cluster3"}},
					{cluster: models.Cluster{CloudExtID: "cloud3", ID: "cluster4"}},
				},
				newlyPlannedLimit:  2,
				newlyPlannedClouds: 2,
			},
			want: map[string]int{
				"cloud1": 1,
				"cloud2": 2,
			},
		},
		{
			name: "2 clouds, no cloud limit, actions unlimited",
			args: args{
				toCreate: []MaintainAction{
					{cluster: models.Cluster{CloudExtID: "cloud1", ID: "cluster1"}},
					{cluster: models.Cluster{CloudExtID: "cloud2", ID: "cluster2"}},
					{cluster: models.Cluster{CloudExtID: "cloud2", ID: "cluster3"}},
				},
				newlyPlannedLimit:  1000,
				newlyPlannedClouds: 1000,
			},
			want: map[string]int{
				"cloud1": 1,
				"cloud2": 2,
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			m := &Maintainer{
				L: &nop.Logger{},
				Config: Config{
					NewlyPlannedLimit:  tt.args.newlyPlannedLimit,
					NewlyPlannedClouds: tt.args.newlyPlannedClouds,
				},
			}
			got := m.limitToCreate(context.Background(), tt.args.toCreate)
			require.Len(t, got, len(tt.want))
			for cloudID, wantLength := range tt.want {
				maintainActions, ok := got[cloudID]
				assert.True(t, ok, "expected actions for cloud %s, but there are none", cloudID)
				assert.Len(t, maintainActions, wantLength, "expected %d actions for cloud %s, but got %d", wantLength, cloudID, len(maintainActions))
			}
		})
	}
}

func Test_filterByEnv(t *testing.T) {
	type args struct {
		clusters []models.Cluster
	}
	tests := []struct {
		name string
		args args
		want []models.Cluster
	}{
		{
			name: "one cluster",
			args: args{
				clusters: []models.Cluster{
					{ID: "1", Env: models.EnvDev, CloudExtID: "cloud1"},
				},
			},
			want: []models.Cluster{
				{ID: "1", Env: models.EnvDev, CloudExtID: "cloud1"},
			},
		},
		{
			name: "one cloud, two envs",
			args: args{
				clusters: []models.Cluster{
					{ID: "1", Env: models.EnvDev, CloudExtID: "cloud1"},
					{ID: "2", Env: models.EnvQA, CloudExtID: "cloud1"},
				},
			},
			want: []models.Cluster{
				{ID: "1", Env: models.EnvDev, CloudExtID: "cloud1"},
			},
		},
		{
			name: "one cloud, same envs",
			args: args{
				clusters: []models.Cluster{
					{ID: "1", Env: models.EnvDev, CloudExtID: "cloud1"},
					{ID: "2", Env: models.EnvDev, CloudExtID: "cloud1"},
				},
			},
			want: []models.Cluster{
				{ID: "1", Env: models.EnvDev, CloudExtID: "cloud1"},
				{ID: "2", Env: models.EnvDev, CloudExtID: "cloud1"},
			},
		},
		{
			name: "different clouds, different envs",
			args: args{
				clusters: []models.Cluster{
					{ID: "1", Env: models.EnvProd, CloudExtID: "cloud1"},
					{ID: "2", Env: models.EnvQA, CloudExtID: "cloud2"},
					{ID: "3", Env: models.EnvDev, CloudExtID: "cloud1"},
					{ID: "4", Env: models.EnvComputeProd, CloudExtID: "cloud2"},
					{ID: "5", Env: models.EnvDev, CloudExtID: "cloud1"},
				},
			},
			want: []models.Cluster{
				{ID: "3", Env: models.EnvDev, CloudExtID: "cloud1"},
				{ID: "2", Env: models.EnvQA, CloudExtID: "cloud2"},
				{ID: "5", Env: models.EnvDev, CloudExtID: "cloud1"},
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			m := &Maintainer{L: &nop.Logger{}}
			got := m.filterByEnv(context.Background(), tt.args.clusters)
			wantMap := make(map[string]models.Cluster, len(tt.want)) //we need map, because we don't care about the order
			for _, cluster := range tt.want {
				wantMap[cluster.ID] = cluster
			}
			require.Len(t, got, len(wantMap), "incorrect number of clusters")
			for _, cluster := range got {
				assert.Equal(t, wantMap[cluster.ID].ID, cluster.ID)
				assert.Equal(t, wantMap[cluster.ID].CloudExtID, cluster.CloudExtID)
				assert.Equal(t, wantMap[cluster.ID].Env, cluster.Env)
			}
		})
	}
}
