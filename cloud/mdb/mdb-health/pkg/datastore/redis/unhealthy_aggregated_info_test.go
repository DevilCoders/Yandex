package redis_test

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/internal/unhealthy"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

func TestUnhealthyAggregatedInfoSetAndLoadCorrectly(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)
	uai := unhealthy.UAInfo{
		RWRecs:         make(map[unhealthy.RWKey]*unhealthy.UARWRecord),
		StatusRecs:     make(map[unhealthy.StatusKey]*unhealthy.UARecord),
		WarningGeoRecs: make(map[unhealthy.GeoKey]*unhealthy.UARecord),
	}
	uai.StatusRecs[unhealthy.StatusKey{Env: "qa", Status: string(types.ClusterStatusDegraded)}] =
		&unhealthy.UARecord{Count: 1, Examples: []string{"cid1"}}
	uai.StatusRecs[unhealthy.StatusKey{Env: "qa", Status: string(types.ClusterStatusUnknown)}] =
		&unhealthy.UARecord{Count: 1, Examples: []string{"cid3"}}
	uai.StatusRecs[unhealthy.StatusKey{Env: "qa", Status: string(types.ClusterStatusDead)}] =
		&unhealthy.UARecord{Count: 2, Examples: []string{"cid2", "cid4"}}
	uai.RWRecs[unhealthy.RWKey{AggType: types.AggClusters, Env: "qa", Readable: true, Writeable: true}] =
		&unhealthy.UARWRecord{Count: 2}
	uai.RWRecs[unhealthy.RWKey{AggType: types.AggClusters, Env: "qa"}] =
		&unhealthy.UARWRecord{Count: 2, NoWriteCount: 1, NoReadCount: 1, Examples: []string{"cid5", "cid6"}}
	uai.RWRecs[unhealthy.RWKey{AggType: types.AggClusters, Env: "qa", Readable: true}] =
		&unhealthy.UARWRecord{Count: 2, NoWriteCount: 2, Examples: []string{"cid2", "cid4"}}
	uai.RWRecs[unhealthy.RWKey{AggType: types.AggShards, Env: "qa", Readable: true}] =
		&unhealthy.UARWRecord{Count: 2, NoWriteCount: 2, Examples: []string{"cid2", "cid4"}}
	uai.RWRecs[unhealthy.RWKey{AggType: types.AggShards, SLA: true, Env: "qa", Readable: true}] =
		&unhealthy.UARWRecord{Count: 2, NoWriteCount: 2, Examples: []string{"cid2", "cid4"}}

	err := ds.SetUnhealthyAggregatedInfo(ctx, metadb.PostgresqlCluster, uai, time.Hour)
	require.NoError(t, err)

	loadedUAI, err := ds.LoadUnhealthyAggregatedInfo(ctx, metadb.PostgresqlCluster, types.AggClusters)
	require.NoError(t, err)
	delete(uai.RWRecs, unhealthy.RWKey{AggType: types.AggShards, SLA: false, Env: "qa", Readable: true})
	delete(uai.RWRecs, unhealthy.RWKey{AggType: types.AggShards, SLA: true, Env: "qa", Readable: true})
	require.Equal(t, uai, loadedUAI)

	newUAI := unhealthy.UAInfo{
		RWRecs:         make(map[unhealthy.RWKey]*unhealthy.UARWRecord),
		StatusRecs:     make(map[unhealthy.StatusKey]*unhealthy.UARecord),
		WarningGeoRecs: make(map[unhealthy.GeoKey]*unhealthy.UARecord),
	}
	newUAI.StatusRecs[unhealthy.StatusKey{SLA: false, Env: "qa", Status: string(types.ClusterStatusDegraded)}] =
		&unhealthy.UARecord{Count: 1, Examples: []string{"cid1"}}
	newUAI.RWRecs[unhealthy.RWKey{SLA: false, Env: "qa", Writeable: false, Readable: false, AggType: types.AggShards}] =
		&unhealthy.UARWRecord{Count: 1, Examples: []string{"cid1"}, NoWriteCount: 1, NoReadCount: 1}

	err = ds.SetUnhealthyAggregatedInfo(ctx, metadb.PostgresqlCluster, newUAI, time.Hour)
	require.NoError(t, err)
	newLoadedUAI, err := ds.LoadUnhealthyAggregatedInfo(ctx, metadb.PostgresqlCluster, types.AggShards)
	require.NoError(t, err)
	delete(newUAI.StatusRecs, unhealthy.StatusKey{Env: "qa", Status: string(types.ClusterStatusDegraded)})
	require.Equal(t, newUAI, newLoadedUAI)

}
