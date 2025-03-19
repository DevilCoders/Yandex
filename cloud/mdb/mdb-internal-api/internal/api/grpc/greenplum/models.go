package greenplum

import (
	"context"
	"strings"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes"
	"google.golang.org/genproto/googleapis/type/timeofday"
	"google.golang.org/protobuf/types/known/wrapperspb"

	gpv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum/gpmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/services"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/system"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	mapStreamLogsServiceTypeToGRPC = map[logs.ServiceType]gpv1.StreamClusterLogsRequest_ServiceType{
		logs.ServiceTypeGreenplum:       gpv1.StreamClusterLogsRequest_GREENPLUM,
		logs.ServiceTypeGreenplumPooler: gpv1.StreamClusterLogsRequest_GREENPLUM_POOLER,
	}
	mapStreamLogsServiceTypeFromGRPC = reflectutil.ReverseMap(mapStreamLogsServiceTypeToGRPC).(map[gpv1.StreamClusterLogsRequest_ServiceType]logs.ServiceType)
)

func StreamLogsServiceTypeToGRPC(st logs.ServiceType) gpv1.StreamClusterLogsRequest_ServiceType {
	v, ok := mapStreamLogsServiceTypeToGRPC[st]
	if !ok {
		return gpv1.StreamClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
	}

	return v
}

func StreamLogsServiceTypeFromGRPC(st gpv1.StreamClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	v, ok := mapStreamLogsServiceTypeFromGRPC[st]
	if !ok {
		return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
	}

	return v, nil
}

var (
	mapClusterHealthToGRPC = map[clusters.Health]gpv1.Cluster_Health{
		clusters.HealthAlive:    gpv1.Cluster_ALIVE,
		clusters.HealthDegraded: gpv1.Cluster_DEGRADED,
		clusters.HealthDead:     gpv1.Cluster_DEAD,
		clusters.HealthUnknown:  gpv1.Cluster_HEALTH_UNKNOWN,
	}

	mapHostRoleToGRPC = map[hosts.Role]gpv1.Host_Type{
		hosts.RoleGreenplumMasterNode:  gpv1.Host_MASTER,
		hosts.RoleGreenplumSegmentNode: gpv1.Host_SEGMENT,
	}
	mapHostRoleFromGRPC = reflectutil.ReverseMap(mapHostRoleToGRPC).(map[gpv1.Host_Type]hosts.Role)
)

func HostRoleToGRPC(role hosts.Role) gpv1.Host_Type {
	v, ok := mapHostRoleToGRPC[role]
	if !ok {
		return gpv1.Host_TYPE_UNSPECIFIED
	}

	return v
}

func ClusterHealthToGRPC(env clusters.Health) gpv1.Cluster_Health {
	v, ok := mapClusterHealthToGRPC[env]
	if !ok {
		return gpv1.Cluster_HEALTH_UNKNOWN
	}
	return v
}

var (
	mapStatusToGRCP = map[clusters.Status]gpv1.Cluster_Status{
		clusters.StatusCreating:             gpv1.Cluster_CREATING,
		clusters.StatusCreateError:          gpv1.Cluster_ERROR,
		clusters.StatusRunning:              gpv1.Cluster_RUNNING,
		clusters.StatusModifying:            gpv1.Cluster_UPDATING,
		clusters.StatusModifyError:          gpv1.Cluster_ERROR,
		clusters.StatusStopping:             gpv1.Cluster_STOPPING,
		clusters.StatusStopped:              gpv1.Cluster_STOPPED,
		clusters.StatusStopError:            gpv1.Cluster_ERROR,
		clusters.StatusStarting:             gpv1.Cluster_STARTING,
		clusters.StatusStartError:           gpv1.Cluster_ERROR,
		clusters.StatusMaintainingOffline:   gpv1.Cluster_UPDATING,
		clusters.StatusMaintainOfflineError: gpv1.Cluster_ERROR,
	}
)

func StatusToGRPC(status clusters.Status) gpv1.Cluster_Status {
	v, ok := mapStatusToGRCP[status]
	if !ok {
		return gpv1.Cluster_STATUS_UNKNOWN
	}
	return v
}

var (
	mapLogStatementToGRPC = map[gpmodels.LogStatement]gpv1.LogStatement{
		gpmodels.LogStatementUnspecified: gpv1.LogStatement_LOG_STATEMENT_UNSPECIFIED,
		gpmodels.LogStatementNone:        gpv1.LogStatement_NONE,
		gpmodels.LogStatementDDL:         gpv1.LogStatement_DDL,
		gpmodels.LogStatementMod:         gpv1.LogStatement_MOD,
		gpmodels.LogStatementAll:         gpv1.LogStatement_ALL,
	}
	mapLogStatementFromGRPC = reflectutil.ReverseMap(mapLogStatementToGRPC).(map[gpv1.LogStatement]gpmodels.LogStatement)
)

func LogStatementToGRPC(ls gpmodels.LogStatement) gpv1.LogStatement {
	v, ok := mapLogStatementToGRPC[ls]
	if !ok {
		return gpv1.LogStatement_LOG_STATEMENT_UNSPECIFIED
	}

	return v
}

func LogStatementFromGRPC(ls gpv1.LogStatement) (gpmodels.LogStatement, error) {
	v, ok := mapLogStatementFromGRPC[ls]
	if !ok {
		return gpmodels.LogStatementUnspecified, semerr.InvalidInputf("Invalid enum value %d for LogStatement", ls)
	}

	return v, nil
}

