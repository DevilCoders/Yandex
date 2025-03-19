package redis

import (
	"math/bits"
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ dbspecific.Extractor = &extractor{}

const (
	RoleRedis           = "redis_cluster"
	ServiceRedisCluster = "redis_cluster"
	ServiceRedis        = "redis"
	ServiceSentinel     = "sentinel"
)

type extractor struct {
}

// New create redis extractor
func New() dbspecific.Extractor {
	return &extractor{}
}

func (e *extractor) EvaluateClusterHealth(cid string, _ []string, roles dbspecific.HostsMap, health dbspecific.HealthMap) (types.ClusterHealth, error) {
	redisHosts, ok := roles[RoleRedis]
	if !ok {
		return types.ClusterHealth{
			Cid:    cid,
			Status: types.ClusterStatusUnknown,
		}, xerrors.Errorf("missed required role %s for cid %s", RoleRedis, cid)
	}

	listServices := [][]string{{ServiceRedisCluster}, {ServiceRedis, ServiceSentinel}}
	redisStatus, redisTimestamp := dbspecific.CollectStatusOneOf(redisHosts, health, listServices)
	return types.ClusterHealth{
		Cid:      cid,
		Status:   redisStatus,
		StatusTS: redisTimestamp,
	}, nil
}

func (e *extractor) EvaluateHostHealth(services []types.ServiceHealth) (types.HostStatus, time.Time) {
	listServices := [][]string{{ServiceRedisCluster}, {ServiceRedis, ServiceSentinel}}
	checkServices := dbspecific.MatchServices(services, listServices)
	return dbspecific.EvalHostHealthStatus(services, checkServices)
}

func (e *extractor) EvaluateDBInfo(_, _ string, roles, shards dbspecific.HostsMap, health dbspecific.ModeMap, _ map[string][]types.ServiceHealth) (types.DBRWInfo, error) {
	var limitFunc dbspecific.LimitFunc = func(i int) int {
		return 0
	}
	return dbspecific.CalcDBInfoCustom(RoleRedis, roles, shards, health, limitFunc, dbspecific.CalculateBrokenInfoCustom), nil
}

// GetSLAGuaranties will return whether cluster-level and shards-level SLAs are satisfied.
// Redis requires 3 hosts for ordinary clusters and at least 3 shards with 2 hosts each for sharded clusters.
func (e *extractor) GetSLAGuaranties(cluster datastore.ClusterTopology) (bool, map[string]bool) {
	shardGeos := make(map[string]uint)
	for _, h := range cluster.Hosts {
		geo := dbspecific.GeoBit(h.Geo)
		if h.ShardID.Valid {
			shard := h.ShardID.Must()
			shardGeos[shard] |= geo
		}
	}

	if len(shardGeos) < 1 {
		return false, nil
	}

	if len(shardGeos) == 1 {
		for _, geos := range shardGeos {
			return bits.OnesCount(geos) >= 3, nil
		}
	}

	slaShards := make(map[string]bool, len(shardGeos))
	slaCluster := true
	for shard, geos := range shardGeos {
		sla := bits.OnesCount(geos) >= 2
		slaShards[shard] = sla
		if !sla {
			slaCluster = false
		}
	}

	return slaCluster, slaShards
}

func (e *extractor) EvaluateGeoAtWarning(_, _, _ dbspecific.HostsMap, _ dbspecific.ModeMap) []string {
	return nil
}
