package clusters

import (
	"encoding/json"
	"fmt"
	"sort"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Pillar interface {
	Validate() error
}

type Cluster struct {
	clustermodels.Cluster
	pillar json.RawMessage
}

func NewClusterModel(c clustermodels.Cluster, p json.RawMessage) Cluster {
	return Cluster{Cluster: c, pillar: p}
}

func (c Cluster) Pillar(p pillars.Marshaler) error {
	return p.UnmarshalPillar(c.pillar)
}

type SkipValidations struct {
	MaxHosts           bool
	DiskSize           bool
	DecommissionedZone bool
}

type HostGroup struct {
	Role                       hosts.Role
	CurrentResourcePresetExtID optional.String
	NewResourcePresetExtID     optional.String
	DiskTypeExtID              string
	CurrentDiskSize            optional.Int64
	NewDiskSize                optional.Int64
	HostsToAdd                 ZoneHostsList
	HostsCurrent               ZoneHostsList
	HostsToRemove              ZoneHostsList
	SkipValidations            SkipValidations
	ShardName                  optional.String
}

func (hg *HostGroup) ValidateAndFix() error {
	if hg.Role == hosts.RoleUnknown {
		return xerrors.New("host role not set")
	}

	if hg.DiskTypeExtID == "" {
		return xerrors.New("disk type not set")
	}

	if len(hg.HostsToAdd)+len(hg.HostsCurrent) == 0 {
		return xerrors.New("both current and new hosts are missing")
	}

	if !hg.CurrentResourcePresetExtID.Valid && !hg.NewResourcePresetExtID.Valid {
		return xerrors.New("both current and new resource presets are missing")
	}

	if !hg.CurrentResourcePresetExtID.Valid && len(hg.HostsToRemove) != 0 {
		return xerrors.New("cannot remove hosts without current resource preset")
	}

	if !hg.CurrentDiskSize.Valid && !hg.NewDiskSize.Valid {
		return xerrors.New("both current and new disk size are missing")
	}

	if !hg.CurrentDiskSize.Valid && len(hg.HostsToRemove) != 0 {
		return xerrors.New("cannot remove hosts without current disk size")
	}

	if hg.CurrentResourcePresetExtID.Valid != hg.CurrentDiskSize.Valid {
		return xerrors.New("both current resource preset and disk size must either be present or missing")
	}

	// Fix those invalid states that are fixable
	if hg.IsResourcePresetChanged() {
		if hg.NewResourcePresetExtID.String == hg.CurrentResourcePresetExtID.String {
			hg.NewResourcePresetExtID = optional.String{}
		}
	}

	if hg.IsDiskSizeChanged() {
		if hg.NewDiskSize.Int64 == hg.CurrentDiskSize.Int64 {
			hg.NewDiskSize = optional.Int64{}
		}
	}

	return nil
}

func (hg HostGroup) HasChanges() bool {
	return hg.NewResourcePresetExtID.Valid ||
		hg.NewDiskSize.Valid ||
		len(hg.HostsToAdd) != 0 ||
		len(hg.HostsToRemove) != 0
}

func (hg HostGroup) IsResourcePresetChanged() bool {
	return hg.NewResourcePresetExtID.Valid && hg.CurrentResourcePresetExtID.Valid
}

func (hg HostGroup) IsDiskSizeChanged() bool {
	return hg.NewDiskSize.Valid && hg.CurrentDiskSize.Valid
}

func (hg HostGroup) TargetResourcePresetExtID() string {
	if hg.NewResourcePresetExtID.Valid {
		return hg.NewResourcePresetExtID.String
	}

	return hg.CurrentResourcePresetExtID.String
}

func (hg HostGroup) TargetDiskSize() int64 {
	if hg.NewDiskSize.Valid {
		return hg.NewDiskSize.Int64
	}

	return hg.CurrentDiskSize.Int64
}

// TargetHostsCount is the number of hosts after addition and/or removal
func (hg HostGroup) TargetHostsCount() int64 {
	return hg.HostsCurrent.Len() - hg.HostsToRemove.Len() + hg.HostsToAdd.Len()
}

type ZoneHosts struct {
	ZoneID string
	Count  int64
}

type ZoneHostsList []ZoneHosts

func (l ZoneHostsList) Len() int64 {
	var count int64
	for _, z := range l {
		count += z.Count
	}
	return count
}

func (l ZoneHostsList) ToMap() map[string]int64 {
	zoneMap := map[string]int64{}

	for _, zoneHosts := range l {
		if count, found := zoneMap[zoneHosts.ZoneID]; found {
			zoneMap[zoneHosts.ZoneID] = count + zoneHosts.Count
		} else {
			zoneMap[zoneHosts.ZoneID] = zoneHosts.Count
		}
	}

	return zoneMap
}

func (ZoneHostsList) fromMap(m map[string]int64) ZoneHostsList {
	var res ZoneHostsList

	for zoneID, count := range m {
		res = append(res, ZoneHosts{zoneID, count})
	}

	sort.Slice(res, func(i, j int) bool {
		return res[i].ZoneID < res[j].ZoneID
	})

	return res
}

func (l ZoneHostsList) Add(zhl ZoneHostsList) ZoneHostsList {
	zoneMap := l.ToMap()

	for _, zoneHosts := range zhl {
		if count, found := zoneMap[zoneHosts.ZoneID]; found {
			zoneMap[zoneHosts.ZoneID] = count + zoneHosts.Count
		} else {
			zoneMap[zoneHosts.ZoneID] = zoneHosts.Count
		}
	}

	return ZoneHostsList{}.fromMap(zoneMap)
}

func (l ZoneHostsList) Sub(zhl ZoneHostsList) (ZoneHostsList, error) {
	zoneMap := l.ToMap()

	for _, zoneHosts := range zhl {
		count, found := zoneMap[zoneHosts.ZoneID]
		if !found {
			return nil, xerrors.Errorf("missing key %q while subtracting zone hosts lists: %v - %v", zoneHosts.ZoneID, l, zhl)
		}

		diff := count - zoneHosts.Count
		if diff < 0 {
			return nil, xerrors.Errorf("count of hosts to delete is larger than count of current hosts: %d-%d=%d", count, zoneHosts.Count, diff)
		}

		if diff == 0 {
			delete(zoneMap, zoneHosts.ZoneID)
		} else {
			zoneMap[zoneHosts.ZoneID] = diff
		}
	}

	return ZoneHostsList{}.fromMap(zoneMap), nil
}

func ZoneHostsListFromHosts(allHosts []hosts.HostExtended) ZoneHostsList {
	zoneHostsByZone := make(map[string]*ZoneHosts)
	for _, host := range allHosts {
		zoneHosts, ok := zoneHostsByZone[host.ZoneID]
		if !ok {
			zoneHosts = &ZoneHosts{ZoneID: host.ZoneID}
			zoneHostsByZone[host.ZoneID] = zoneHosts
		}
		zoneHosts.Count += 1
	}
	zoneHostsList := make([]ZoneHosts, 0, len(zoneHostsByZone))
	for _, zoneHosts := range zoneHostsByZone {
		zoneHostsList = append(zoneHostsList, *zoneHosts)
	}
	return zoneHostsList
}

type ResolvedHostGroup struct {
	HostGroup
	CurrentResourcePreset resources.Preset
	NewResourcePreset     resources.Preset
}

func (rhg ResolvedHostGroup) IsUpscale() bool {
	if !rhg.IsResourcePresetChanged() {
		return false
	}

	return rhg.NewResourcePreset.IsUpscaleOf(rhg.CurrentResourcePreset)
}

func (rhg ResolvedHostGroup) IsDownscale() bool {
	if !rhg.IsResourcePresetChanged() {
		return false
	}

	return rhg.NewResourcePreset.IsDownscaleOf(rhg.CurrentResourcePreset)
}

func (rhg ResolvedHostGroup) TargetResourcePreset() resources.Preset {
	if rhg.NewResourcePresetExtID.Valid {
		return rhg.NewResourcePreset
	}

	return rhg.CurrentResourcePreset
}

type ResolvedHostGroups struct {
	byHostRole map[hosts.Role][]ResolvedHostGroup
}

func NewResolvedHostGroups(rhgs []ResolvedHostGroup) ResolvedHostGroups {
	res := ResolvedHostGroups{
		byHostRole: make(map[hosts.Role][]ResolvedHostGroup),
	}

	for _, rhg := range rhgs {
		res.byHostRole[rhg.Role] = append(res.byHostRole[rhg.Role], rhg)
	}

	return res
}

func (rhg ResolvedHostGroups) ByHostRole(role hosts.Role) (ResolvedHostGroup, bool) {
	// TODO: when byShardID or something like that is added, this func might need to panic in some cases
	res, ok := rhg.byHostRole[role]

	if len(res) > 1 {
		panic(fmt.Sprintf("resolved host groups have multiple host groups with role %q", role))
	}

	if len(res) == 0 {
		return ResolvedHostGroup{}, false
	}

	return res[0], ok
}

func (rhg ResolvedHostGroups) MustByHostRole(role hosts.Role) ResolvedHostGroup {
	res, ok := rhg.ByHostRole(role)
	if !ok {
		panic(fmt.Sprintf("resolved host groups do not have a host group with role %q", role))
	}

	return res
}

func (rhg ResolvedHostGroups) GroupsByHostRole(role hosts.Role) ([]ResolvedHostGroup, bool) {
	res, ok := rhg.byHostRole[role]

	return res, ok
}

func (rhg ResolvedHostGroups) MustGroupsByHostRole(role hosts.Role) []ResolvedHostGroup {
	res, ok := rhg.GroupsByHostRole(role)
	if !ok {
		panic(fmt.Sprintf("resolved host groups do not have a host group with role %q", role))
	}

	return res
}

// Single returns the only resolved host group preset.
// Panics if more or less than one is present.
func (rhg ResolvedHostGroups) Single() ResolvedHostGroup {
	if len(rhg.byHostRole) != 1 {
		panic(fmt.Sprintf("expected single resolved host group by have %+v", rhg.byHostRole))
	}

	for _, v := range rhg.byHostRole {
		if len(v) != 1 {
			panic(fmt.Sprintf("expected single resolved host group by have %+v", rhg.byHostRole))
		}

		return v[0]
	}

	panic("could not have happened!")
}

// SingleWithChanges returns the only changed host group preset.
// Panics if more or less than one is present.
func (rhg ResolvedHostGroups) SingleWithChanges() ResolvedHostGroup {
	if len(rhg.byHostRole) < 1 {
		panic(fmt.Sprintf("expected single resolved host group but have %+v", rhg.byHostRole))
	}

	var result *ResolvedHostGroup
	for role, v := range rhg.byHostRole {
		for i, g := range v {
			if g.HasChanges() {
				if result != nil {
					panic(fmt.Sprintf("expected single changed host group but have %+v", rhg.byHostRole))
				}

				result = &rhg.byHostRole[role][i]
			}
		}
	}

	if result == nil {
		panic(fmt.Sprintf("expected single changed host group but have %+v", rhg.byHostRole))
	}

	return *result
}

func (rhg ResolvedHostGroups) MustMapByShardName(role hosts.Role) map[string]ResolvedHostGroup {
	groups, ok := rhg.GroupsByHostRole(role)
	if !ok {
		panic(fmt.Sprintf("resolved host groups do not have a host group with role %q", role))
	}

	res := map[string]ResolvedHostGroup{}
	for _, g := range groups {
		if !g.ShardName.Valid {
			panic(fmt.Sprintf("some host groups shard name are not set %+v", rhg.byHostRole))
		}

		res[g.ShardName.String] = g
	}

	return res
}

func (rhg ResolvedHostGroups) ExtraTimeout() time.Duration {
	result := time.Duration(0)
	for _, groups := range rhg.byHostRole {
		for _, group := range groups {
			if group.HasChanges() {
				// expect 1 GB to be transferred in 60 seconds
				result += time.Duration(group.TargetDiskSize()/(1<<30)) * time.Minute
			}

			result += time.Minute * time.Duration(15*(group.HostsToAdd.Len()+group.HostsToRemove.Len()))
		}
	}

	return result
}

func GetHostsWithRole(allHosts []hosts.HostExtended, role hosts.Role) []hosts.HostExtended {
	hostsWithRole := make([]hosts.HostExtended, 0, len(allHosts))
	for _, host := range allHosts {
		hasRole := false
		for _, r := range host.Roles {
			if r == role {
				hasRole = true
			}
		}
		if hasRole {
			hostsWithRole = append(hostsWithRole, host)
		}
	}
	return hostsWithRole
}

type HostBillingSpec struct {
	HostRole         hosts.Role
	ClusterResources models.ClusterResources
	AssignPublicIP   bool
	OnDedicatedHost  bool
}

func HostExtendedByFQDN(allHosts []hosts.HostExtended, fqdn string) (hosts.HostExtended, error) {
	for _, h := range allHosts {
		if h.FQDN == fqdn {
			return h, nil
		}
	}

	return hosts.HostExtended{}, semerr.NotFoundf("host %q does not exist", fqdn)
}
