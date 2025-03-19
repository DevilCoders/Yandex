package core

import (
	"context"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	lemocks "a.yandex-team.ru/cloud/mdb/internal/leaderelection/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	mdbmocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-health/internal/unhealthy"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	dsmocks "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore/mocks"
	ssmocks "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/secretsstore/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func initGW(t *testing.T, gwCfg GWConfig) (context.Context, *dsmocks.MockBackend, *ssmocks.MockBackend, *lemocks.MockLeaderElector, *GeneralWard) {
	ctx := context.Background()
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))
	ctrl := gomock.NewController(t)
	dsmock := dsmocks.NewMockBackend(ctrl)
	ssmock := ssmocks.NewMockBackend(ctrl)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	lemock := lemocks.NewMockLeaderElector(ctrl)
	gw := NewGeneralWard(ctx, logger, gwCfg, dsmock, nil, ssmock, mdb, nil, false, lemock)
	require.NotNil(t, gw)
	return ctx, dsmock, ssmock, lemock, gw
}

func generateAliveClusterHealth(env string, count int, now time.Time) []types.ClusterHealth {
	result := make([]types.ClusterHealth, count)
	for i := 0; i < count; i++ {
		result[i] = types.ClusterHealth{
			Cid:      "cid-" + uuid.Must(uuid.NewV4()).String(),
			Env:      env,
			Status:   types.ClusterStatusAlive,
			StatusTS: now,
			AliveTS:  now,
			SLA:      false,
		}
	}
	return result
}

func TestProcessSLI(t *testing.T) {
	gwCfg := DefaultConfig()
	ctx, dsmock, _, lemock, gw := initGW(t, gwCfg)
	now := time.Now()
	env := "qa"

	clHealth0 := generateAliveClusterHealth(env, 5, now)
	clHealth0[2].Status = types.ClusterStatusDead
	clHealth0[4].Status = types.ClusterStatusDegraded
	clHealth1 := generateAliveClusterHealth(env, 2, now)
	clHealth1[0].Status = types.ClusterStatusDead
	clHealth2 := generateAliveClusterHealth(env, 7, now)
	clHealth2[3].Status = types.ClusterStatusUnknown
	aggInfo := datastore.AggregatedInfo{
		SLA:       false,
		Timestamp: now,
		CType:     metadb.PostgresqlCluster,
		AggType:   types.AggClusters,
		Env:       env,
		Total:     len(clHealth0) + len(clHealth1) + len(clHealth2),
		Alive:     len(clHealth0) + len(clHealth1) + len(clHealth2) - 4,
		Degraded:  1,
		Unknown:   1,
		Dead:      2,
	}
	uAggInfo := unhealthy.UAInfo{
		RWRecs:         make(map[unhealthy.RWKey]*unhealthy.UARWRecord),
		StatusRecs:     make(map[unhealthy.StatusKey]*unhealthy.UARecord),
		WarningGeoRecs: make(map[unhealthy.GeoKey]*unhealthy.UARecord),
	}
	uAggInfo.StatusRecs[unhealthy.StatusKey{SLA: false, Env: env, Status: string(types.ClusterStatusAlive)}] =
		&unhealthy.UARecord{Count: 10}
	uAggInfo.StatusRecs[unhealthy.StatusKey{SLA: false, Env: env, Status: string(types.ClusterStatusDegraded)}] =
		&unhealthy.UARecord{Count: 1, Examples: []string{clHealth0[4].Cid}}
	uAggInfo.StatusRecs[unhealthy.StatusKey{SLA: false, Env: env, Status: string(types.ClusterStatusUnknown)}] =
		&unhealthy.UARecord{Count: 1, Examples: []string{clHealth2[3].Cid}}
	uAggInfo.StatusRecs[unhealthy.StatusKey{SLA: false, Env: env, Status: string(types.ClusterStatusDead)}] =
		&unhealthy.UARecord{Count: 2, Examples: []string{clHealth0[2].Cid, clHealth1[0].Cid}}
	dsmock.EXPECT().LoadFewClustersHealth(gomock.Any(), metadb.PostgresqlCluster, "").Return(datastore.FewClusterHealthInfo{Clusters: clHealth0, NextCursor: "5"}, nil)
	dsmock.EXPECT().LoadFewClustersHealth(gomock.Any(), metadb.PostgresqlCluster, "5").Return(datastore.FewClusterHealthInfo{Clusters: clHealth1, NextCursor: "7"}, nil)
	dsmock.EXPECT().LoadFewClustersHealth(gomock.Any(), metadb.PostgresqlCluster, "7").Return(datastore.FewClusterHealthInfo{Clusters: clHealth2, NextCursor: datastore.EndCursor}, nil)
	dsmock.EXPECT().SaveClustersHealth(gomock.Any(), clHealth0, gwCfg.ClusterHealthTimeout).Return(nil)
	dsmock.EXPECT().SaveClustersHealth(gomock.Any(), clHealth1, gwCfg.ClusterHealthTimeout).Return(nil)
	dsmock.EXPECT().SaveClustersHealth(gomock.Any(), clHealth2, gwCfg.ClusterHealthTimeout).Return(nil)
	dsmock.EXPECT().SetAggregateInfo(gomock.Any(), aggInfo, gwCfg.ClusterHealthTimeout).Return(nil)
	dsmock.EXPECT().SetUnhealthyAggregatedInfo(gomock.Any(), metadb.PostgresqlCluster, gomock.Eq(uAggInfo), gomock.Any()).Return(nil)
	lemock.EXPECT().IsLeader(gomock.Any()).Return(true)
	gw.stat = newStat(gw.ds, true)
	gw.processSLI(ctx, metadb.PostgresqlCluster, now)
}

