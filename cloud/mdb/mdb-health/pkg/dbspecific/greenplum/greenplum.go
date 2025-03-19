package greenplum

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (

	/* in greenplum cluster we have 2 subclusters:
	   1. Master subcluster, which consist of master and stanby
	   2. Segments subcluster which consist of segments nodes
	*/

	/* Roles  */
	RoleMaster   = "greenplum_cluster.master_subcluster"
	RoleSegments = "greenplum_cluster.segment_subcluster"
	/* Services */
	ServiceMaster             = "greenplum_master"
	ServiceSegments           = "greenplum_segments"
	ServiceSegmentsAnyAlive   = "greenplum_segments_any_alive"
	ServiceSegmentsAnyPrimary = "greenplum_segments_any_primary"
	ServiceOdyssey            = "odyssey"
)

type extractor struct {
}

var _ dbspecific.Extractor = &extractor{}

// New create sqlserver extractor
func New() dbspecific.Extractor {
	return &extractor{}
}

func checkRole(role string, service string, cid string, roles dbspecific.HostsMap, health dbspecific.HealthMap,
) (roleStatus types.ClusterStatus, timestamp time.Time, err error) {
	greenplumHosts, ok := roles[role]
	if !ok {
		return roleStatus, timestamp, xerrors.Errorf("missed required role %s for cid %s", role, cid)
	}
	roleStatus, timestamp = dbspecific.CollectStatus(greenplumHosts, health, []string{service})
	return roleStatus, timestamp, nil
}

func (e extractor) EvaluateClusterHealth(cid string, fqdns []string, roles dbspecific.HostsMap, health dbspecific.HealthMap) (types.ClusterHealth, error) {
	var timestamp time.Time
	var segmentAnyAliveRoleStatus types.ClusterStatus
	var segmentRoleStatus types.ClusterStatus
	var masterRoleStatus types.ClusterStatus
	var err error
	resultError := types.ClusterHealth{
		Cid:    cid,
		Status: types.ClusterStatusUnknown,
	}
	status := types.ClusterStatusAlive

	segmentRoleStatus, _, err = checkRole(RoleSegments, ServiceSegments, cid, roles, health)
	if err != nil {
		return resultError, err
	}

	segmentAnyAliveRoleStatus, _, err = checkRole(RoleSegments, ServiceSegmentsAnyAlive, cid, roles, health)
	if err != nil {
		return resultError, err
	}

	masterRoleStatus, timestamp, err = checkRole(RoleMaster, ServiceMaster, cid, roles, health)
	if err != nil {
		return resultError, err
	}

	status = dbspecific.StatusAggregate(masterRoleStatus, status)

	/*
		If segmentRoleStatus is dead then greenplum works with errors
		So then if segmentAnyAliveRoleStatus is not unknown then
			If segmentAnyAliveRoleStatus is alive then aggregated status is degraded
			If segmentAnyAliveRoleStatus is dead then aggregated status is dead
		Example:
			3 segments:
			if all segments are alive then alive (segmentRoleStatus is alive)
			if one or two of segments are dead but exists alive segement then degraded
			if all segments are dead then dead
	*/
	if segmentAnyAliveRoleStatus != types.ClusterStatusUnknown {
		if segmentRoleStatus == types.ClusterStatusDead {
			segmentRoleStatus = types.ClusterStatusDegraded
		}
		status = dbspecific.StatusAggregate(segmentAnyAliveRoleStatus, status)
	}
	status = dbspecific.StatusAggregate(segmentRoleStatus, status)

	greenplumMasterHosts := roles[RoleMaster]
	for _, mh := range greenplumMasterHosts {
		isMaster := false
		hasBouncer := false
		for _, serviceHost := range health[mh] {
			if serviceHost.Name() == ServiceMaster {
				isMaster = serviceHost.Role() == types.ServiceRoleMaster
			}
			if serviceHost.Name() == ServiceOdyssey {
				hasBouncer = serviceHost.Status() == types.ServiceStatusAlive
			}
		}

		if isMaster && !hasBouncer {
			status = dbspecific.StatusAggregate(types.ClusterStatusDegraded, status)
		}

		if isMaster {
			break
		}
	}

	return types.ClusterHealth{
		Cid:      cid,
		Status:   status,
		StatusTS: timestamp,
	}, nil
}

func (e extractor) EvaluateHostHealth(services []types.ServiceHealth) (types.HostStatus, time.Time) {
	listServices := [][]string{{ServiceMaster}, {ServiceSegments}}
	checkServices := dbspecific.MatchServices(services, listServices)
	hostStatus, timestamp := dbspecific.EvalHostHealthStatus(services, checkServices)
	if len(checkServices) == 0 {
		return hostStatus, timestamp
	}
	if checkServices[0] == ServiceMaster {
		if hostStatus == types.HostStatusDead {
			return hostStatus, timestamp
		}
		if hostStatus == types.HostStatusUnknown {
			return hostStatus, timestamp
		}
		hostStatusOdyssey, _ := dbspecific.EvalHostHealthStatus(services, []string{ServiceOdyssey})
		if hostStatusOdyssey != types.HostStatusAlive {
			return types.HostStatusDegraded, timestamp
		}
		return hostStatus, timestamp
	}

	if hostStatus == types.HostStatusAlive {
		return hostStatus, timestamp
	}
	hostStatusAnyAlive, _ := dbspecific.EvalHostHealthStatus(services, []string{ServiceSegmentsAnyAlive})
	if hostStatusAnyAlive == types.HostStatusAlive {
		return types.HostStatusDegraded, timestamp
	}
	return hostStatus, timestamp
}

func (e extractor) EvaluateDBInfo(cid, status string, roles, shards dbspecific.HostsMap, health dbspecific.ModeMap, _ map[string][]types.ServiceHealth) (types.DBRWInfo, error) {
	infoMaster := dbspecific.CalcDBInfoCustom(RoleMaster, roles, shards, health, dbspecific.DefaultLimitFunc, dbspecific.CalculateBrokenInfoCustom)

	/*

				in greenplum, there are multiple segments on each host,
				each segment has "master" and read-only mirror
				so we hosts read/write/total here is actually number of r/w/tot node segments in consistent state
				and segments subcluster is considered to be ok only if each segment is in consistent state

				-----------------------------------

				To make everything a clear:

				information about segments r/w availability in aggregated inside greenplum master node system table and
				will be delivered from master (or from his stanby, anyway from master_subcluster ) node.
		        (That means, hosts in segment subcluster do not push any metrics to mdb-health).

		        While collecting metrics, we use the following logic:
		            1. can_write = true if one two thing is true: either this particular segment node does not have
		            any master nor every master of every segment is open for write

		            2. can_read = true if every segment on this particular segment node is open for read
		        TODO: switch to segment monitoring
	*/

	infoSegments := dbspecific.CalculateRWHosts(roles[RoleSegments], health)

	infoSegments.DBTotal++ // whole subcluster itslef

	if infoSegments.HostsTotal == infoSegments.HostsRead {
		infoSegments.DBRead++
	}

	if infoSegments.HostsTotal == infoSegments.HostsWrite {
		infoSegments.DBWrite++
	}

	return infoMaster.Add(infoSegments), nil
}

func (e extractor) GetSLAGuaranties(cluster datastore.ClusterTopology) (bool, map[string]bool) {
	// TODO: implement this properly later
	return true, nil
}

func (e *extractor) EvaluateGeoAtWarning(roles, _, geos dbspecific.HostsMap, health dbspecific.ModeMap) []string {
	return nil
}
