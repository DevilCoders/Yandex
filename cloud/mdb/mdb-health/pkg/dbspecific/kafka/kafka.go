package kafka

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ dbspecific.Extractor = &extractor{}

const (
	roleKafka    = "kafka_cluster"
	roleZK       = "zk"
	serviceKafka = "kafka"
	serviceZK    = "zookeeper"
)

type extractor struct {
}

// New create kafka extractor
func New() dbspecific.Extractor {
	return &extractor{}
}

func (e *extractor) EvaluateClusterHealth(cid string, fqdns []string, roles dbspecific.HostsMap, health dbspecific.HealthMap) (types.ClusterHealth, error) {
	brokerHosts, ok := roles[roleKafka]
	if !ok {
		return types.ClusterHealth{
			Cid:    cid,
			Status: types.ClusterStatusUnknown,
		}, xerrors.Errorf("missed required role %s for cid %s", roleKafka, cid)
	}

	kafkaStatus, kafkaTimestamp := dbspecific.CollectStatus(brokerHosts, health, []string{serviceKafka})
	if len(roles) == 1 || kafkaStatus == types.ClusterStatusDead {
		return types.ClusterHealth{
			Cid:      cid,
			Status:   kafkaStatus,
			StatusTS: kafkaTimestamp,
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
	if !zkTimestamp.IsZero() && zkTimestamp.Before(kafkaTimestamp) {
		kafkaTimestamp = zkTimestamp
	}
	if kafkaStatus != zkStatus && kafkaStatus == types.ClusterStatusAlive {
		kafkaStatus = types.ClusterStatusDegraded
	}
	return types.ClusterHealth{
		Cid:      cid,
		Status:   kafkaStatus,
		StatusTS: kafkaTimestamp,
	}, nil
}

func (e *extractor) EvaluateHostHealth(services []types.ServiceHealth) (types.HostStatus, time.Time) {
	listServices := [][]string{{serviceKafka}, {serviceZK}}
	checkServices := dbspecific.MatchServices(services, listServices)
	return dbspecific.EvalHostHealthStatus(services, checkServices)
}

func (e *extractor) EvaluateDBInfo(cid, status string, roles, shards dbspecific.HostsMap, health dbspecific.ModeMap, _ map[string][]types.ServiceHealth) (types.DBRWInfo, error) {
	return dbspecific.CalcDBInfoCustom(roleKafka, roles, shards, health, dbspecific.DefaultLimitFunc, dbspecific.CalculateBrokenInfoCustom), nil
}

func (e *extractor) GetSLAGuaranties(cluster datastore.ClusterTopology) (bool, map[string]bool) {
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
