package hosts

import (
	"database/sql"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/slices"
)

type Host struct {
	ClusterID           string
	SubClusterID        string
	ShardID             optional.String
	SpaceLimit          int64
	ResourcePresetExtID string
	ResourcePresetID    resources.PresetID
	ZoneID              string
	FQDN                string
	DiskTypeExtID       string
	SubnetID            string
	AssignPublicIP      bool
	Revision            int64
	Roles               []Role
	VType               string
	VTypeID             sql.NullString
	ReplicaPriority     optional.Int64
}

// HostExtended holds dynamic information like health
// TODO: refactor this... clearly separate metadb, logic and cluster-typed models
type HostExtended struct {
	Host
	Health Health
}

func HostsExtendedToHosts(hs []HostExtended) []Host {
	res := make([]Host, 0, len(hs))
	for _, h := range hs {
		res = append(res, h.Host)
	}

	return res
}

func GetUniqueZones(hosts []Host) []string {
	var zones []string
	for _, host := range hosts {
		if !slices.ContainsString(zones, host.ZoneID) {
			zones = append(zones, host.ZoneID)
		}
	}
	return zones
}

func GetFQDNs(hosts []Host) []string {
	fqdns := make([]string, 0, len(hosts))
	for _, host := range hosts {
		fqdns = append(fqdns, host.FQDN)

	}
	return fqdns
}

func SplitHostsExtendedByShard(hostSpecs []HostExtended) map[string][]HostExtended {
	shardMap := map[string][]HostExtended{}
	for _, host := range hostSpecs {
		shardMap[host.ShardID.String] = append(shardMap[host.ShardID.String], host)
	}
	return shardMap
}
