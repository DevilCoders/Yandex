package chmodels

import (
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

type HostSpec struct {
	ZoneID         string
	HostRole       hosts.Role
	SubnetID       string
	ShardName      string
	AssignPublicIP bool
}

type UpdateHostSpec struct {
	HostName       string
	AssignPublicIP optional.Bool
}

type ClusterHosts struct {
	ClickHouseNodes []HostSpec
	ZooKeeperNodes  []HostSpec
}

func (ch ClusterHosts) NeedZookeeper() bool {
	if len(ch.ZooKeeperNodes) > 0 {
		return true
	}

	shards := map[string]struct{}{}
	for _, host := range ch.ClickHouseNodes {
		if _, ok := shards[host.ShardName]; ok {
			return true
		}

		shards[host.ShardName] = struct{}{}
	}

	return false
}

func (ch ClusterHosts) Count() int64 {
	return int64(len(ch.ClickHouseNodes) + len(ch.ZooKeeperNodes))
}

func SplitHostsByType(hostSpecs []HostSpec) (ClusterHosts, error) {
	var chHosts []HostSpec
	var zkHosts []HostSpec
	for _, host := range hostSpecs {
		switch host.HostRole {
		case hosts.RoleClickHouse:
			chHosts = append(chHosts, host)
		case hosts.RoleZooKeeper:
			zkHosts = append(zkHosts, host)
		default:
			return ClusterHosts{}, semerr.InvalidInputf("invalid host role %s", host.HostRole.String())
		}
	}
	return ClusterHosts{ClickHouseNodes: chHosts, ZooKeeperNodes: zkHosts}, nil
}

func SplitHostsExtendedByShard(hostSpecs []hosts.HostExtended) map[string][]hosts.HostExtended {
	shardMap := map[string][]hosts.HostExtended{}
	for _, host := range hostSpecs {
		shardMap[host.ShardID.String] = append(shardMap[host.ShardID.String], host)
	}
	return shardMap
}

func ToZoneHosts(hostSpecs []HostSpec) clusterslogic.ZoneHostsList {
	zoneMap := map[string]int64{}
	for _, host := range hostSpecs {
		count := zoneMap[host.ZoneID]
		zoneMap[host.ZoneID] = count + 1
	}
	res := make([]clusterslogic.ZoneHosts, 0, len(zoneMap))
	for zone, count := range zoneMap {
		res = append(res, clusterslogic.ZoneHosts{ZoneID: zone, Count: count})
	}
	return res
}

func ZonesFromHostSpecs(hostSpecs []HostSpec) []string {
	zoneMap := map[string]struct{}{}
	for _, host := range hostSpecs {
		zoneMap[host.ZoneID] = struct{}{}
	}
	res := make([]string, 0, len(zoneMap))
	for zone := range zoneMap {
		res = append(res, zone)
	}
	return res
}

func ZonesFromHostSpecsByRole(hostSpecs []HostSpec, roleFilter hosts.Role) []string {
	zoneMap := map[string]struct{}{}
	for _, host := range hostSpecs {
		if host.HostRole == roleFilter {
			zoneMap[host.ZoneID] = struct{}{}
		}
	}

	res := make([]string, 0, len(zoneMap))
	for zone := range zoneMap {
		res = append(res, zone)
	}

	return res
}

func ZonesFromHosts(chHosts []hosts.HostExtended, hostSpecs []HostSpec) []string {
	zoneMap := map[string]struct{}{}
	for _, host := range chHosts {
		zoneMap[host.ZoneID] = struct{}{}
	}

	for _, host := range hostSpecs {
		zoneMap[host.ZoneID] = struct{}{}
	}

	res := make([]string, 0, len(zoneMap))
	for zone := range zoneMap {
		res = append(res, zone)
	}
	return res
}

func SelectSubnetForZone(zone string, subnets []networkProvider.Subnet, hostSpecs []HostSpec) string {
	for _, host := range hostSpecs {
		if host.ZoneID == zone {
			//  Select one that used by clickhouse if specified
			return host.SubnetID
		}
	}

	for _, subnet := range subnets {
		if subnet.ZoneID == zone {
			//  Select first from list
			return subnet.ID
		}
	}

	return ""
}
