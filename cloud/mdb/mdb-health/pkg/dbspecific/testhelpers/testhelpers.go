package testhelpers

import (
	"fmt"
	"time"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types/testhelpers"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
)

// RoleService is map of list services for role
type RoleService map[string][]string

// ServiceHealth is map of list services to it health
type ServiceHealth map[string]types.ServiceStatus

// RoleServiceHealth is map of list services and it health for role
type RoleServiceHealth map[string]ServiceHealth

// RoleStatus is map of role to used status
type RoleStatus map[string]types.ClusterStatus

// TestRole base struct for test and expectations
type TestRole struct {
	Input    RoleStatus
	Expected types.ClusterStatus
}

type TestRWMode struct {
	Name       string
	HealthMode dbspecific.ModeMap
	Result     types.DBRWInfo
}

// TestService base struct for test and expectations
type TestService struct {
	Input      RoleServiceHealth
	Expected   types.ClusterStatus
	ExpectHost types.HostStatus
}

type TestGroupRWMode struct {
	Cases  []TestRWMode
	Roles  dbspecific.HostsMap
	Shards dbspecific.HostsMap
}

// GenerateOneHostWithAllServices generate required info for test
func GenerateOneHostWithAllServices(input RoleServiceHealth) (string, []string, dbspecific.HostsMap, dbspecific.HealthMap) {
	cid := generateString("cid")
	var fqdns []string
	roles := make(dbspecific.HostsMap)
	health := make(dbspecific.HealthMap)
	generateHostForEachService := func(role string, serviceHealth map[string]types.ServiceStatus) {
		fqdn := generateString("fqdn")
		roles[role] = append(roles[role], fqdn)
		fqdns = append(fqdns, fqdn)
		for service, status := range serviceHealth {
			health[fqdn] = append(health[fqdn], createServiceHealth(service, status))
		}
	}
	for role, serviceHealth := range input {
		generateHostForEachService(role, serviceHealth)
	}
	return cid, fqdns, roles, health
}

// GenerateOneHostsForRoleStatus generate required info for test
func GenerateOneHostsForRoleStatus(toService RoleService, input RoleStatus) (string, []string, dbspecific.HostsMap, dbspecific.HealthMap) {
	cid := generateString("cid")
	var fqdns []string
	roles := make(dbspecific.HostsMap)
	health := make(dbspecific.HealthMap)
	generateHost := func(role string, status types.ServiceStatus, serviceList []string) {
		fqdn := generateString("fqdn")
		roles[role] = append(roles[role], fqdn)
		fqdns = append(fqdns, fqdn)
		for _, service := range serviceList {
			health[fqdn] = append(health[fqdn], createServiceHealth(service, status))
		}
	}
	for role, serviceList := range toService {
		rs, ok := input[role]
		if !ok {
			continue
		}
		switch rs {
		case types.ClusterStatusDead:
			generateHost(role, types.ServiceStatusDead, serviceList)
		case types.ClusterStatusUnknown:
			generateHost(role, types.ServiceStatusUnknown, serviceList)
		case types.ClusterStatusAlive:
			generateHost(role, types.ServiceStatusAlive, serviceList)
		case types.ClusterStatusDegraded:
			generateHost(role, types.ServiceStatusAlive, serviceList)
			generateHost(role, types.ServiceStatusUnknown, serviceList)
		}
	}
	return cid, fqdns, roles, health
}

func generateString(prefix string) string {
	return fmt.Sprintf("%s-%s", prefix, uuid.Must(uuid.NewV4()).String())
}

func createServiceHealth(name string, status types.ServiceStatus) types.ServiceHealth {
	metrics := testhelpers.NewMetrics(0)
	return types.NewServiceHealth(name, time.Now(), status, types.ServiceRoleMaster, types.ServiceReplicaTypeUnknown, "", 0, metrics)
}
