package planner

import (
	"sort"

	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/health"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
)

// RolloutPlanner return plan in what cluster hosts should be updated.
// It takes cluster with its host and services info from mdb-health (e.g. role=master | replica ... etc)
type RolloutPlanner func(cluster Cluster) ([][]string, error)

// LinearPlan returns rollout plan for given fqdns.
// Each step increment parallelism unless maxSpeed reached
func LinearPlan(fqdns []string, maxSpeed int) [][]string {
	var ret [][]string
	speed := 1
	for {
		if len(fqdns) <= speed {
			if len(fqdns) > 0 {
				ret = append(ret, fqdns)
			}
			break
		}
		ret = append(ret, fqdns[:speed])
		fqdns = fqdns[speed:]
		if speed < maxSpeed {
			speed++
		}
	}
	return ret
}

// Default returns default rollout planner
// It start with one host.
// Maximum parallelism is 32
func Default(cluster Cluster) ([][]string, error) {
	fqdns := make([]string, 0, len(cluster.Hosts))
	for h := range cluster.Hosts {
		fqdns = append(fqdns, h)
	}
	sort.Strings(fqdns)

	return LinearPlan(fqdns, 32), nil
}

// NeedHostsHealth returns true whenever planner need hosts heath for then given cluster
func NeedHostsHealth(cluster models.Cluster) bool {
	if len(cluster.Hosts) == 1 {
		return false
	}
	return health.ManagedByHealth(cluster.Tags)
}
