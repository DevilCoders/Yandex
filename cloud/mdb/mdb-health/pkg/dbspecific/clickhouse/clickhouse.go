package clickhouse

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
	RoleCH    = "clickhouse_cluster"
	roleZK    = "zk"
	serviceCH = "clickhouse"
	serviceZK = "zookeeper"
)

type extractor struct {
}

// New create clickhouse extractor
func New() dbspecific.Extractor {
	return &extractor{}
}

func (e *extractor) EvaluateClusterHealth(cid string, fqdns []string, roles dbspecific.HostsMap, health dbspecific.HealthMap) (types.ClusterHealth, error) {
	chHosts, ok := roles[RoleCH]
	if !ok {
		return types.ClusterHealth{
			Cid:    cid,
			Status: types.ClusterStatusUnknown,
		}, xerrors.Errorf("missed required role %s for cid %s", RoleCH, cid)
	}

	chStatus, chTimestamp := dbspecific.CollectStatus(chHosts, health, []string{serviceCH})
	if len(roles) == 1 || chStatus == types.ClusterStatusDead {
		return types.ClusterHealth{
			Cid:      cid,
			Status:   chStatus,
			StatusTS: chTimestamp,
		}, nil
	}

	zkHosts, ok := roles[roleZK]
	if !ok {
		return types.ClusterHealth{
			Cid:    cid,
			Status: types.ClusterStatusUnknown,
		}, xerrors.Errorf("missed required role %s for cid %s", roleZK, cid)
	}

	zkStatus, zkTimestamp := dbspecific.CollectStatus(zkHosts, health, []string{serviceZK})
	if !zkTimestamp.IsZero() && zkTimestamp.Before(chTimestamp) {
		chTimestamp = zkTimestamp
	}
	if chStatus != zkStatus && chStatus == types.ClusterStatusAlive {
		chStatus = types.ClusterStatusDegraded
	}
	return types.ClusterHealth{
		Cid:      cid,
		Status:   chStatus,
		StatusTS: chTimestamp,
	}, nil
}

func (e *extractor) EvaluateHostHealth(services []types.ServiceHealth) (types.HostStatus, time.Time) {
	listServices := [][]string{{serviceCH}, {serviceZK}}
	checkServices := dbspecific.MatchServices(services, listServices)
	return dbspecific.EvalHostHealthStatus(services, checkServices)
}

func (e *extractor) EvaluateDBInfo(cid, status string, roles, shards dbspecific.HostsMap, health dbspecific.ModeMap, _ map[string][]types.ServiceHealth) (types.DBRWInfo, error) {
	return dbspecific.CalcDBInfoCustom(RoleCH, roles, shards, health, dbspecific.DefaultLimitFunc, calculateUserBrokenInfo(status)), nil
}

func (e *extractor) GetSLAGuaranties(cluster datastore.ClusterTopology) (bool, map[string]bool) {
	shardGeos := make(map[string]uint)       // map[shardId]=geoBits
	shardMatureGeos := make(map[string]uint) // map[shardId]=matureGeoBits
	zkGeoCounts := make(map[uint]int)        // map[geo]=count
	zkCount := 0

	matureTime := time.Now().Add(-1 * time.Hour)

	for _, h := range cluster.Hosts {
		geo := dbspecific.GeoBit(h.Geo)
		hostRole := RoleCH
		for _, role := range h.Roles {
			if role == roleZK {
				hostRole = roleZK
				break
			}
		}
		if hostRole == roleZK {
			zkGeoCounts[geo]++
			zkCount++
		} else if h.ShardID.Valid {
			shard := h.ShardID.Must()
			shardGeos[shard] |= geo
			if h.CreatedAt.Before(matureTime) {
				shardMatureGeos[shard] |= geo
			}
		}
	}

	if len(shardGeos) < 1 {
		return false, nil
	}

	slaShards := make(map[string]bool, len(shardGeos))
	slaCluster := true
	slaZK := true

	zkMajority := (zkCount + 1) / 2
	for _, count := range zkGeoCounts {
		if zkMajority <= count {
			slaCluster = false
			slaZK = false
			break
		}
	}

	for shard, geos := range shardGeos {
		sla := slaZK && bits.OnesCount(geos) >= 2
		matureSLA := sla && bits.OnesCount(shardMatureGeos[shard]) >= 2
		slaShards[shard] = matureSLA
		if !sla {
			slaCluster = false
		}
	}

	return slaCluster, slaShards
}

func (e *extractor) EvaluateGeoAtWarning(roles, _, geos dbspecific.HostsMap, health dbspecific.ModeMap) []string {
	return nil
}

// calculateUserBrokenInfo acts like dbspecific.CalculateBrokenInfoCustom but if cluster in MODIFYING state, only one host needed to mark shard user broken.
func calculateUserBrokenInfo(clusterStatus string) dbspecific.CalculateUserBrokenFunc {
	return func(_ string, _, shards dbspecific.HostsMap, health dbspecific.ModeMap, limitFunc dbspecific.LimitFunc) (types.DBRWInfo, map[string]struct{}) {
		var info types.DBRWInfo
		mapBroken := make(map[string]struct{})

		for sid, fqdns := range shards {
			var brokenByUser int
			var brokenByServiceOnRead int
			var readAvailableCnt int
			var writeAvailableCnt int

			for _, fqdn := range fqdns {
				hi, ok := health[fqdn]
				if !ok {
					continue
				}

				if hi.Read {
					readAvailableCnt++
				}

				if hi.Write {
					writeAvailableCnt++
				}

				if hi.UserFaultBroken {
					brokenByUser++
				} else if !hi.Read {
					brokenByServiceOnRead++
				}
			}

			info.HostsBrokenByUser += brokenByUser
			if writeAvailableCnt == 0 || readAvailableCnt <= limitFunc(len(fqdns)) {
				byUserLimit := 0
				if clusterStatus != "MODIFYING" {
					byUserLimit = limitFunc(len(fqdns))
				}

				if brokenByUser > byUserLimit {
					info.DBBroken++
					mapBroken[sid] = struct{}{}
				}
			}
		}
		return info, mapBroken
	}
}
