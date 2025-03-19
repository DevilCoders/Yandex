package postgresql

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

var _ dbspecific.Extractor = &extractor{}

const (
	RolePG             = "postgresql_cluster"
	ServiceReplication = "pg_replication"
	ServicePGBouncer   = "pgbouncer"
)

type extractor struct {
}

// New create postgresql extractor
func New() dbspecific.Extractor {
	return &extractor{}
}

func (e *extractor) EvaluateClusterHealth(cid string, _ []string, roles dbspecific.HostsMap, health dbspecific.HealthMap) (types.ClusterHealth, error) {
	pgHosts, ok := roles[RolePG]
	if !ok {
		return types.ClusterHealth{
			Cid:    cid,
			Status: types.ClusterStatusUnknown,
		}, xerrors.Errorf("missed required role %s for cid %s", RolePG, cid)
	}

	pgStatus, pgTimestamp := dbspecific.CollectStatus(pgHosts, health, []string{ServiceReplication, ServicePGBouncer})
	return types.ClusterHealth{
		Cid:      cid,
		Status:   pgStatus,
		StatusTS: pgTimestamp,
	}, nil
}

func (e *extractor) EvaluateHostHealth(services []types.ServiceHealth) (types.HostStatus, time.Time) {
	return dbspecific.EvalHostHealthStatus(services, []string{ServiceReplication, ServicePGBouncer})
}

func (e *extractor) EvaluateDBInfo(_, status string, roles, shards dbspecific.HostsMap, health dbspecific.ModeMap, _ map[string][]types.ServiceHealth) (types.DBRWInfo, error) {
	return dbspecific.CalcDBInfoCustom(RolePG, roles, shards, health, dbspecific.DefaultLimitFunc, dbspecific.CalculateBrokenInfoCustomWithStatusFilter(status)), nil
}

func (e *extractor) GetSLAGuaranties(cluster datastore.ClusterTopology) (bool, map[string]bool) {
	hostGeos := make(map[string]struct{})
	for _, h := range cluster.Hosts {
		if !slices.ContainsString(h.Roles, string(metadb.PostgresqlCascadeReplicaRole)) {
			hostGeos[h.Geo] = struct{}{}
		}
	}
	// we have HA hosts in 2 or more different AZ
	slaCluster := len(hostGeos) >= 2
	return slaCluster, nil
}

func (e *extractor) EvaluateGeoAtWarning(roles, _, geos dbspecific.HostsMap, health dbspecific.ModeMap) []string {
	if len(geos) <= 1 {
		// Single AZ
		return nil
	}

	warningHosts := make(map[string]struct{})
	if fqdns, ok := roles[RolePG]; ok {
		brokenHosts := make(map[string]struct{})
		for _, fqdn := range fqdns {
			if !health[fqdn].Read || !health[fqdn].Write {
				brokenHosts[fqdn] = struct{}{}
			}
		}

		if len(brokenHosts) > 0 {
			for _, fqdn := range fqdns {
				if _, ok = brokenHosts[fqdn]; ok {
					continue
				}
				warningHosts[fqdn] = struct{}{}
			}
		}
	}

	if len(warningHosts) == 0 {
		return nil
	}

	var warningGeos []string
	for geo, fqdns := range geos {
		for _, fqdn := range fqdns {
			if _, ok := warningHosts[fqdn]; ok {
				warningGeos = append(warningGeos, geo)
				break
			}
		}
	}

	return warningGeos
}
