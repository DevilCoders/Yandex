package redis

import (
	redisv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/redis/v1"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis/rmodels"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/services"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/system"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
)

var (
	mapListLogsServiceTypeToGRPC = map[logs.ServiceType]redisv1.ListClusterLogsRequest_ServiceType{
		logs.ServiceTypeRedis: redisv1.ListClusterLogsRequest_REDIS,
	}
	mapListLogsServiceTypeFromGRPC = reflectutil.ReverseMap(mapListLogsServiceTypeToGRPC).(map[redisv1.ListClusterLogsRequest_ServiceType]logs.ServiceType)
)

func ListLogsServiceTypeToGRPC(st logs.ServiceType) redisv1.ListClusterLogsRequest_ServiceType {
	v, ok := mapListLogsServiceTypeToGRPC[st]
	if !ok {
		return redisv1.ListClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
	}

	return v
}

func ListLogsServiceTypeFromGRPC(st redisv1.ListClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	v, ok := mapListLogsServiceTypeFromGRPC[st]
	if !ok {
		return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
	}

	return v, nil
}

var (
	mapStreamLogsServiceTypeToGRPC = map[logs.ServiceType]redisv1.StreamClusterLogsRequest_ServiceType{
		logs.ServiceTypeRedis: redisv1.StreamClusterLogsRequest_REDIS,
	}
	mapStreamLogsServiceTypeFromGRPC = reflectutil.ReverseMap(mapStreamLogsServiceTypeToGRPC).(map[redisv1.StreamClusterLogsRequest_ServiceType]logs.ServiceType)
)

func StreamLogsServiceTypeToGRPC(st logs.ServiceType) redisv1.StreamClusterLogsRequest_ServiceType {
	v, ok := mapStreamLogsServiceTypeToGRPC[st]
	if !ok {
		return redisv1.StreamClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
	}

	return v
}

func StreamLogsServiceTypeFromGRPC(st redisv1.StreamClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	v, ok := mapStreamLogsServiceTypeFromGRPC[st]
	if !ok {
		return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
	}

	return v, nil
}

func BackupToGRPC(backup bmodels.Backup) *redisv1.Backup {
	return &redisv1.Backup{
		Id:               backup.GlobalBackupID(),
		FolderId:         backup.FolderID,
		SourceClusterId:  backup.SourceClusterID,
		SourceShardNames: backup.SourceShardNames,
		CreatedAt:        grpc.TimeToGRPC(backup.CreatedAt),
		StartedAt:        grpc.OptionalTimeToGRPC(backup.StartedAt),
	}
}

func BackupsToGRPC(backups []bmodels.Backup) []*redisv1.Backup {
	v := make([]*redisv1.Backup, 0, len(backups))
	for _, b := range backups {
		v = append(v, BackupToGRPC(b))
	}
	return v
}

var (
	hostHealthToGRPC = map[hosts.Status]redisv1.Host_Health{
		hosts.StatusAlive:    redisv1.Host_ALIVE,
		hosts.StatusDegraded: redisv1.Host_DEGRADED,
		hosts.StatusDead:     redisv1.Host_DEAD,
		hosts.StatusUnknown:  redisv1.Host_HEALTH_UNKNOWN,
	}
)

func HostHealthToGRPC(hh hosts.Health) redisv1.Host_Health {
	v, ok := hostHealthToGRPC[hh.Status]
	if !ok {
		return redisv1.Host_HEALTH_UNKNOWN
	}
	return v
}

var (
	serviceTypeMap = map[services.Type]redisv1.Service_Type{
		services.TypeRedisNonsharded: redisv1.Service_REDIS,
		services.TypeRedisSharded:    redisv1.Service_REDIS_CLUSTER,
		services.TypeRedisSentinel:   redisv1.Service_ARBITER,
		services.TypeUnknown:         redisv1.Service_TYPE_UNSPECIFIED,
	}
)

func serviceTypeToGRPC(t services.Type) redisv1.Service_Type {
	v, ok := serviceTypeMap[t]
	if !ok {
		return redisv1.Service_TYPE_UNSPECIFIED
	}
	return v
}

var (
	serviceHealthMap = map[services.Status]redisv1.Service_Health{
		services.StatusAlive:   redisv1.Service_ALIVE,
		services.StatusDead:    redisv1.Service_DEAD,
		services.StatusUnknown: redisv1.Service_HEALTH_UNKNOWN,
	}
)

