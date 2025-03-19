package dbspecific

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
)

// HostsMap map of some attribute -> list of fqdn
type HostsMap map[string][]string

// HealthMap map of host -> list of service health
type HealthMap map[string][]types.ServiceHealth

type ModeMap map[string]types.Mode

// Extractor DB specific extractor from service health
type Extractor interface {
	EvaluateClusterHealth(cid string, fqdns []string, roles HostsMap, health HealthMap) (types.ClusterHealth, error)
	EvaluateHostHealth(services []types.ServiceHealth) (types.HostStatus, time.Time)
	EvaluateDBInfo(cid, status string, roles, shards HostsMap, health ModeMap, services map[string][]types.ServiceHealth) (types.DBRWInfo, error)
	GetSLAGuaranties(cluster datastore.ClusterTopology) (bool, map[string]bool)
	EvaluateGeoAtWarning(roles, _, geos HostsMap, health ModeMap) []string
}

// Base algorithm to collect roles by fqdn
func CollectRoles(fqdn string, roles HostsMap) []string {
	usedRoles := make([]string, 0, 1)
	for role, hosts := range roles {
		for _, h := range hosts {
			if h == fqdn {
				usedRoles = append(usedRoles, role)
				break
			}
		}
	}
	return usedRoles
}

// Base algorithm to evaluate alive hosts, may be should move to Extractor in feature
func CountAliveHosts(extractor Extractor, hosts map[string]struct{}, health HealthMap) (int, time.Time) {
	alive := 0
	var mints time.Time
	for host := range hosts {
		services, ok := health[host]
		if !ok {
			continue
		}
		status, ts := extractor.EvaluateHostHealth(services)
		if status == types.HostStatusAlive {
			alive++
		}
		mints = MinTS(mints, ts)
	}
	return alive, mints
}

func getServiceHealth(listServices []types.ServiceHealth, serviceName string) (types.ServiceHealth, bool) {
	for _, service := range listServices {
		if service.Name() == serviceName {
			return service, true
		}
	}
	return types.NewUnknownServiceHealth(), false
}

// StatusAggregate return common worse cluster status
func StatusAggregate(lhs, rhs types.ClusterStatus) types.ClusterStatus {
	if lhs == types.ClusterStatusAlive && rhs == types.ClusterStatusAlive {
		return types.ClusterStatusAlive
	}
	if lhs == types.ClusterStatusDead || rhs == types.ClusterStatusDead {
		return types.ClusterStatusDead
	}
	if lhs == types.ClusterStatusUnknown || rhs == types.ClusterStatusUnknown {
		return types.ClusterStatusUnknown
	}
	return types.ClusterStatusDegraded
}

// CollectStatus common code for aggregate status by hosts list
func CollectStatus(hosts []string, health HealthMap, services []string) (types.ClusterStatus, time.Time) {
	return CollectStatusOneOf(hosts, health, [][]string{services})
}

// CollectStatusOneOf check host on one of services from list
func CollectStatusOneOf(hosts []string, health HealthMap, listServices [][]string) (types.ClusterStatus, time.Time) {
	if len(listServices) == 0 {
		return types.ClusterStatusUnknown, time.Time{}
	}
	var agg statusAggregator
	for _, host := range hosts {
		hostServices, ok := health[host]
		if !ok || len(hostServices) == 0 {
			agg.AddHost(types.HostStatusUnknown, time.Time{})
			continue
		}
		services := MatchServices(hostServices, listServices)
		hs, ts := EvalHostHealthStatus(hostServices, services)
		agg.AddHost(hs, ts)
	}
	if agg.AllUnknown() {
		return types.ClusterStatusUnknown, agg.ts
	}
	if agg.AllDead() {
		return types.ClusterStatusDead, agg.ts
	}
	if agg.AllAlive() {
		return types.ClusterStatusAlive, agg.ts
	}
	return types.ClusterStatusDegraded, agg.ts
}

// MatchServices match corresponding service for host
func MatchServices(hostServices []types.ServiceHealth, listServices [][]string) []string {
	if len(listServices) == 1 {
		return listServices[0]
	}
	match := make([]int, len(listServices))
	for i, s := range listServices {
		for _, n := range s {
			_, ok := getServiceHealth(hostServices, n)
			if ok {
				match[i]++
			}
		}
	}
	maxInd := 0
	maxMatch := 0
	for i, m := range match {
		if m > maxMatch {
			maxMatch = m
			maxInd = i
		}
	}
	return listServices[maxInd]
}