func clusterDBConfigSpecFromGRPC(cfg *gpv1.ConfigSpec, ccfg *gpv1.GreenplumConfig, paths *grpcapi.FieldPaths) (gpmodels.ConfigSpec, error) {

	if cfg == nil {
		return gpmodels.ConfigSpec{}, nil
	}

	config := gpmodels.ConfigSpec{
		Config: gpmodels.ConfigBase{},
		Pool:   gpmodels.PoolerConfigBase{},
	}
	version := ccfg.GetVersion()

	switch gpcfg := cfg.GetGreenplumConfig(); gpcfg.(type) {
	case *gpv1.ConfigSpec_GreenplumConfig_6_17:
		cfgPaths := paths.Subtree("greenplumConfig_6_17.")
		config.Config.Version = "6.17"
		if !cfgPaths.Empty() {

			config.Config = gpmodels.ConfigBase{
				MaxConnections:               grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_17().GetMaxConnections()),
				MaxPreparedTransactions:      grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_17().GetMaxPreparedTransactions()),
				GPWorkfileLimitFilesPerQuery: grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_17().GetGpWorkfileLimitFilesPerQuery()),
				GPWorkfileLimitPerQuery:      grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_17().GetGpWorkfileLimitPerQuery()),
				GPWorkfileLimitPerSegment:    grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_17().GetGpWorkfileLimitPerSegment()),
				MaxSlotWalKeepSize:           grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_17().GetMaxSlotWalKeepSize()),
				GPWorkfileCompression:        grpcapi.OptionalBoolFromGRPC(cfg.GetGreenplumConfig_6_17().GetGpWorkfileCompression()),
			}
		}
	case *gpv1.ConfigSpec_GreenplumConfig_6_19:
		config.Config.Version = "6.19"
		cfgPaths := paths.Subtree("greenplumConfig_6_19.")
		if !cfgPaths.Empty() {
			logStatement, err := LogStatementFromGRPC(cfg.GetGreenplumConfig_6_19().GetLogStatement())
			if err != nil {
				return gpmodels.ConfigSpec{}, err
			}
			config.Config = gpmodels.ConfigBase{
				MaxConnections:               grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetMaxConnections()),
				MaxPreparedTransactions:      grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetMaxPreparedTransactions()),
				GPWorkfileLimitFilesPerQuery: grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetGpWorkfileLimitFilesPerQuery()),
				GPWorkfileLimitPerQuery:      grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetGpWorkfileLimitPerQuery()),
				GPWorkfileLimitPerSegment:    grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetGpWorkfileLimitPerSegment()),
				MaxSlotWalKeepSize:           grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetMaxSlotWalKeepSize()),
				GPWorkfileCompression:        grpcapi.OptionalBoolFromGRPC(cfg.GetGreenplumConfig_6_19().GetGpWorkfileCompression()),
				MaxStatementMem:              grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetMaxStatementMem()),
				LogStatement:                 logStatement,
			}
		}
	default:
		config.Config.Version = version
	}
	if cfg.GetPool() != nil {
		config.Pool = gpmodels.PoolerConfigBase{
			PoolSize:          grpcapi.OptionalInt64FromGRPC(cfg.Pool.GetSize()),
			ClientIdleTimeout: grpcapi.OptionalInt64FromGRPC(cfg.Pool.GetClientIdleTimeout())}

		if v := cfg.Pool.GetMode(); v != gpv1.ConnectionPoolerConfig_POOL_MODE_UNSPECIFIED {
			s := strings.ToLower(gpv1.ConnectionPoolerConfig_PoolMode_name[int32(v)])
			config.Pool.Mode = grpcapi.OptionalStringFromGRPC(s)
		}
	}

	return config, nil
}

func clusterConfigSpecFromGRPC(cfg *gpv1.GreenplumConfig, paths *grpcapi.FieldPaths,
	cfgMS *gpv1.MasterSubclusterConfigSpec, cfgSS *gpv1.SegmentSubclusterConfigSpec) (gpmodels.ClusterConfig, error) {

	if cfg == nil {
		return gpmodels.ClusterConfig{}, semerr.InvalidInput("Greenplum cluster config should be set")
	}

	if cfgMS == nil {
		return gpmodels.ClusterConfig{}, semerr.InvalidInput("Greenplum master config should be set")
	}

	if cfgSS == nil {
		return gpmodels.ClusterConfig{}, semerr.InvalidInput("Greenplum segment config should be set")
	}

	config := gpmodels.ClusterConfig{
		Version:        cfg.GetVersion(),
		ZoneID:         cfg.ZoneId,
		SubnetID:       cfg.SubnetId,
		AssignPublicIP: cfg.AssignPublicIp,

		MaxConnections: &gpmodels.IntMasterSegment{},
		LogStatement:   &gpmodels.StringMasterSegment{},
	}

	if cfg.SegmentMirroringEnable == nil {
		config.SegmentMirroringEnable = true
	} else {
		config.SegmentMirroringEnable = cfg.SegmentMirroringEnable.Value
	}

	if cfg.Access != nil {
		config.Access = &gpmodels.AccessSettings{
			WebSQL:       cfg.Access.WebSql,
			DataLens:     cfg.Access.DataLens,
			DataTransfer: cfg.Access.DataTransfer,
			Serverless:   cfg.Access.Serverless,
		}
	}

	if cfg.BackupWindowStart != nil {
		config.BackupWindowStart.Set(backups.BackupWindowStart{
			Hours:   int(cfg.BackupWindowStart.Hours),
			Minutes: int(cfg.BackupWindowStart.Minutes),
			Seconds: int(cfg.BackupWindowStart.Seconds),
			Nanos:   int(cfg.BackupWindowStart.Nanos),
		})
	}

	return config, nil
}

