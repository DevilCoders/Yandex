package mongodb

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
	RoleMongod      = "mongodb_cluster.mongod"
	RoleMongos      = "mongodb_cluster.mongos"
	RoleMongocfg    = "mongodb_cluster.mongocfg"
	RoleMongoinfra  = "mongodb_cluster.mongoinfra"
	ServiceMongod   = "mongod"
	ServiceMongos   = "mongos"
	ServiceMongocfg = "mongocfg"
)

type extractor struct {
}

// New create mongodb extractor
func New() dbspecific.Extractor {
	return &extractor{}
}

func (e *extractor) EvaluateClusterHealth(cid string, fqdns []string, roles dbspecific.HostsMap, health dbspecific.HealthMap) (types.ClusterHealth, error) {
	mongodHosts, ok := roles[RoleMongod]
	if !ok {
		return types.ClusterHealth{
			Cid:    cid,
			Status: types.ClusterStatusUnknown,
		}, xerrors.Errorf("missed required role %s for cid %s", RoleMongod, cid)
	}

	mongodStatus, mongodTimestamp := dbspecific.CollectStatus(mongodHosts, health, []string{ServiceMongod})
	if len(roles) == 1 || mongodStatus == types.ClusterStatusDead {
		return types.ClusterHealth{
			Cid:      cid,
			Status:   mongodStatus,
			StatusTS: mongodTimestamp,
		}, nil
	}

	mongocfgHosts, okMongocfg := roles[RoleMongocfg]
	mongocfgTimestamp := time.Time{}
	if okMongocfg {
		mongocfgStatus := types.ClusterStatusUnknown
		mongocfgStatus, mongocfgTimestamp = dbspecific.CollectStatus(mongocfgHosts, health, []string{ServiceMongocfg})
		mongodStatus = dbspecific.StatusAggregate(mongocfgStatus, mongodStatus)
	}

	mongosHosts, okMongos := roles[RoleMongos]
	mongosTimestamp := time.Time{}
	if okMongos {
		mongosStatus := types.ClusterStatusUnknown
		mongosStatus, mongosTimestamp = dbspecific.CollectStatus(mongosHosts, health, []string{ServiceMongos})
		mongodStatus = dbspecific.StatusAggregate(mongosStatus, mongodStatus)
	}

	mongoinfraHosts, okMongoinfra := roles[RoleMongoinfra]
	mongoinfraTimestamp := time.Time{}
	if okMongoinfra {
		mongoinfraStatus := types.ClusterStatusUnknown
		mongoinfraStatus, mongoinfraTimestamp = dbspecific.CollectStatus(mongoinfraHosts, health, []string{ServiceMongos, ServiceMongocfg})
		mongodStatus = dbspecific.StatusAggregate(mongoinfraStatus, mongodStatus)
	}

	if !okMongoinfra && !(okMongos && okMongocfg) {
		return types.ClusterHealth{
			Cid:    cid,
			Status: types.ClusterStatusUnknown,
		}, xerrors.Errorf("missed required roles %s or %s + %s for cid %s", RoleMongoinfra, RoleMongos, RoleMongocfg, cid)
	}

	return types.ClusterHealth{
		Cid:      cid,
		Status:   mongodStatus,
		StatusTS: dbspecific.MinTS(mongodTimestamp, mongocfgTimestamp, mongosTimestamp, mongoinfraTimestamp),
	}, nil
}

func (e *extractor) EvaluateHostHealth(services []types.ServiceHealth) (types.HostStatus, time.Time) {
	listServices := [][]string{{ServiceMongod}, {ServiceMongos}, {ServiceMongocfg}}
	checkServices := dbspecific.MatchServices(services, listServices)
	return dbspecific.EvalHostHealthStatus(services, checkServices)
}

func (e *extractor) EvaluateDBInfo(cid, status string, roles, shards dbspecific.HostsMap, health dbspecific.ModeMap, _ map[string][]types.ServiceHealth) (types.DBRWInfo, error) {
	var limitFunc dbspecific.LimitFunc = func(i int) int {
		return 0
	}
	return dbspecific.CalcDBInfoCustom(RoleMongod, roles, shards, health, limitFunc, dbspecific.CalculateBrokenInfoCustom), nil
}

func (e *extractor) GetSLAGuaranties(cluster datastore.ClusterTopology) (bool, map[string]bool) {
	roleGeos := make(map[string]uint)
	shardGeos := make(map[string]uint)
	for _, h := range cluster.Hosts {
		geo := dbspecific.GeoBit(h.Geo)
		if h.ShardID.Valid {
			shard := h.ShardID.Must()
			shardGeos[shard] |= geo
			continue
		}
		for _, r := range h.Roles {
			roleGeos[r] |= geo
		}
	}

	if len(shardGeos) == 0 {
		return bits.OnesCount(roleGeos[RoleMongod]) >= 3, nil
	}

	slaAllowShards := (bits.OnesCount(roleGeos[RoleMongos]) >= 2 && bits.OnesCount(roleGeos[RoleMongocfg]) >= 3) || bits.OnesCount(roleGeos[RoleMongoinfra]) >= 3 || len(shardGeos) == 1
	slaCluster := slaAllowShards
	slaShards := make(map[string]bool, len(shardGeos))
	for shard, geos := range shardGeos {
		sla := slaAllowShards && bits.OnesCount(geos) >= 3
		slaShards[shard] = sla
		if !sla {
			slaCluster = false
		}
	}

	return slaCluster, slaShards
}

func (e *extractor) EvaluateGeoAtWarning(roles, _, geos dbspecific.HostsMap, health dbspecific.ModeMap) []string {
	return nil
}