// EvalHostHealthStatus evaluate host service by known list of services
func EvalHostHealthStatus(hostServices []types.ServiceHealth, services []string) (types.HostStatus, time.Time) {
	var agg statusAggregator
	for _, serviceName := range services {
		health, _ := getServiceHealth(hostServices, serviceName)
		agg.AddService(health.Status(), health.Timestamp())
	}
	if agg.HasDead() {
		return types.HostStatusDead, agg.ts
	}
	if agg.AllAlive() {
		return types.HostStatusAlive, agg.ts
	}
	if agg.AllDegraded() {
		return types.HostStatusDegraded, agg.ts
	}
	return types.HostStatusUnknown, agg.ts
}

// statusAggregator ...
type statusAggregator struct {
	ts       time.Time
	unknown  int
	alive    int
	dead     int
	degraded int
	total    int
}

// AddService ...
func (sa *statusAggregator) AddService(ss types.ServiceStatus, ts time.Time) {
	sa.total++
	switch ss {
	case types.ServiceStatusAlive:
		sa.alive++
	case types.ServiceStatusDead:
		sa.dead++
	case types.ServiceStatusUnknown:
		sa.unknown++
	}
	if sa.ts.IsZero() || !ts.IsZero() && ts.Before(sa.ts) {
		sa.ts = ts
	}
}

// AddHost ...
func (sa *statusAggregator) AddHost(ss types.HostStatus, ts time.Time) {
	sa.total++
	switch ss {
	case types.HostStatusAlive:
		sa.alive++
	case types.HostStatusDead:
		sa.dead++
	case types.HostStatusDegraded:
		sa.degraded++
	case types.HostStatusUnknown:
		sa.unknown++
	}
	if sa.ts.IsZero() || !ts.IsZero() && ts.Before(sa.ts) {
		sa.ts = ts
	}
}

// AllDegraded ...
func (sa *statusAggregator) AllDegraded() bool {
	return sa.alive == 0 && sa.dead == 0 && sa.unknown == 0 && sa.degraded > 0
}

// AllUnknown ...
func (sa *statusAggregator) AllUnknown() bool {
	return sa.alive == 0 && sa.dead == 0 && sa.unknown > 0 && sa.degraded == 0
}

// HasAlive ...
func (sa *statusAggregator) HasAlive() bool {
	return sa.alive > 0
}

// HasDead ...
func (sa *statusAggregator) HasDead() bool {
	return sa.dead > 0
}

// MoreThanHalfAlive ...
func (sa *statusAggregator) MoreThanHalfAlive() bool {
	return sa.alive > sa.total/2
}

// AllAlive ...
func (sa *statusAggregator) AllAlive() bool {
	return sa.alive > 0 && sa.dead == 0 && sa.unknown == 0 && sa.degraded == 0
}

// AllDead ...
func (sa *statusAggregator) AllDead() bool {
	return sa.alive == 0 && sa.dead > 0 && sa.unknown == 0 && sa.degraded == 0
}

// MinTS return minimal timestamp
func MinTS(timestamps ...time.Time) time.Time {
	var min time.Time
	for _, ts := range timestamps {
		if ts.IsZero() {
			continue
		}
		if min.IsZero() || ts.Before(min) {
			min = ts
		}
	}
	return min
}

// returns number of read, write and total number of hosts respectively

func CalculateRWHosts(fqdns []string, health ModeMap) types.DBRWInfo {
	read := 0
	write := 0
	total := len(fqdns)

	for _, fqdn := range fqdns {
		hi, ok := health[fqdn]
		if !ok {
			continue
		}
		if hi.Read {
			read++
		}
		if hi.Write {
			write++
		}
	}

	return types.DBRWInfo{
		HostsWrite: write,
		HostsRead:  read,
		HostsTotal: total,
	}
}

// CalculateRWInfo return aggregated rw info
func CalculateRWInfo(dbRole string, roles, shards HostsMap, health ModeMap, mapBroken map[string]struct{}) types.DBRWInfo {
	var info types.DBRWInfo
	if len(shards) > 0 {
		for sid, fqdns := range shards {
			infoCurrent := CalculateRWHosts(fqdns, health)

			if _, ok := mapBroken[sid]; !ok {
				if infoCurrent.HostsRead > 0 {
					infoCurrent.DBRead++
				}
				if infoCurrent.HostsWrite > 0 {
					infoCurrent.DBWrite++
				}
			}

			info = info.Add(infoCurrent)
		}
		return info
	}

	fqdns, ok := roles[dbRole]
	if !ok {
		return info
	}

	info = info.Add(CalculateRWHosts(fqdns, health))

	if _, ok := mapBroken[dbRole]; ok {
		return info
	}

	if info.HostsRead > 0 {
		info.DBRead++
	}
	if info.HostsWrite > 0 {
		info.DBWrite++
	}
	return info
}