func clusterRestoreConfigSpecFromGRPC(cfg *gpv1.GreenplumRestoreConfig) (gpmodels.ClusterConfig, error) {
	if cfg == nil {
		return gpmodels.ClusterConfig{}, semerr.InvalidInput("Greenplum cluster restore config should be set")
	}

	config := gpmodels.ClusterConfig{
		ZoneID:         cfg.ZoneId,
		SubnetID:       cfg.SubnetId,
		AssignPublicIP: cfg.AssignPublicIp,
	}

	if cfg.Access != nil {
		config.Access = &gpmodels.AccessSettings{
			WebSQL:       cfg.Access.WebSql,
			DataLens:     cfg.Access.DataLens,
			DataTransfer: cfg.Access.DataTransfer,
			Serverless:   cfg.Access.Serverless,
		}
	}

	if cfg.BackupWindowStart != nil {
		config.BackupWindowStart.Set(backups.BackupWindowStart{
			Hours:   int(cfg.BackupWindowStart.Hours),
			Minutes: int(cfg.BackupWindowStart.Minutes),
			Seconds: int(cfg.BackupWindowStart.Seconds),
			Nanos:   int(cfg.BackupWindowStart.Nanos),
		})
	}

	return config, nil
}

func clusterResourcesGRPC(res *gpv1.Resources) (models.ClusterResources, error) {

	if res == nil {
		return models.ClusterResources{}, semerr.InvalidInput("Resources config should be set")
	}

	return models.ClusterResources{
		ResourcePresetExtID: res.ResourcePresetId,
		DiskSize:            res.DiskSize,
		DiskTypeExtID:       res.DiskTypeId,
	}, nil
}

func clusterMasterConfigSpecFromGRPC(cfg *gpv1.MasterSubclusterConfigSpec, paths *grpcapi.FieldPaths) (gpmodels.MasterSubclusterConfigSpec, error) {

	if cfg == nil {
		return gpmodels.MasterSubclusterConfigSpec{}, semerr.InvalidInput("Greenplum master config should be set")
	}

	config := gpmodels.MasterSubclusterConfigSpec{
		Resources: models.ClusterResources{
			ResourcePresetExtID: cfg.Resources.ResourcePresetId,
			DiskSize:            cfg.Resources.DiskSize,
			DiskTypeExtID:       cfg.Resources.DiskTypeId,
		},
	}

	return config, nil
}

func clusterSegmentConfigSpecFromGRPC(cfg *gpv1.SegmentSubclusterConfigSpec, paths *grpcapi.FieldPaths) (gpmodels.SegmentSubclusterConfigSpec, error) {

	if cfg == nil {
		return gpmodels.SegmentSubclusterConfigSpec{}, semerr.InvalidInput("Greenplum segment config should be set")
	}

	config := gpmodels.SegmentSubclusterConfigSpec{
		Resources: models.ClusterResources{
			ResourcePresetExtID: cfg.Resources.ResourcePresetId,
			DiskSize:            cfg.Resources.DiskSize,
			DiskTypeExtID:       cfg.Resources.DiskTypeId,
		},
	}

	return config, nil
}

func clusterConfigToGRPC(cluster gpmodels.Cluster) *gpv1.GreenplumConfig {
	clusterConfig := cluster.Config
	cc := &gpv1.GreenplumConfig{
		Version: clusterConfig.Version,
		BackupWindowStart: &timeofday.TimeOfDay{
			Hours:   int32(cluster.BackupSchedule.Start.Hours),
			Minutes: int32(cluster.BackupSchedule.Start.Minutes),
			Seconds: int32(cluster.BackupSchedule.Start.Seconds),
			Nanos:   int32(cluster.BackupSchedule.Start.Nanos),
		},
		ZoneId:                 clusterConfig.ZoneID,
		SubnetId:               clusterConfig.SubnetID,
		AssignPublicIp:         clusterConfig.AssignPublicIP,
		SegmentMirroringEnable: wrapperspb.Bool(clusterConfig.SegmentMirroringEnable),
	}
	cc.Version = clusterConfig.Version

	if clusterConfig.Access != nil {
		cc.Access = &gpv1.Access{
			DataLens:     clusterConfig.Access.DataLens,
			WebSql:       clusterConfig.Access.WebSQL,
			DataTransfer: clusterConfig.Access.DataTransfer,
			Serverless:   clusterConfig.Access.Serverless,
		}
	}

	return cc
}

func clusterConfigSubclusterMasterToGRPC(config gpmodels.MasterConfig, cc gpmodels.ClusterConfigSet) (*gpv1.MasterSubclusterConfig, error) {
	msc := &gpv1.MasterSubclusterConfig{
		Resources: &gpv1.Resources{
			ResourcePresetId: config.Resources.ResourcePresetExtID,
			DiskSize:         config.Resources.DiskSize,
			DiskTypeId:       config.Resources.DiskTypeExtID,
		},
	}

	return msc, nil
}

func clusterConfigSubclusterSegmentToGRPC(config gpmodels.SegmentConfig, cc gpmodels.ClusterConfigSet) (*gpv1.SegmentSubclusterConfig, error) {
	ssc := &gpv1.SegmentSubclusterConfig{
		Resources: &gpv1.Resources{
			ResourcePresetId: config.Resources.ResourcePresetExtID,
			DiskSize:         config.Resources.DiskSize,
			DiskTypeId:       config.Resources.DiskTypeExtID,
		},
	}

	return ssc, nil
}

func poolerToGrpc(config gpmodels.PoolerConfig) *gpv1.ConnectionPoolerConfig {
	Pool := &gpv1.ConnectionPoolerConfig{}

	if config.Mode != nil {
		Pool.Mode = gpv1.ConnectionPoolerConfig_PoolMode(gpv1.ConnectionPoolerConfig_PoolMode_value[strings.ToUpper(*config.Mode)])
	}

	if config.PoolSize != nil {
		Pool.Size = &wrapperspb.Int64Value{Value: *config.PoolSize}
	}

	if config.ClientIdleTimeout != nil {
		Pool.ClientIdleTimeout = &wrapperspb.Int64Value{Value: *config.ClientIdleTimeout}
	}
	return Pool
}