func serviceHealthToGRPC(s services.Status) redisv1.Service_Health {
	v, ok := serviceHealthMap[s]
	if !ok {
		return redisv1.Service_HEALTH_UNKNOWN
	}
	return v
}

func hostServicesToGRPC(hh hosts.Health) []*redisv1.Service {
	var result []*redisv1.Service
	for _, service := range hh.Services {
		result = append(result, &redisv1.Service{
			Type:   serviceTypeToGRPC(service.Type),
			Health: serviceHealthToGRPC(service.Status),
		})
	}

	return result
}

var (
	serviceRoleMap = map[services.Role]redisv1.Host_Role{
		services.RoleMaster:  redisv1.Host_MASTER,
		services.RoleReplica: redisv1.Host_REPLICA,
		services.RoleUnknown: redisv1.Host_ROLE_UNKNOWN,
	}
)

func HostRoleToGRPC(hh hosts.Health) redisv1.Host_Role {
	result := redisv1.Host_ROLE_UNKNOWN
	for _, service := range hh.Services {
		if service.Type == services.TypeRedisSharded || service.Type == services.TypeRedisNonsharded {
			v, ok := serviceRoleMap[service.Role]
			if ok {
				result = v
			}
		}
	}

	return result
}

func cpuMetricToGRPC(cpu *system.CPUMetric) *redisv1.Host_CPUMetric {
	if cpu == nil {
		return nil
	}

	return &redisv1.Host_CPUMetric{
		Timestamp: cpu.Timestamp,
		Used:      cpu.Used,
	}
}

func memoryMetricToGRPC(memory *system.MemoryMetric) *redisv1.Host_MemoryMetric {
	if memory == nil {
		return nil
	}

	return &redisv1.Host_MemoryMetric{
		Timestamp: memory.Timestamp,
		Used:      memory.Used,
		Total:     memory.Total,
	}
}

func diskMetricToGRPC(disk *system.DiskMetric) *redisv1.Host_DiskMetric {
	if disk == nil {
		return nil
	}

	return &redisv1.Host_DiskMetric{
		Timestamp: disk.Timestamp,
		Used:      disk.Used,
		Total:     disk.Total,
	}
}

func HostSystemMetricsToGRPC(sm *system.Metrics) *redisv1.Host_SystemMetrics {
	if sm == nil {
		return nil
	}

	return &redisv1.Host_SystemMetrics{
		Cpu:    cpuMetricToGRPC(sm.CPU),
		Memory: memoryMetricToGRPC(sm.Memory),
		Disk:   diskMetricToGRPC(sm.Disk),
	}
}

func HostToGRPC(host hosts.HostExtended) *redisv1.Host {
	h := &redisv1.Host{
		AssignPublicIp:  host.AssignPublicIP,
		ClusterId:       host.ClusterID,
		Health:          HostHealthToGRPC(host.Health),
		Name:            host.FQDN,
		ReplicaPriority: grpc.OptionalInt64ToGRPC(host.ReplicaPriority),
		Resources: &redisv1.Resources{
			DiskSize:         host.SpaceLimit,
			DiskTypeId:       host.DiskTypeExtID,
			ResourcePresetId: host.ResourcePresetExtID,
		},
		Role:      HostRoleToGRPC(host.Health),
		Services:  hostServicesToGRPC(host.Health),
		ShardName: host.ShardID.String,
		SubnetId:  host.SubnetID,
		System:    HostSystemMetricsToGRPC(host.Health.System),
		ZoneId:    host.ZoneID,
	}
	return h
}

func HostsToGRPC(hosts []hosts.HostExtended) []*redisv1.Host {
	var v []*redisv1.Host
	for _, host := range hosts {
		v = append(v, HostToGRPC(host))
	}
	return v
}

func HostSpecsFromGRPC(specs []*redisv1.HostSpec) ([]rmodels.HostSpec, error) {
	var result []rmodels.HostSpec
	for _, s := range specs {
		var spec = rmodels.HostSpec{
			AssignPublicIP:  s.AssignPublicIp,
			ShardName:       s.ShardName,
			SubnetID:        s.SubnetId,
			ZoneID:          s.ZoneId,
			ReplicaPriority: grpc.OptionalInt64FromGRPC(s.ReplicaPriority),
		}
		result = append(result, spec)
	}
	return result, nil
}