func TestProcessSLIFewEnvs(t *testing.T) {
	gwCfg := DefaultConfig()
	ctx, dsmock, _, lemock, gw := initGW(t, gwCfg)
	now := time.Now()
	nowNextEnv := now.Add(time.Minute)
	env1 := "qa"
	env2 := "prod"

	clHealth0 := generateAliveClusterHealth(env1, 6, now)
	clHealth0[0].Status = types.ClusterStatusDead
	clHealth0[3].Status = types.ClusterStatusDegraded
	clHealth1 := generateAliveClusterHealth(env1, 1, now)
	clHealth1[0].Status = types.ClusterStatusDead
	clHealth2 := generateAliveClusterHealth(env1, 8, now)
	clHealth2[5].Status = types.ClusterStatusUnknown
	aggInfo := datastore.AggregatedInfo{
		Timestamp: nowNextEnv,
		CType:     metadb.PostgresqlCluster,
		AggType:   types.AggClusters,
		Env:       env1,
		Total:     len(clHealth0) + len(clHealth1) + len(clHealth2),
		Alive:     len(clHealth0) + len(clHealth1) + len(clHealth2) - 4,
		Degraded:  1,
		Unknown:   1,
		Dead:      2,
	}

	clHealthNextEnv0 := generateAliveClusterHealth(env2, 9, now)
	clHealthNextEnv0[1].Status = types.ClusterStatusDead
	clHealthNextEnv0[7].Status = types.ClusterStatusUnknown
	clHealthNextEnv0[8].Status = types.ClusterStatusDegraded
	clHealthNextEnv1 := generateAliveClusterHealth(env2, 4, now)
	clHealthNextEnv1[3].Status = types.ClusterStatusUnknown
	aggInfoNext := datastore.AggregatedInfo{
		Timestamp: nowNextEnv,
		CType:     metadb.PostgresqlCluster,
		AggType:   types.AggClusters,
		Env:       env2,
		Total:     len(clHealthNextEnv0) + len(clHealthNextEnv1),
		Alive:     len(clHealthNextEnv0) + len(clHealthNextEnv1) - 4,
		Degraded:  1,
		Unknown:   2,
		Dead:      1,
	}
	uAggInfo := unhealthy.UAInfo{
		RWRecs:         make(map[unhealthy.RWKey]*unhealthy.UARWRecord),
		StatusRecs:     make(map[unhealthy.StatusKey]*unhealthy.UARecord),
		WarningGeoRecs: make(map[unhealthy.GeoKey]*unhealthy.UARecord),
	}
	uAggInfo.StatusRecs[unhealthy.StatusKey{Env: env1, Status: string(types.ClusterStatusAlive)}] =
		&unhealthy.UARecord{Count: 11}
	uAggInfo.StatusRecs[unhealthy.StatusKey{Env: env1, Status: string(types.ClusterStatusDead)}] =
		&unhealthy.UARecord{Count: 2, Examples: []string{clHealth0[0].Cid, clHealth1[0].Cid}}
	uAggInfo.StatusRecs[unhealthy.StatusKey{Env: env1, Status: string(types.ClusterStatusDegraded)}] =
		&unhealthy.UARecord{Count: 1, Examples: []string{clHealth0[3].Cid}}
	uAggInfo.StatusRecs[unhealthy.StatusKey{Env: env1, Status: string(types.ClusterStatusUnknown)}] =
		&unhealthy.UARecord{Count: 1, Examples: []string{clHealth2[5].Cid}}
	uAggInfo.StatusRecs[unhealthy.StatusKey{Env: env2, Status: string(types.ClusterStatusAlive)}] =
		&unhealthy.UARecord{Count: 9}
	uAggInfo.StatusRecs[unhealthy.StatusKey{Env: env2, Status: string(types.ClusterStatusDead)}] =
		&unhealthy.UARecord{Count: 1, Examples: []string{clHealthNextEnv0[1].Cid}}
	uAggInfo.StatusRecs[unhealthy.StatusKey{Env: env2, Status: string(types.ClusterStatusUnknown)}] =
		&unhealthy.UARecord{Count: 2, Examples: []string{clHealthNextEnv0[7].Cid, clHealthNextEnv1[3].Cid}}
	uAggInfo.StatusRecs[unhealthy.StatusKey{Env: env2, Status: string(types.ClusterStatusDegraded)}] =
		&unhealthy.UARecord{Count: 1, Examples: []string{clHealthNextEnv0[8].Cid}}
	dsmock.EXPECT().LoadFewClustersHealth(gomock.Any(), metadb.PostgresqlCluster, "").Return(datastore.FewClusterHealthInfo{Clusters: clHealth0, NextCursor: "6"}, nil)
	dsmock.EXPECT().LoadFewClustersHealth(gomock.Any(), metadb.PostgresqlCluster, "6").Return(datastore.FewClusterHealthInfo{Clusters: clHealthNextEnv0, NextCursor: "15"}, nil)
	dsmock.EXPECT().LoadFewClustersHealth(gomock.Any(), metadb.PostgresqlCluster, "15").Return(datastore.FewClusterHealthInfo{Clusters: clHealth1, NextCursor: "16"}, nil)
	dsmock.EXPECT().LoadFewClustersHealth(gomock.Any(), metadb.PostgresqlCluster, "16").Return(datastore.FewClusterHealthInfo{Clusters: clHealthNextEnv1, NextCursor: "19"}, nil)
	dsmock.EXPECT().LoadFewClustersHealth(gomock.Any(), metadb.PostgresqlCluster, "19").Return(datastore.FewClusterHealthInfo{Clusters: clHealth2, NextCursor: datastore.EndCursor}, nil)
	dsmock.EXPECT().SaveClustersHealth(gomock.Any(), clHealth0, gwCfg.ClusterHealthTimeout).Return(nil)
	dsmock.EXPECT().SaveClustersHealth(gomock.Any(), clHealthNextEnv0, gwCfg.ClusterHealthTimeout).Return(nil)
	dsmock.EXPECT().SaveClustersHealth(gomock.Any(), clHealth1, gwCfg.ClusterHealthTimeout).Return(nil)
	dsmock.EXPECT().SaveClustersHealth(gomock.Any(), clHealthNextEnv1, gwCfg.ClusterHealthTimeout).Return(nil)
	dsmock.EXPECT().SaveClustersHealth(gomock.Any(), clHealth2, gwCfg.ClusterHealthTimeout).Return(nil)
	dsmock.EXPECT().SetAggregateInfo(gomock.Any(), aggInfo, gwCfg.ClusterHealthTimeout).Return(nil)
	dsmock.EXPECT().SetAggregateInfo(gomock.Any(), aggInfoNext, gwCfg.ClusterHealthTimeout).Return(nil)
	dsmock.EXPECT().SetUnhealthyAggregatedInfo(gomock.Any(), metadb.PostgresqlCluster, gomock.Eq(uAggInfo), gomock.Any()).Return(nil)
	lemock.EXPECT().IsLeader(gomock.Any()).Return(true)
	gw.stat = newStat(gw.ds, true)
	gw.processSLI(ctx, metadb.PostgresqlCluster, nowNextEnv)
}