func poolerConfigSetToGRPC(pgs gpmodels.PoolerConfigSet) (*gpv1.ConnectionPoolerConfigSet, error) {

	msc := &gpv1.ConnectionPoolerConfigSet{
		EffectiveConfig: poolerToGrpc(pgs.EffectiveConfig),
		UserConfig:      poolerToGrpc(pgs.UserConfig),
		DefaultConfig:   poolerToGrpc(pgs.DefaultConfig),
	}

	return msc, nil
}

func clusterToGRPC(cluster gpmodels.Cluster, saltEnvMapper grpcapi.SaltEnvMapper) (*gpv1.Cluster, error) {
	masterConfig, err := clusterConfigSubclusterMasterToGRPC(cluster.MasterConfig, cluster.ClusterConfig)
	if err != nil {
		return nil, err
	}
	segmentConfig, err := clusterConfigSubclusterSegmentToGRPC(cluster.SegmentConfig, cluster.ClusterConfig)
	if err != nil {
		return nil, err
	}

	v := &gpv1.Cluster{
		Id:                 cluster.ClusterID,
		FolderId:           cluster.FolderExtID,
		CreatedAt:          grpcapi.TimeToGRPC(cluster.CreatedAt),
		Config:             clusterConfigToGRPC(cluster),
		MasterConfig:       masterConfig,
		SegmentConfig:      segmentConfig,
		ClusterConfig:      ClusterConfigSetToGrpc(cluster.ConfigSpec, cluster.Config.Version),
		Name:               cluster.Name,
		Description:        cluster.Description,
		Labels:             cluster.Labels,
		Environment:        gpv1.Cluster_Environment(saltEnvMapper.ToGRPC(cluster.Environment)),
		NetworkId:          cluster.NetworkID,
		SecurityGroupIds:   cluster.SecurityGroupIDs,
		MaintenanceWindow:  MaintenanceWindowToGRPC(cluster.MaintenanceInfo.Window),
		PlannedOperation:   MaintenanceOperationToGRPC(cluster.MaintenanceInfo.Operation),
		Health:             ClusterHealthToGRPC(cluster.Health),
		Status:             StatusToGRPC(cluster.Status),
		DeletionProtection: cluster.DeletionProtection,
		HostGroupIds:       cluster.HostGroupIDs,

		MasterHostCount:  cluster.MasterHostCount,
		SegmentHostCount: cluster.SegmentHostCount,
		SegmentInHost:    cluster.SegmentInHost,
		UserName:         cluster.UserName,
	}

	v.Monitoring = make([]*gpv1.Monitoring, 0, len(cluster.Monitoring.Charts))
	for _, chart := range cluster.Monitoring.Charts {
		mon := &gpv1.Monitoring{
			Name:        chart.Name,
			Description: chart.Description,
			Link:        chart.Link,
		}
		v.Monitoring = append(v.Monitoring, mon)
	}

	return v, nil
}

func clustersToGRPC(clusters []gpmodels.Cluster, saltEnvMapper grpcapi.SaltEnvMapper) ([]*gpv1.Cluster, error) {
	v := make([]*gpv1.Cluster, 0, len(clusters))
	for _, cluster := range clusters {
		cl, err := clusterToGRPC(cluster, saltEnvMapper)
		if err != nil {
			return nil, err
		}
		v = append(v, cl)
	}
	return v, nil
}

var (
	mapHostStatusToGRPC = map[hosts.Status]gpv1.Host_Health{
		hosts.StatusAlive:    gpv1.Host_ALIVE,
		hosts.StatusDead:     gpv1.Host_DEAD,
		hosts.StatusDegraded: gpv1.Host_DEGRADED,
		hosts.StatusUnknown:  gpv1.Host_UNKNOWN,
	}
)

func HostTypeToGRPC(h hosts.Health) gpv1.Host_Type {
	for _, svc := range h.Services {
		if svc.Type == services.TypeGreenplumMasterServer {
			switch svc.Role {
			case services.RoleMaster:
				return gpv1.Host_MASTER
			case services.RoleReplica:
				return gpv1.Host_REPLICA
			default:
				return gpv1.Host_TYPE_UNSPECIFIED
			}
		}
		if svc.Type == services.TypeGreenplumSegmentServer {
			return gpv1.Host_SEGMENT
		}
	}
	return gpv1.Host_TYPE_UNSPECIFIED
}

func HostStatusToGRPC(h hosts.Health) gpv1.Host_Health {
	v, ok := mapHostStatusToGRPC[h.Status]
	if !ok {
		return gpv1.Host_UNKNOWN
	}
	return v
}

func cpuMetricToGRPC(cpu *system.CPUMetric) *gpv1.Host_CPUMetric {
	if cpu == nil {
		return nil
	}

	return &gpv1.Host_CPUMetric{
		Timestamp: cpu.Timestamp,
		Used:      cpu.Used,
	}
}

func memoryMetricToGRPC(memory *system.MemoryMetric) *gpv1.Host_MemoryMetric {
	if memory == nil {
		return nil
	}

	return &gpv1.Host_MemoryMetric{
		Timestamp: memory.Timestamp,
		Used:      memory.Used,
		Total:     memory.Total,
	}
}

func diskMetricToGRPC(disk *system.DiskMetric) *gpv1.Host_DiskMetric {
	if disk == nil {
		return nil
	}

	return &gpv1.Host_DiskMetric{
		Timestamp: disk.Timestamp,
		Used:      disk.Used,
		Total:     disk.Total,
	}
}

func hostSystemMetricsToGRPC(sm *system.Metrics) *gpv1.Host_SystemMetrics {
	if sm == nil {
		return nil
	}

	return &gpv1.Host_SystemMetrics{
		Cpu:    cpuMetricToGRPC(sm.CPU),
		Memory: memoryMetricToGRPC(sm.Memory),
		Disk:   diskMetricToGRPC(sm.Disk),
	}
}