// GeoBit return bit for specified zone or 0 if unknown
func GeoBit(geo string) uint {

	switch geo {
	case "iva":
		return 0x01
	case "myt":
		return 0x02
	case "sas":
		return 0x04
	case "man":
		return 0x08
	case "vla":
		return 0x10
	case "ru-central1-a":
		return 0x1000
	case "ru-central1-b":
		return 0x2000
	case "ru-central1-c":
		return 0x4000
	default:
		return 0
	}
}

type LimitFunc func(i int) int

var DefaultLimitFunc LimitFunc = func(i int) int {
	return i / 2
}

type CalculateUserBrokenFunc func(dbRole string, roles, shards HostsMap, health ModeMap, limitFunc LimitFunc) (types.DBRWInfo, map[string]struct{})

// CalculateBrokenInfoCustom return aggregated broken info
func CalculateBrokenInfoCustom(dbRole string, roles, shards HostsMap, health ModeMap, limitFunc LimitFunc) (types.DBRWInfo, map[string]struct{}) {
	calc := CalculateBrokenInfoCustomWithStatusFilter(metadb.ClusterStatusRunning)
	return calc(dbRole, roles, shards, health, limitFunc)
}

func CalculateBrokenInfoCustomWithStatusFilter(clusterStatus string) CalculateUserBrokenFunc {
	isStatusTracked := false
	for _, status := range types.TrackedSLACLusterStatuses {
		if status == clusterStatus {
			isStatusTracked = true
		}
	}
	return func(dbRole string, roles, shards HostsMap, health ModeMap, limitFunc LimitFunc) (types.DBRWInfo, map[string]struct{}) {

		var info types.DBRWInfo
		mapBroken := make(map[string]struct{})

		if len(shards) > 0 {
			for sid, fqdns := range shards {
				var brokenByUser int
				var brokenByServiceOnRead int
				var readAvailableCnt int
				var writeAvailableCnt int

				for _, fqdn := range fqdns {
					hi, ok := health[fqdn]
					if !ok {
						continue
					}

					if hi.Read {
						readAvailableCnt++
					}
					if hi.Write {
						writeAvailableCnt++
					}

					if hi.UserFaultBroken || !isStatusTracked {
						brokenByUser++
					} else if !hi.Read {
						brokenByServiceOnRead++
					}
				}

				info.HostsBrokenByUser += brokenByUser
				// Mark shard as broken by user only if it is really broken
				// that`s to @Cthulho!
				if writeAvailableCnt == 0 || readAvailableCnt <= limitFunc(len(fqdns)) {
					if brokenByUser > limitFunc(len(fqdns)) && brokenByServiceOnRead == 0 {
						info.DBBroken++
						mapBroken[sid] = struct{}{}
					}
				}
			}
			return info, mapBroken
		}

		fqdns, ok := roles[dbRole]
		if !ok {
			return info, mapBroken
		}

		var brokenByUser int
		var brokenByServiceOnRead int
		for _, fqdn := range fqdns {
			hi, ok := health[fqdn]
			if !ok {
				continue
			}
			if hi.UserFaultBroken || !isStatusTracked {
				brokenByUser++
			} else if !hi.Read {
				brokenByServiceOnRead++
			}
		}

		info.HostsBrokenByUser += brokenByUser

		if brokenByUser > limitFunc(len(fqdns)) && brokenByServiceOnRead == 0 {
			mapBroken[dbRole] = struct{}{}
			info.DBBroken++
		}

		return info, mapBroken
	}
}

func CalcDBInfoCustom(role string, roles, shards HostsMap, health ModeMap, limitFunc LimitFunc, calculateUserBrokenFunc CalculateUserBrokenFunc) types.DBRWInfo {
	/*
			in order for monitoring to work consistently,
			each cluster or host must be counted 1 time broken by the user or (0/1) times as read / write
			but total must not change in both cases
			Example: total 6 cluster with 5 ok read and 5 ok write, but 1 is broken
			we want to get 5/5 or write clusters and calculate this number like that
		    ok_read = read / (total - broken)
			ok_write = write / (total - broken)
			therefore information about broken clusters should not contain read or write marks
	*/

	infoBroken, mapBroken := calculateUserBrokenFunc(role, roles, shards, health, limitFunc)
	info := CalculateRWInfo(role, roles, shards, health, mapBroken)

	if len(shards) > 0 {
		info.DBTotal = len(shards)
	} else {
		info.DBTotal = 1
	}

	info.DBBroken += infoBroken.DBBroken
	info.HostsBrokenByUser += infoBroken.HostsBrokenByUser

	return info
}
