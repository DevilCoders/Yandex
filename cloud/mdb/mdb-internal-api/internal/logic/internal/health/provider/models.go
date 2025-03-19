package provider

import (
	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/services"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/system"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func clusterHealthFromClient(health healthapi.ClusterStatus) (clusters.Health, error) {
	switch health {
	case healthapi.ClusterStatusUnknown:
		return clusters.HealthUnknown, nil
	case healthapi.ClusterStatusDead:
		return clusters.HealthDead, nil
	case healthapi.ClusterStatusDegraded:
		return clusters.HealthDegraded, nil
	case healthapi.ClusterStatusAlive:
		return clusters.HealthAlive, nil
	}

	return clusters.HealthUnknown, xerrors.Errorf("invalid cluster health from mdb-health: %s", health)
}

func hostStatusFromClient(health types.HostStatus) (hosts.Status, error) {
	switch health {
	case types.HostStatusUnknown:
		return hosts.StatusUnknown, nil
	case types.HostStatusDead:
		return hosts.StatusDead, nil
	case types.HostStatusDegraded:
		return hosts.StatusDegraded, nil
	case types.HostStatusAlive:
		return hosts.StatusAlive, nil
	}

	return hosts.StatusUnknown, xerrors.Errorf("invalid host status from mdb-health: %s", health)
}

func cpuMetricFromClient(cpu *types.CPUMetric) *system.CPUMetric {
	if cpu == nil {
		return nil
	}

	return &system.CPUMetric{
		BaseMetric: system.BaseMetric{Timestamp: cpu.Timestamp},
		Used:       cpu.Used,
	}
}

func memoryMetricFromClient(memory *types.MemoryMetric) *system.MemoryMetric {
	if memory == nil {
		return nil
	}

	return &system.MemoryMetric{
		BaseMetric: system.BaseMetric{Timestamp: memory.Timestamp},
		Used:       memory.Used,
		Total:      memory.Total,
	}
}

func diskMetricFromClient(disk *types.DiskMetric) *system.DiskMetric {
	if disk == nil {
		return nil
	}

	return &system.DiskMetric{
		BaseMetric: system.BaseMetric{Timestamp: disk.Timestamp},
		Used:       disk.Used,
		Total:      disk.Total,
	}
}

func hostSystemMetricsFromClient(sm *types.SystemMetrics) *system.Metrics {
	if sm == nil {
		return nil
	}

	return &system.Metrics{
		CPU:    cpuMetricFromClient(sm.CPU),
		Memory: memoryMetricFromClient(sm.Memory),
		Disk:   diskMetricFromClient(sm.Disk),
	}
}

func serviceStatusFromClient(status types.ServiceStatus) (services.Status, error) {
	switch status {
	case types.ServiceStatusUnknown:
		return services.StatusUnknown, nil
	case types.ServiceStatusDead:
		return services.StatusDead, nil
	case types.ServiceStatusAlive:
		return services.StatusAlive, nil
	}

	return services.StatusUnknown, xerrors.Errorf("invalid service status from mdb-health: %s", status)
}

func serviceRoleFromClient(role types.ServiceRole) (services.Role, error) {
	switch role {
	case types.ServiceRoleUnknown:
		return services.RoleUnknown, nil
	case types.ServiceRoleReplica:
		return services.RoleReplica, nil
	case types.ServiceRoleMaster:
		return services.RoleMaster, nil
	}

	return services.RoleUnknown, xerrors.Errorf("invalid service role from mdb-health: %s", role)
}

func serviceTypeFromClient(name string) (services.Type, error) {
	switch name {
	// TODO: typed service types
	case "sqlserver":
		return services.TypeSQLServer, nil
	case "clickhouse":
		return services.TypeClickHouse, nil
	case "zookeeper":
		return services.TypeZooKeeper, nil
	case "greenplum_master":
		return services.TypeGreenplumMasterServer, nil
	case "greenplum_segments":
		return services.TypeGreenplumSegmentServer, nil
	case "kafka":
		return services.TypeKafka, nil
	case "kafka_connect":
		return services.TypeKafkaConnect, nil
	case "witness":
		return services.TypeWindowsWitnessNode, nil
	case "redis":
		return services.TypeRedisNonsharded, nil
	case "redis_cluster":
		return services.TypeRedisSharded, nil
	case "sentinel":
		return services.TypeRedisSentinel, nil
	}

	// TODO: add other service types and return error
	return services.TypeUnknown, nil
}

func hostHealthFromClient(health types.HostHealth) (hosts.Health, error) {
	var h hosts.Health
	var err error
	h.Status, err = hostStatusFromClient(health.Status())
	h.System = hostSystemMetricsFromClient(health.System())
	if err != nil {
		return hosts.Health{}, err
	}

	h.Services = make([]services.Health, 0, len(health.Services()))
	for _, svc := range health.Services() {
		svcStatus, err := serviceStatusFromClient(svc.Status())
		if err != nil {
			return hosts.Health{}, err
		}

		svcRole, err := serviceRoleFromClient(svc.Role())
		if err != nil {
			return hosts.Health{}, err
		}

		svcType, err := serviceTypeFromClient(svc.Name())
		if err != nil {
			return hosts.Health{}, err
		}

		h.Services = append(h.Services, services.Health{
			Type:    svcType,
			Status:  svcStatus,
			Role:    svcRole,
			Metrics: svc.Metrics(),
		})
	}

	return h, nil
}