func hostToGRPC(host hosts.HostExtended) *gpv1.Host {
	h := &gpv1.Host{
		Name:      host.FQDN,
		ClusterId: host.ClusterID,
		ZoneId:    host.ZoneID,
		Type:      HostTypeToGRPC(host.Health),
		Resources: &gpv1.Resources{
			ResourcePresetId: host.ResourcePresetExtID,
			DiskSize:         host.SpaceLimit,
			DiskTypeId:       host.DiskTypeExtID,
		},
		Health:         HostStatusToGRPC(host.Health),
		System:         hostSystemMetricsToGRPC(host.Health.System),
		SubnetId:       host.SubnetID,
		AssignPublicIp: host.AssignPublicIP,
	}
	return h
}

func hostsToGRPC(hosts []hosts.HostExtended) []*gpv1.Host {
	v := make([]*gpv1.Host, 0, len(hosts))
	for _, host := range hosts {
		v = append(v, hostToGRPC(host))
	}
	return v
}

func operationToGRPC(ctx context.Context, op operations.Operation, gp greenplum.Greenplum, saltEnvMapper grpcapi.SaltEnvMapper, l log.Logger) (*operation.Operation, error) {
	opGrpc, err := grpcapi.OperationToGRPC(ctx, op, l)
	if err != nil {
		return nil, err
	}
	if op.Status != operations.StatusDone {
		return opGrpc, nil
	}

	switch op.MetaData.(type) {
	// Must be at first position before interfaces
	case gpmodels.MetadataDeleteCluster:
		return withEmptyResult(opGrpc)
	// TODO: remove type list
	case gpmodels.MetadataCreateCluster, gpmodels.MetadataModifyCluster,
		gpmodels.MetadataStartCluster, gpmodels.MetadataStopCluster,
		gpmodels.MetadataUpdateTLSCertsCluster, gpmodels.MetadataUpdateCluster:
		cluster, err := gp.Cluster(ctx, op.ClusterID)
		if err != nil {
			return opGrpc, nil
		}
		cl, err := clusterToGRPC(cluster, saltEnvMapper)
		if err != nil {
			return nil, err
		}
		return withResult(opGrpc, cl)
	}

	return opGrpc, nil
}

func withEmptyResult(opGrpc *operation.Operation) (*operation.Operation, error) {
	opGrpc.Result = &operation.Operation_Response{}
	return opGrpc, nil
}
func withResult(opGrpc *operation.Operation, message proto.Message) (*operation.Operation, error) {
	r, err := ptypes.MarshalAny(message)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal operation %+v response: %w", opGrpc, err)
	}
	opGrpc.Result = &operation.Operation_Response{Response: r}
	return opGrpc, nil
}

func toStringOpt(v *string) greenplum.OptString {
	if v == nil {
		return greenplum.OptString{IsSet: true}
	}
	return greenplum.OptString{IsSet: true, Value: v}
}

func addHostsArgsFromGRPC(request *gpv1.AddClusterHostsRequest) (greenplum.AddHostsClusterArgs, error) {
	args := greenplum.AddHostsClusterArgs{
		ClusterID:        request.GetClusterId(),
		MasterHostCount:  request.GetMasterHostCount(),
		SegmentHostCount: request.GetSegmentHostCount(),
	}

	return args, nil
}

