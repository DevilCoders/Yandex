package sqlserver

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

// cloud/mdb/mdb-health/pkg/dbspecific/sqlserver/sqlserver.go > serviceSqlserver
const ServiceName = "sqlserver"

type hostRole string

const (
	hostRoleSQLServer = hostRole("sqlserver_cluster")
	hostRoleWitness   = hostRole("windows_witness")
	hostRoleUnknown   = hostRole("")
)

func parseHostRole(str string) hostRole {
	switch str {
	case string(hostRoleSQLServer):
		return hostRoleSQLServer
	case string(hostRoleWitness):
		return hostRoleWitness
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

// Planner plan rollout on sqlserver cluster
func Planner(cluster planner.Cluster) ([][]string, error) {
	if len(cluster.Hosts) == 0 {
		return nil, fmt.Errorf("got empty cluster: %+v", cluster)
	}

	// Split onto 2/3 groups
	// 1. Witness (if any)
	// 2. Replicas
	// 3. Master
	var master optional.String
	replicas := make([]string, 0)
	witness := make([]string, 0)

	for fqdn, host := range cluster.Hosts {
		service, ok := host.Services[ServiceName]
		if !ok {
			return nil, fmt.Errorf("no %q in %q host services: %+v", ServiceName, fqdn, host.Services)
		}
		hostRole, err := hostRoleFromMeta(host.Tags.Meta)
		if err != nil {
			return nil, err
		}
		switch hostRole {
		case hostRoleSQLServer:
			switch service.Role {
			case types.ServiceRoleMaster:
				if master.Valid {
					return nil, fmt.Errorf("find 2 masters in services. %q and %q. Last services: %+v", master.String, fqdn, host.Services)
				}
				master.Set(fqdn)
			case types.ServiceRoleReplica:
				replicas = append(replicas, fqdn)
			default:
				return nil, fmt.Errorf("unsupported service role: %q. On %q: %+v", service.Role, fqdn, service)
			}
		case hostRoleWitness:
			witness = append(witness, fqdn)
		default:
			return nil, fmt.Errorf("unsupported host role: %q. On %q: %+v", hostRole, fqdn, service)
		}

	}
	if !master.Valid {
		return nil, fmt.Errorf("master not found in hosts: %+v", cluster.Hosts)
	}

	if len(replicas)+len(witness) != len(cluster.Hosts)-1 {
		// right now it's unreachable code,
		// but better panics at that point.
		panic(
			fmt.Sprintf(
				"got itself into trouble: found %d replicas + %d witness, expected %d, hosts: %+v",
				len(replicas),
				len(witness),
				len(cluster.Hosts)-1,
				cluster.Hosts,
			),
		)
	}

	plan := [][]string{}
	plan = append(plan, planner.LinearPlan(witness, 3)...)
	plan = append(plan, planner.LinearPlan(replicas, 3)...)
	plan = append(plan, []string{master.String})
	return plan, nil
}
