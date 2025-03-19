package sqlserver

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ dbspecific.Extractor = &extractor{}

const (
	roleSQLServer    = "sqlserver_cluster"
	roleWitness      = "windows_witness"
	serviceSqlserver = "sqlserver"
	serviceWitness   = "witness"
)

type extractor struct {
}

// New create sqlserver extractor
func New() dbspecific.Extractor {
	return &extractor{}
}

func (e extractor) EvaluateClusterHealth(cid string, fqdns []string, roles dbspecific.HostsMap, health dbspecific.HealthMap) (types.ClusterHealth, error) {
	sqlserverHosts, ok := roles[roleSQLServer]
	if !ok {
		return types.ClusterHealth{
			Cid:    cid,
			Status: types.ClusterStatusUnknown,
		}, xerrors.Errorf("missed required role %s for cid %s", roleSQLServer, cid)
	}
	witnessHosts, witnessPresent := roles[roleWitness]

	sqlserverStatus, sqlserverTimestamp := dbspecific.CollectStatus(sqlserverHosts, health, []string{serviceSqlserver})
	if witnessPresent {
		witnessStatus, witnessTimestamp := dbspecific.CollectStatus(witnessHosts, health, []string{serviceWitness})
		if !witnessTimestamp.IsZero() && witnessTimestamp.Before(sqlserverTimestamp) {
			sqlserverTimestamp = witnessTimestamp
		}
		if sqlserverStatus != witnessStatus && sqlserverStatus == types.ClusterStatusAlive {
			sqlserverStatus = types.ClusterStatusDegraded
		}
	}

	return types.ClusterHealth{
		Cid:      cid,
		Status:   sqlserverStatus,
		StatusTS: sqlserverTimestamp,
	}, nil
}

func (e extractor) EvaluateHostHealth(services []types.ServiceHealth) (types.HostStatus, time.Time) {
	listServices := [][]string{{serviceSqlserver}, {serviceWitness}}
	checkServices := dbspecific.MatchServices(services, listServices)
	return dbspecific.EvalHostHealthStatus(services, checkServices)
}

func (e extractor) EvaluateDBInfo(cid, status string, roles, shards dbspecific.HostsMap, health dbspecific.ModeMap, _ map[string][]types.ServiceHealth) (types.DBRWInfo, error) {
	return dbspecific.CalcDBInfoCustom(roleSQLServer, roles, shards, health, dbspecific.DefaultLimitFunc, dbspecific.CalculateBrokenInfoCustom), nil
}

func (e extractor) GetSLAGuaranties(cluster datastore.ClusterTopology) (bool, map[string]bool) {
	hostGeos := make(map[string]struct{})
	for _, h := range cluster.Hosts {
		hostGeos[h.Geo] = struct{}{}
	}
	// we have HA hosts in 2 or more different AZ
	slaCluster := len(hostGeos) >= 2
	return slaCluster, nil
}

func (e *extractor) EvaluateGeoAtWarning(roles, _, geos dbspecific.HostsMap, health dbspecific.ModeMap) []string {
	return nil
}