func clusterModifyArgsFromGRPC(request *gpv1.UpdateClusterRequest) (greenplum.ModifyClusterArgs, error) {
	if request.GetClusterId() == "" {
		return greenplum.ModifyClusterArgs{}, semerr.InvalidInput("cluster id need to be set in cluster update request")
	}

	modifyCluster := greenplum.ModifyClusterArgs{ClusterID: request.GetClusterId()}
	paths := grpcapi.NewFieldPaths(request.UpdateMask.GetPaths())
	paths.Remove("cluster_id") // just ignore cluster_id

	if paths.Remove("name") {
		modifyCluster.Name.Set(request.GetName())
	}
	if paths.Remove("description") {
		modifyCluster.Description.Set(request.GetDescription())
	}
	if paths.Remove("labels") {
		modifyCluster.Labels.Set(request.GetLabels())
	}

	if paths.Remove("security_group_ids") {
		modifyCluster.SecurityGroupsIDs.Set(request.GetSecurityGroupIds())
	}
	if paths.Remove("deletion_protection") {
		modifyCluster.DeletionProtection.Set(request.GetDeletionProtection())
	}
	if paths.Remove("maintenance_window") {
		window, err := MaintenanceWindowFromGRPC(request.GetMaintenanceWindow())
		if err != nil {
			return greenplum.ModifyClusterArgs{}, err
		}
		modifyCluster.MaintenanceWindow.Set(window)
	}

	if paths.Remove("user_password") {
		password := request.GetUserPassword()
		if password != "" {
			modifyCluster.UserPassword.Set(password)
		}
	}

	if cfg := request.GetConfig(); cfg != nil {
		if cfg.BackupWindowStart != nil && paths.Remove("config.backup_window_start") {
			modifyCluster.BackupWindowStart.Set(backups.BackupWindowStart{
				Hours:   int(cfg.BackupWindowStart.Hours),
				Minutes: int(cfg.BackupWindowStart.Minutes),
				Seconds: int(cfg.BackupWindowStart.Seconds),
				Nanos:   int(cfg.BackupWindowStart.Nanos),
			})
		}

		if cfg.Access != nil && paths.Remove("config.access.data_lens") {
			modifyCluster.Access.DataLens = optional.NewBool(cfg.Access.DataLens)
		}
		if cfg.Access != nil && paths.Remove("config.access.web_sql") {
			modifyCluster.Access.WebSQL = optional.NewBool(cfg.Access.WebSql)
		}
		if cfg.Access != nil && paths.Remove("config.access.data_transfer") {
			modifyCluster.Access.DataTransfer = optional.NewBool(cfg.Access.DataTransfer)
		}
		if cfg.Access != nil && paths.Remove("config.access.serverless") {
			modifyCluster.Access.Serverless = optional.NewBool(cfg.Access.Serverless)
		}
	}

	if request.MasterConfig != nil && request.MasterConfig.Resources != nil {
		if paths.Remove("master_config.resources.resource_preset_id") {
			modifyCluster.MasterConfig.Resource.ResourcePresetExtID = toStringOpt(&request.MasterConfig.Resources.ResourcePresetId)
		}
		if paths.Remove("master_config.resources.disk_size") {
			modifyCluster.MasterConfig.Resource.DiskSize = greenplum.OptInt{IsSet: true, Value: &request.MasterConfig.Resources.DiskSize}
		}
	}

	if request.ConfigSpec != nil {
		cfg := request.ConfigSpec
		if cfg.GreenplumConfig != nil {
			switch gpcfg := cfg.GetGreenplumConfig(); gpcfg.(type) {
			case *gpv1.ConfigSpec_GreenplumConfig_6_17:
				if paths.Remove("config_spec.greenplum_config_6_17.max_connections") {
					modifyCluster.ConfigSpec.Config.MaxConnections = grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_17().GetMaxConnections())
				}
				if paths.Remove("config_spec.greenplum_config_6_17.max_prepared_transactions") {
					modifyCluster.ConfigSpec.Config.MaxPreparedTransactions = grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_17().GetMaxPreparedTransactions())
				}
				if paths.Remove("config_spec.greenplum_config_6_17.gp_workfile_limit_per_query") {
					modifyCluster.ConfigSpec.Config.GPWorkfileLimitPerQuery = grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_17().GetGpWorkfileLimitPerQuery())
				}
				if paths.Remove("config_spec.greenplum_config_6_17.gp_workfile_limit_files_per_query") {
					modifyCluster.ConfigSpec.Config.GPWorkfileLimitFilesPerQuery = grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_17().GetGpWorkfileLimitFilesPerQuery())
				}
				if paths.Remove("config_spec.greenplum_config_6_17.max_slot_wal_keep_size") {
					modifyCluster.ConfigSpec.Config.MaxSlotWalKeepSize = grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_17().GetMaxSlotWalKeepSize())
				}
				if paths.Remove("config_spec.greenplum_config_6_17.gp_workfile_limit_per_segment") {
					modifyCluster.ConfigSpec.Config.GPWorkfileLimitPerSegment = grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_17().GetGpWorkfileLimitPerSegment())
				}
				if paths.Remove("config_spec.greenplum_config_6_17.gp_workfile_compression") {
					modifyCluster.ConfigSpec.Config.GPWorkfileCompression = grpcapi.OptionalBoolFromGRPC(cfg.GetGreenplumConfig_6_17().GetGpWorkfileCompression())
				}
			case *gpv1.ConfigSpec_GreenplumConfig_6_19:
				if paths.Remove("config_spec.greenplum_config_6_19.max_connections") {
					modifyCluster.ConfigSpec.Config.MaxConnections = grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetMaxConnections())
				}
				if paths.Remove("config_spec.greenplum_config_6_19.max_prepared_transactions") {
					modifyCluster.ConfigSpec.Config.MaxPreparedTransactions = grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetMaxPreparedTransactions())
				}
				if paths.Remove("config_spec.greenplum_config_6_19.gp_workfile_limit_per_query") {
					modifyCluster.ConfigSpec.Config.GPWorkfileLimitPerQuery = grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetGpWorkfileLimitPerQuery())
				}
				if paths.Remove("config_spec.greenplum_config_6_19.gp_workfile_limit_files_per_query") {
					modifyCluster.ConfigSpec.Config.GPWorkfileLimitFilesPerQuery = grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetGpWorkfileLimitFilesPerQuery())
				}
				if paths.Remove("config_spec.greenplum_config_6_19.max_slot_wal_keep_size") {
					modifyCluster.ConfigSpec.Config.MaxSlotWalKeepSize = grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetMaxSlotWalKeepSize())
				}
				if paths.Remove("config_spec.greenplum_config_6_19.gp_workfile_limit_per_segment") {
					modifyCluster.ConfigSpec.Config.GPWorkfileLimitPerSegment = grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetGpWorkfileLimitPerSegment())
				}
				if paths.Remove("config_spec.greenplum_config_6_19.gp_workfile_compression") {
					modifyCluster.ConfigSpec.Config.GPWorkfileCompression = grpcapi.OptionalBoolFromGRPC(cfg.GetGreenplumConfig_6_19().GetGpWorkfileCompression())
				}
				if paths.Remove("config_spec.greenplum_config_6_19.max_statement_mem") {
					modifyCluster.ConfigSpec.Config.MaxStatementMem = grpcapi.OptionalInt64FromGRPC(cfg.GetGreenplumConfig_6_19().GetMaxStatementMem())
				}
				if paths.Remove("config_spec.greenplum_config_6_19.log_statement") {
					logStatement, err := LogStatementFromGRPC(cfg.GetGreenplumConfig_6_19().GetLogStatement())
					if err != nil {
						return greenplum.ModifyClusterArgs{}, err
					}
					modifyCluster.ConfigSpec.Config.LogStatement = logStatement
				}
			default:
				//
			}

		}

		if cfg.Pool != nil {
			if paths.Remove("config_spec.pool.mode") {
				var val *string
				if v := cfg.Pool.GetMode(); v != gpv1.ConnectionPoolerConfig_POOL_MODE_UNSPECIFIED {
					s := strings.ToLower(gpv1.ConnectionPoolerConfig_PoolMode_name[int32(v)])
					val = &s
				}
				modifyCluster.ConfigSpec.Pool.Mode = grpcapi.OptionalStringFromGRPC(*val)
			}
			if paths.Remove("config_spec.pool.size") {
				modifyCluster.ConfigSpec.Pool.PoolSize = grpcapi.OptionalInt64FromGRPC(cfg.Pool.GetSize())
			}
			if paths.Remove("config_spec.pool.client_idle_timeout") {
				modifyCluster.ConfigSpec.Pool.ClientIdleTimeout = grpcapi.OptionalInt64FromGRPC(cfg.Pool.GetClientIdleTimeout())
			}

		}
	}

	if request.SegmentConfig != nil && request.SegmentConfig.Resources != nil {
		if paths.Remove("segment_config.resources.resource_preset_id") {
			modifyCluster.SegmentConfig.Resource.ResourcePresetExtID = toStringOpt(&request.SegmentConfig.Resources.ResourcePresetId)
		}
		if paths.Remove("segment_config.resources.disk_size") {
			modifyCluster.SegmentConfig.Resource.DiskSize = greenplum.OptInt{IsSet: true, Value: &request.SegmentConfig.Resources.DiskSize}
		}
	}

	err := paths.MustBeEmpty()
	if err != nil {
		return greenplum.ModifyClusterArgs{}, err
	}

	return modifyCluster, nil
}

