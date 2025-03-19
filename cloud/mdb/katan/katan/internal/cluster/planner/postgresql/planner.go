package postgresql

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

// cloud/mdb/mdb-health/pkg/dbspecific/postgresql/postgresql.go > serviceReplication
const ReplicationService = "pg_replication"

// Planner plan rollout on postgresql cluster
func Planner(cluster planner.Cluster) ([][]string, error) {
	if len(cluster.Hosts) == 0 {
		return nil, fmt.Errorf("got empty cluster: %+v", cluster)
	}

	// Split onto 2 groups
	// 1. Replicas
	// 2. Master
	var master optional.String
	replicas := make([]string, 0)

	for fqdn, host := range cluster.Hosts {
		repl, ok := host.Services[ReplicationService]
		if !ok {
			return nil, fmt.Errorf("no %q in %q host services: %+v", ReplicationService, fqdn, host.Services)
		}

		switch repl.Role {
		case types.ServiceRoleMaster:
			if master.Valid {
				return nil, fmt.Errorf("find 2 masters in services. %q and %q. Last services: %+v", master.String, fqdn, host.Services)
			}
			master.Set(fqdn)
		case types.ServiceRoleReplica:
			switch repl.ReplicaType {
			case types.ServiceReplicaTypeQuorum, types.ServiceReplicaTypeSync:
				replicas = append(replicas, fqdn)
			default:
				replicas = append([]string{fqdn}, replicas...)
			}

		default:
			return nil, fmt.Errorf("unsupported service role: %q. On %q: %+v", repl.Role, fqdn, repl)
		}
	}
	if !master.Valid {
		return nil, fmt.Errorf("master not found in hosts: %+v", cluster.Hosts)
	}

	if len(replicas) != len(cluster.Hosts)-1 {
		// right now it's unreachable code,
		// but better panics at that point.
		panic(
			fmt.Sprintf(
				"got itself into trouble: found %d replicas, expected %d, hosts: %+v",
				len(replicas),
				len(cluster.Hosts)-1,
				cluster.Hosts,
			),
		)
	}

	plan := planner.LinearPlan(replicas, 3)
	plan = append(plan, []string{master.String})

	return plan, nil
}
