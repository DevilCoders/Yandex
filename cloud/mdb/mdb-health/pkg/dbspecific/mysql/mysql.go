package mysql

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
	RoleMY    = "mysql_cluster"
	ServiceMY = "mysql"
)

type extractor struct {
}

// New create mysql extractor
func New() dbspecific.Extractor {
	return &extractor{}
}

func (e *extractor) EvaluateClusterHealth(cid string, _ []string, roles dbspecific.HostsMap, health dbspecific.HealthMap) (types.ClusterHealth, error) {
	myHosts, ok := roles[RoleMY]
	if !ok {
		return types.ClusterHealth{
			Cid:    cid,
			Status: types.ClusterStatusUnknown,
		}, xerrors.Errorf("missed required role %s for cid %s", RoleMY, cid)
	}

	myStatus, myTimestamp := dbspecific.CollectStatus(myHosts, health, []string{ServiceMY})
	return types.ClusterHealth{
		Cid:      cid,
		Status:   myStatus,
		StatusTS: myTimestamp,
	}, nil
}

func (e *extractor) EvaluateHostHealth(services []types.ServiceHealth) (types.HostStatus, time.Time) {
	return dbspecific.EvalHostHealthStatus(services, []string{ServiceMY})
}

func findMaster(hostsServices map[string][]types.ServiceHealth) (string, bool) {
	for fqdn, services := range hostsServices {
		for _, s := range services {
			if s.Name() == ServiceMY && s.Role() == types.ServiceRoleMaster {
				return fqdn, true
			}
		}
	}
	return "", false
}

func (e *extractor) EvaluateDBInfo(_, _ string, roles, _ dbspecific.HostsMap, health dbspecific.ModeMap, hostsServices map[string][]types.ServiceHealth) (types.DBRWInfo, error) {
	fqdns, ok := roles[RoleMY]
	if !ok {
		return types.DBRWInfo{}, nil
	}

	info := types.DBRWInfo{
		DBTotal:    1,
		DBRead:     1,
		DBWrite:    1,
		HostsTotal: len(fqdns),
	}
	for _, fqdn := range fqdns {
		hi, ok := health[fqdn]
		if !ok {
			continue
		}
		if hi.UserFaultBroken {
			info.HostsBrokenByUser++
		}
		if hi.Read {
			info.HostsRead++
		}
		if hi.Write {
			info.HostsWrite++
		}
	}

	// Should we somehow specifically handle two masters (HostsWrite > 1)?
	if info.HostsWrite == 0 {
		info.DBWrite = 0
		masterFQDN, ok := findMaster(hostsServices)
		if ok && health[masterFQDN].UserFaultBroken {
			// host with master broken by a user
			info.DBBroken = 1
		}
	}

	if info.HostsRead == 0 {
		info.DBRead = 0
		if info.HostsTotal == info.HostsBrokenByUser {
			// all host are broken by a user
			info.DBBroken = 1
		} else {
			// Master is dead and broken by a user, but replicas are unavailable.
			// Is it user fault? I treat it as ours. We should discuss it.
			info.DBBroken = 0
		}
	}

	if info.DBBroken != 0 {
		// The broken cluster shouldn't have RW metrics
		// https://a.yandex-team.ru/arc/commit/r7450077
		return types.DBRWInfo{
			HostsTotal:        info.HostsTotal,
			DBTotal:           info.DBTotal,
			HostsBrokenByUser: info.HostsBrokenByUser,
			DBBroken:          info.DBBroken,
		}, nil
	}

	return info, nil
}

func (e *extractor) GetSLAGuaranties(cluster datastore.ClusterTopology) (bool, map[string]bool) {
	hostGeos := make(map[string]struct{})
	for _, h := range cluster.Hosts {
		if !slices.ContainsString(h.Roles, string(metadb.MysqlCascadeReplicaRole)) {
			hostGeos[h.Geo] = struct{}{}
		}
	}
	// we have HA hosts in 2 or more different AZ
	slaCluster := len(hostGeos) >= 2
	return slaCluster, nil
}

func (e *extractor) EvaluateGeoAtWarning(_, _, _ dbspecific.HostsMap, _ dbspecific.ModeMap) []string {
	return nil
}