var (
	mapMaintenanceWindowDayStringToGRPC = map[string]gpv1.WeeklyMaintenanceWindow_WeekDay{
		"MON": gpv1.WeeklyMaintenanceWindow_MON,
		"TUE": gpv1.WeeklyMaintenanceWindow_TUE,
		"WED": gpv1.WeeklyMaintenanceWindow_WED,
		"THU": gpv1.WeeklyMaintenanceWindow_THU,
		"FRI": gpv1.WeeklyMaintenanceWindow_FRI,
		"SAT": gpv1.WeeklyMaintenanceWindow_SAT,
		"SUN": gpv1.WeeklyMaintenanceWindow_SUN,
	}
	mapMaintenanceWindowDayStringFromGRPC = reflectutil.ReverseMap(mapMaintenanceWindowDayStringToGRPC).(map[gpv1.WeeklyMaintenanceWindow_WeekDay]string)
)

func MaintenanceWindowToGRPC(window clusters.MaintenanceWindow) *gpv1.MaintenanceWindow {
	if window.Anytime() {
		return &gpv1.MaintenanceWindow{
			Policy: &gpv1.MaintenanceWindow_Anytime{
				Anytime: &gpv1.AnytimeMaintenanceWindow{},
			},
		}
	}

	day, found := mapMaintenanceWindowDayStringToGRPC[window.Day]
	if !found {
		day = gpv1.WeeklyMaintenanceWindow_WEEK_DAY_UNSPECIFIED
	}

	return &gpv1.MaintenanceWindow{
		Policy: &gpv1.MaintenanceWindow_WeeklyMaintenanceWindow{
			WeeklyMaintenanceWindow: &gpv1.WeeklyMaintenanceWindow{
				Day:  day,
				Hour: int64(window.Hour),
			},
		},
	}
}

func MaintenanceWindowFromGRPC(window *gpv1.MaintenanceWindow) (clusters.MaintenanceWindow, error) {
	if window == nil {
		return clusters.NewAnytimeMaintenanceWindow(), nil
	}

	switch p := window.GetPolicy().(type) {
	case *gpv1.MaintenanceWindow_Anytime:
		return clusters.NewAnytimeMaintenanceWindow(), nil
	case *gpv1.MaintenanceWindow_WeeklyMaintenanceWindow:
		day, ok := mapMaintenanceWindowDayStringFromGRPC[p.WeeklyMaintenanceWindow.GetDay()]
		if !ok {
			return clusters.MaintenanceWindow{}, semerr.InvalidInputf("invalid maintenance window data: %q", p.WeeklyMaintenanceWindow.GetDay().String())
		}

		return clusters.NewWeeklyMaintenanceWindow(day, int(p.WeeklyMaintenanceWindow.GetHour())), nil
	default:
		return clusters.MaintenanceWindow{}, semerr.InvalidInputf("invalid maintenance window type")
	}
}

func MaintenanceOperationToGRPC(op clusters.MaintenanceOperation) *gpv1.MaintenanceOperation {
	if !op.Valid() {
		return nil
	}

	return &gpv1.MaintenanceOperation{
		Info:                      op.Info,
		DelayedUntil:              grpcapi.TimeToGRPC(op.DelayedUntil),
		LatestMaintenanceTime:     grpcapi.TimeToGRPC(op.LatestMaintenanceTime),
		NextMaintenanceWindowTime: grpcapi.TimePtrToGRPC(op.NearestMaintenanceWindow),
	}
}

var (
	mapRescheduleTypeToGRPC = map[clusters.RescheduleType]gpv1.RescheduleMaintenanceRequest_RescheduleType{
		clusters.RescheduleTypeUnspecified:         gpv1.RescheduleMaintenanceRequest_RESCHEDULE_TYPE_UNSPECIFIED,
		clusters.RescheduleTypeImmediate:           gpv1.RescheduleMaintenanceRequest_IMMEDIATE,
		clusters.RescheduleTypeNextAvailableWindow: gpv1.RescheduleMaintenanceRequest_NEXT_AVAILABLE_WINDOW,
		clusters.RescheduleTypeSpecificTime:        gpv1.RescheduleMaintenanceRequest_SPECIFIC_TIME,
	}
	mapRescheduleTypeFromGRPC = reflectutil.ReverseMap(mapRescheduleTypeToGRPC).(map[gpv1.RescheduleMaintenanceRequest_RescheduleType]clusters.RescheduleType)
)

