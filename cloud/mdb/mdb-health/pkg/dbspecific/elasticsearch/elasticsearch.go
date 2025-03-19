package elasticsearch

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
	RoleData      = "elasticsearch_cluster.datanode"
	RoleMaster    = "elasticsearch_cluster.masternode"
	serviceES     = "elasticsearch"
	serviceKibana = "kibana"
)

type extractor struct {
}

// New create elasticsearch extractor
func New() dbspecific.Extractor {
	return &extractor{}
}

func (e *extractor) EvaluateClusterHealth(cid string, fqdns []string, roles dbspecific.HostsMap, health dbspecific.HealthMap) (types.ClusterHealth, error) {
	dataHosts, ok := roles[RoleData]
	if !ok {
		return types.ClusterHealth{
			Cid:    cid,
			Status: types.ClusterStatusUnknown,
		}, xerrors.Errorf("missed required role %s for cid %s", RoleData, cid)
	}

	dataStatus, dataTimestamp := dbspecific.CollectStatus(dataHosts, health, []string{serviceES})
	if len(roles) == 1 || dataStatus == types.ClusterStatusDead {
		return types.ClusterHealth{
			Cid:      cid,
			Status:   dataStatus,
			StatusTS: dataTimestamp,
		}, nil
	}

	masterHosts, ok := roles[RoleMaster]
	if !ok {
		return types.ClusterHealth{
			Cid:    cid,
			Status: types.ClusterStatusUnknown,
		}, xerrors.Errorf("missed required role %s for cid %s", RoleMaster, cid)
	}

	masterStatus, masterTimestamp := dbspecific.CollectStatus(masterHosts, health, []string{serviceES})
	if !masterTimestamp.IsZero() && masterTimestamp.Before(dataTimestamp) {
		dataTimestamp = masterTimestamp
	}
	if dataStatus != masterStatus && dataStatus == types.ClusterStatusAlive {
		dataStatus = types.ClusterStatusDegraded
	}
	return types.ClusterHealth{
		Cid:      cid,
		Status:   dataStatus,
		StatusTS: dataTimestamp,
	}, nil
}

func (e *extractor) EvaluateHostHealth(services []types.ServiceHealth) (types.HostStatus, time.Time) {
	listServices := [][]string{{serviceES}, {serviceKibana}}
	checkServices := dbspecific.MatchServices(services, listServices)
	return dbspecific.EvalHostHealthStatus(services, checkServices)
}

func (e *extractor) EvaluateDBInfo(cid, status string, roles, shards dbspecific.HostsMap, health dbspecific.ModeMap, _ map[string][]types.ServiceHealth) (types.DBRWInfo, error) {
	return dbspecific.CalcDBInfoCustom(RoleData, roles, shards, health, dbspecific.DefaultLimitFunc, dbspecific.CalculateBrokenInfoCustom), nil
}

func (e *extractor) GetSLAGuaranties(cluster datastore.ClusterTopology) (bool, map[string]bool) {
	roleGeos := make(map[string]uint)
	for _, h := range cluster.Hosts {
		geo := dbspecific.GeoBit(h.Geo)
		for _, r := range h.Roles {
			roleGeos[r] |= geo
		}
	}

	return bits.OnesCount(roleGeos[RoleData]) >= 2, nil
}

func (e *extractor) EvaluateGeoAtWarning(roles, _, geos dbspecific.HostsMap, health dbspecific.ModeMap) []string {
	return nil
}
