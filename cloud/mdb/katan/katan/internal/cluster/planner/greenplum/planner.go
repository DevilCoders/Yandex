package greenplum

import (
	"fmt"
	"sort"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

type hostRole string

const (
	hostRoleSegment = hostRole("greenplum_cluster.segment_subcluster")
	hostRoleMaster  = hostRole("greenplum_cluster.master_subcluster")
	hostRoleUnknown = hostRole("")

	PlanMaxSpeed = 1

	MasterHealthService  = "greenplum_master"
	SegmentHealthService = "greenplum_segments"

	MasterSubCluster   = "master_subcluster"
	SegmentsSubCluster = "segments_subcluster"

	MasterRole = "Master"
)

func parseHostRole(str string) hostRole {
	switch str {
	case string(hostRoleMaster):
		return hostRoleMaster
	case string(hostRoleSegment):
		return hostRoleSegment
	default:
		return hostRoleUnknown
	}
}

func hostRoleFromMeta(meta tags.HostMeta) (hostRole, error) {
	if len(meta.Roles) != 1 {
		return "", fmt.Errorf("unexpected roles count: %+v", meta.Roles)
	}
	role := parseHostRole(meta.Roles[0])
	if role == hostRoleUnknown {
		return "", fmt.Errorf("unknown host role: %+v", meta.Roles[0])
	}
	return role, nil
}

func GroupBySubCluster(cluster planner.Cluster) (map[string][]string, error) {
	var master string
	subClusterGroups := map[string][]string{
		MasterSubCluster:   {},
		SegmentsSubCluster: {},
	}
	for fqdn, host := range cluster.Hosts {
		role, err := hostRoleFromMeta(host.Tags.Meta)
		if err != nil {
			return nil, err
		}

		switch role {
		case hostRoleMaster:
			masterHosts := subClusterGroups[MasterSubCluster]
			service, ok := host.Services[MasterHealthService]
			if !ok {
				return nil, fmt.Errorf("greenplum_master service expected, but given %q on host: %q", host.Services, fqdn)
			}
			switch service.Role {
			case types.ServiceRoleMaster:
				master = fqdn
			case types.ServiceRoleReplica:
				subClusterGroups[MasterSubCluster] = append(masterHosts, fqdn)
			default:
				return nil, fmt.Errorf("unsupported service role: %q. On %q: %+v", service.Role, fqdn, service)
			}

		case hostRoleSegment:
			segmentHosts := subClusterGroups[SegmentsSubCluster]
			subClusterGroups[SegmentsSubCluster] = append(segmentHosts, fqdn)
		default:
			panic(
				fmt.Sprintf("got itself into trouble: found %+v role", hostRoleUnknown))
		}

	}

	for subCluster := range subClusterGroups {
		sort.Strings(subClusterGroups[subCluster])
	}

	//Add master at the end of master_subcluster fqdns
	subClusterGroups[MasterSubCluster] = append(subClusterGroups[MasterSubCluster], master)

	return subClusterGroups, nil
}

func Assert(subClusters map[string][]string, cluster planner.Cluster) error {
	if len(subClusters[MasterSubCluster]) == 0 {
		return fmt.Errorf("master not found on hosts: %+v", cluster.Hosts)
	}

	for _, fqdn := range subClusters[MasterSubCluster] {
		host := cluster.Hosts[fqdn]
		_, ok := host.Services[MasterHealthService]
		if !ok {
			return fmt.Errorf("greenplum_master service expected, but given %q on host: %q", host.Services, fqdn)
		}
	}

	if len(subClusters[SegmentsSubCluster]) == 0 {
		return fmt.Errorf("segments not found on hosts: %+v", cluster.Hosts)
	}

	for _, fqdn := range subClusters[SegmentsSubCluster] {
		host := cluster.Hosts[fqdn]
		_, ok := host.Services[SegmentHealthService]
		if !ok {
			return fmt.Errorf("greenplum_segments service expected, but given %q on host: %q", host.Services, fqdn)
		}
	}
	return nil
}

func Planner(cluster planner.Cluster) ([][]string, error) {
	if len(cluster.Hosts) == 0 {
		return nil, fmt.Errorf("got empty cluster: %+v", cluster)
	}

	// Create clusters map
	subClusters, err := GroupBySubCluster(cluster)
	if err != nil {
		return nil, err
	}

	// Run checks
	err = Assert(subClusters, cluster)
	if err != nil {
		return nil, err
	}

	// Create plan
	var plan [][]string
	plan = planner.LinearPlan(subClusters[SegmentsSubCluster], PlanMaxSpeed)
	plan = append(plan, planner.LinearPlan(subClusters[MasterSubCluster], PlanMaxSpeed)...)
	return plan, nil
}