func RescheduleTypeToGRPC(rt clusters.RescheduleType) gpv1.RescheduleMaintenanceRequest_RescheduleType {
	v, ok := mapRescheduleTypeToGRPC[rt]
	if !ok {
		return gpv1.RescheduleMaintenanceRequest_RESCHEDULE_TYPE_UNSPECIFIED
	}

	return v
}

func RescheduleTypeFromGRPC(rt gpv1.RescheduleMaintenanceRequest_RescheduleType) (clusters.RescheduleType, error) {
	v, ok := mapRescheduleTypeFromGRPC[rt]
	if !ok {
		return clusters.RescheduleTypeUnspecified, semerr.InvalidInput("unknown reschedule type")
	}

	return v, nil
}

func BackupToGRPC(backup backups.Backup) *gpv1.Backup {
	return &gpv1.Backup{
		Id:              backup.GlobalBackupID(),
		FolderId:        backup.FolderID,
		SourceClusterId: backup.SourceClusterID,
		Size:            backup.Size,
		CreatedAt:       grpcapi.TimeToGRPC(backup.CreatedAt),
		StartedAt:       grpcapi.OptionalTimeToGRPC(backup.StartedAt),
	}
}

func BackupsToGRPC(backups []backups.Backup) []*gpv1.Backup {
	v := make([]*gpv1.Backup, 0, len(backups))
	for _, b := range backups {
		v = append(v, BackupToGRPC(b))
	}
	return v
}

func Config6_17ToGRPC(config gpmodels.ConfigBase) *gpv1.GreenplumConfig6_17 {
	result := gpv1.GreenplumConfig6_17{
		GpWorkfileCompression:        grpcapi.OptionalBoolToGRPC(config.GPWorkfileCompression),
		MaxConnections:               grpcapi.OptionalInt64ToGRPC(config.MaxConnections),
		MaxPreparedTransactions:      grpcapi.OptionalInt64ToGRPC(config.MaxPreparedTransactions),
		GpWorkfileLimitFilesPerQuery: grpcapi.OptionalInt64ToGRPC(config.GPWorkfileLimitFilesPerQuery),
		MaxSlotWalKeepSize:           grpcapi.OptionalInt64ToGRPC(config.MaxSlotWalKeepSize),
		GpWorkfileLimitPerQuery:      grpcapi.OptionalInt64ToGRPC(config.GPWorkfileLimitPerQuery),
		GpWorkfileLimitPerSegment:    grpcapi.OptionalInt64ToGRPC(config.GPWorkfileLimitPerSegment),
	}
	return &result
}

func Config6_17SetToGRPC(configSet gpmodels.ClusterGPDBConfigSet) *gpv1.ClusterConfigSet_GreenplumConfigSet_6_17 {
	return &gpv1.ClusterConfigSet_GreenplumConfigSet_6_17{
		GreenplumConfigSet_6_17: &gpv1.GreenplumConfigSet6_17{
			EffectiveConfig: Config6_17ToGRPC(configSet.EffectiveConfig),
			DefaultConfig:   Config6_17ToGRPC(configSet.DefaultConfig),
			UserConfig:      Config6_17ToGRPC(configSet.UserConfig),
		},
	}
}

func Config6_19ToGRPC(config gpmodels.ConfigBase) *gpv1.GreenplumConfig6_19 {
	result := gpv1.GreenplumConfig6_19{
		GpWorkfileCompression:        grpcapi.OptionalBoolToGRPC(config.GPWorkfileCompression),
		MaxConnections:               grpcapi.OptionalInt64ToGRPC(config.MaxConnections),
		MaxPreparedTransactions:      grpcapi.OptionalInt64ToGRPC(config.MaxPreparedTransactions),
		GpWorkfileLimitFilesPerQuery: grpcapi.OptionalInt64ToGRPC(config.GPWorkfileLimitFilesPerQuery),
		MaxSlotWalKeepSize:           grpcapi.OptionalInt64ToGRPC(config.MaxSlotWalKeepSize),
		GpWorkfileLimitPerQuery:      grpcapi.OptionalInt64ToGRPC(config.GPWorkfileLimitPerQuery),
		GpWorkfileLimitPerSegment:    grpcapi.OptionalInt64ToGRPC(config.GPWorkfileLimitPerSegment),
		MaxStatementMem:              grpcapi.OptionalInt64ToGRPC(config.MaxStatementMem),
		LogStatement:                 LogStatementToGRPC(config.LogStatement),
	}
	return &result
}

func Config6_19SetToGRPC(configSet gpmodels.ClusterGPDBConfigSet) *gpv1.ClusterConfigSet_GreenplumConfigSet_6_19 {
	return &gpv1.ClusterConfigSet_GreenplumConfigSet_6_19{
		GreenplumConfigSet_6_19: &gpv1.GreenplumConfigSet6_19{
			EffectiveConfig: Config6_19ToGRPC(configSet.EffectiveConfig),
			DefaultConfig:   Config6_19ToGRPC(configSet.DefaultConfig),
			UserConfig:      Config6_19ToGRPC(configSet.UserConfig),
		},
	}
}

func ClusterConfigSetToGrpc(ccs gpmodels.ClusterGpConfigSet, version string) *gpv1.ClusterConfigSet {
	pool, _ := poolerConfigSetToGRPC(ccs.Pooler)
	switch version {
	case "6.17":
		cfg := Config6_17SetToGRPC(ccs.Config)
		return &gpv1.ClusterConfigSet{Pool: pool, GreenplumConfig: cfg}
	case "6.19":
		cfg := Config6_19SetToGRPC(ccs.Config)
		return &gpv1.ClusterConfigSet{Pool: pool, GreenplumConfig: cfg}
	default:
		cfg := Config6_17SetToGRPC(ccs.Config)
		return &gpv1.ClusterConfigSet{Pool: pool, GreenplumConfig: cfg}
	}

}
