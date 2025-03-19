package clickhouse

import (
	"context"
	"sort"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes"
	"github.com/golang/protobuf/ptypes/wrappers"
	fieldmaskutils "github.com/mennanov/fieldmask-utils"
	"google.golang.org/protobuf/types/known/wrapperspb"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	config "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1/config"
	chv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1/console"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/services"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/system"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/monitoring"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	modelsoptional "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/optional"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	mapListLogsServiceTypeToGRPC = map[logs.ServiceType]chv1.ListClusterLogsRequest_ServiceType{
		logs.ServiceTypeClickHouse: chv1.ListClusterLogsRequest_CLICKHOUSE,
	}
	mapListLogsServiceTypeFromGRPC = reflectutil.ReverseMap(mapListLogsServiceTypeToGRPC).(map[chv1.ListClusterLogsRequest_ServiceType]logs.ServiceType)
)

func ListLogsServiceTypeToGRPC(st logs.ServiceType) chv1.ListClusterLogsRequest_ServiceType {
	v, ok := mapListLogsServiceTypeToGRPC[st]
	if !ok {
		return chv1.ListClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
	}

	return v
}

func ListLogsServiceTypeFromGRPC(st chv1.ListClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	v, ok := mapListLogsServiceTypeFromGRPC[st]
	if !ok {
		return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
	}

	return v, nil
}

var (
	mapStreamLogsServiceTypeToGRPC = map[logs.ServiceType]chv1.StreamClusterLogsRequest_ServiceType{
		logs.ServiceTypeClickHouse: chv1.StreamClusterLogsRequest_CLICKHOUSE,
	}
	mapStreamLogsServiceTypeFromGRPC = reflectutil.ReverseMap(mapStreamLogsServiceTypeToGRPC).(map[chv1.StreamClusterLogsRequest_ServiceType]logs.ServiceType)
)

func StreamLogsServiceTypeToGRPC(st logs.ServiceType) chv1.StreamClusterLogsRequest_ServiceType {
	v, ok := mapStreamLogsServiceTypeToGRPC[st]
	if !ok {
		return chv1.StreamClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
	}

	return v
}

func StreamLogsServiceTypeFromGRPC(st chv1.StreamClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	v, ok := mapStreamLogsServiceTypeFromGRPC[st]
	if !ok {
		return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
	}

	return v, nil
}

var (
	mapOverflowModeToGRPC = map[chmodels.OverflowMode]chv1.UserSettings_OverflowMode{
		chmodels.OverflowModeUnspecified: chv1.UserSettings_OVERFLOW_MODE_UNSPECIFIED,
		chmodels.OverflowModeThrow:       chv1.UserSettings_OVERFLOW_MODE_THROW,
		chmodels.OverflowModeBreak:       chv1.UserSettings_OVERFLOW_MODE_BREAK,
	}
	mapOverflowModeFromGRPC = reflectutil.ReverseMap(mapOverflowModeToGRPC).(map[chv1.UserSettings_OverflowMode]chmodels.OverflowMode)
)

func OverflowModeToGRPC(mode chmodels.OverflowMode) chv1.UserSettings_OverflowMode {
	v, ok := mapOverflowModeToGRPC[mode]
	if !ok {
		return chv1.UserSettings_OVERFLOW_MODE_UNSPECIFIED
	}

	return v
}

func OverflowModeFromGRPC(mode chv1.UserSettings_OverflowMode) (chmodels.OverflowMode, error) {
	v, ok := mapOverflowModeFromGRPC[mode]
	if !ok {
		return chmodels.OverflowModeUnspecified, semerr.InvalidInput("unknown overflow mode")
	}

	return v, nil
}

var (
	mapGroupByOverflowModeToGRPC = map[chmodels.GroupByOverflowMode]chv1.UserSettings_GroupByOverflowMode{
		chmodels.GroupByOverflowModeUnspecified: chv1.UserSettings_GROUP_BY_OVERFLOW_MODE_UNSPECIFIED,
		chmodels.GroupByOverflowModeThrow:       chv1.UserSettings_GROUP_BY_OVERFLOW_MODE_THROW,
		chmodels.GroupByOverflowModeBreak:       chv1.UserSettings_GROUP_BY_OVERFLOW_MODE_BREAK,
		chmodels.GroupByOverflowModeAny:         chv1.UserSettings_GROUP_BY_OVERFLOW_MODE_ANY,
	}
	mapGroupByOverflowModeFromGRPC = reflectutil.ReverseMap(mapGroupByOverflowModeToGRPC).(map[chv1.UserSettings_GroupByOverflowMode]chmodels.GroupByOverflowMode)
)

func GroupByOverflowModeToGRPC(mode chmodels.GroupByOverflowMode) chv1.UserSettings_GroupByOverflowMode {
	v, ok := mapGroupByOverflowModeToGRPC[mode]
	if !ok {
		return chv1.UserSettings_GROUP_BY_OVERFLOW_MODE_UNSPECIFIED
	}

	return v
}

func GroupByOverflowModeFromGRPC(mode chv1.UserSettings_GroupByOverflowMode) (chmodels.GroupByOverflowMode, error) {
	v, ok := mapGroupByOverflowModeFromGRPC[mode]
	if !ok {
		return chmodels.GroupByOverflowModeUnspecified, semerr.InvalidInput("unknown groupby overflow mode")
	}

	return v, nil
}

var (
	mapDistributedProductModeToGRPC = map[chmodels.DistributedProductMode]chv1.UserSettings_DistributedProductMode{
		chmodels.DistributedProductModeUnspecified: chv1.UserSettings_DISTRIBUTED_PRODUCT_MODE_UNSPECIFIED,
		chmodels.DistributedProductModeDeny:        chv1.UserSettings_DISTRIBUTED_PRODUCT_MODE_DENY,
		chmodels.DistributedProductModeLocal:       chv1.UserSettings_DISTRIBUTED_PRODUCT_MODE_LOCAL,
		chmodels.DistributedProductModeGlobal:      chv1.UserSettings_DISTRIBUTED_PRODUCT_MODE_GLOBAL,
		chmodels.DistributedProductModeAllow:       chv1.UserSettings_DISTRIBUTED_PRODUCT_MODE_ALLOW,
	}
	mapDistributedProductModeFromGRPC = reflectutil.ReverseMap(mapDistributedProductModeToGRPC).(map[chv1.UserSettings_DistributedProductMode]chmodels.DistributedProductMode)
)

func DistributedProductModeToGRPC(mode chmodels.DistributedProductMode) chv1.UserSettings_DistributedProductMode {
	v, ok := mapDistributedProductModeToGRPC[mode]
	if !ok {
		return chv1.UserSettings_DISTRIBUTED_PRODUCT_MODE_UNSPECIFIED
	}

	return v
}

func DistributedProductModeFromGRPC(mode chv1.UserSettings_DistributedProductMode) (chmodels.DistributedProductMode, error) {
	v, ok := mapDistributedProductModeFromGRPC[mode]
	if !ok {
		return chmodels.DistributedProductModeUnspecified, semerr.InvalidInput("unknown distributed product mode")
	}

	return v, nil
}

var (
	mapQuotaModeToGRPC = map[chmodels.QuotaMode]chv1.UserSettings_QuotaMode{
		chmodels.QuotaModeUnspecified: chv1.UserSettings_QUOTA_MODE_UNSPECIFIED,
		chmodels.QuotaModeDefault:     chv1.UserSettings_QUOTA_MODE_DEFAULT,
		chmodels.QuotaModeKeyed:       chv1.UserSettings_QUOTA_MODE_KEYED,
		chmodels.QuotaModeKeyedByIP:   chv1.UserSettings_QUOTA_MODE_KEYED_BY_IP,
	}
	mapQuotaModeFromGRPC = reflectutil.ReverseMap(mapQuotaModeToGRPC).(map[chv1.UserSettings_QuotaMode]chmodels.QuotaMode)
)

func QuotaModeToGRPC(mode chmodels.QuotaMode) chv1.UserSettings_QuotaMode {
	v, ok := mapQuotaModeToGRPC[mode]
	if !ok {
		return chv1.UserSettings_QUOTA_MODE_UNSPECIFIED
	}

	return v
}

func QuotaModeFromGRPC(mode chv1.UserSettings_QuotaMode) (chmodels.QuotaMode, error) {
	v, ok := mapQuotaModeFromGRPC[mode]
	if !ok {
		return chmodels.QuotaModeUnspecified, semerr.InvalidInput("unknown quota mode")
	}

	return v, nil
}

var (
	mapCountDistinctImplementationToGRPC = map[chmodels.CountDistinctImplementation]chv1.UserSettings_CountDistinctImplementation{
		chmodels.CountDistinctImplementationUnspecified:    chv1.UserSettings_COUNT_DISTINCT_IMPLEMENTATION_UNSPECIFIED,
		chmodels.CountDistinctImplementationUniq:           chv1.UserSettings_COUNT_DISTINCT_IMPLEMENTATION_UNIQ,
		chmodels.CountDistinctImplementationUniqCombined:   chv1.UserSettings_COUNT_DISTINCT_IMPLEMENTATION_UNIQ_COMBINED,
		chmodels.CountDistinctImplementationUniqCombined64: chv1.UserSettings_COUNT_DISTINCT_IMPLEMENTATION_UNIQ_COMBINED_64,
		chmodels.CountDistinctImplementationUniqHLL12:      chv1.UserSettings_COUNT_DISTINCT_IMPLEMENTATION_UNIQ_HLL_12,
		chmodels.CountDistinctImplementationUniqExact:      chv1.UserSettings_COUNT_DISTINCT_IMPLEMENTATION_UNIQ_EXACT,
	}
	mapCountDistinctImplementationFromGRPC = reflectutil.ReverseMap(mapCountDistinctImplementationToGRPC).(map[chv1.UserSettings_CountDistinctImplementation]chmodels.CountDistinctImplementation)
)

func CountDistinctImplementationToGRPC(implementation chmodels.CountDistinctImplementation) chv1.UserSettings_CountDistinctImplementation {
	v, ok := mapCountDistinctImplementationToGRPC[implementation]
	if !ok {
		return chv1.UserSettings_COUNT_DISTINCT_IMPLEMENTATION_UNSPECIFIED
	}

	return v
}

func CountDistinctImplementationFromGRPC(mode chv1.UserSettings_CountDistinctImplementation) (chmodels.CountDistinctImplementation, error) {
	v, ok := mapCountDistinctImplementationFromGRPC[mode]
	if !ok {
		return chmodels.CountDistinctImplementationUnspecified, semerr.InvalidInput("unknown overflow mode")
	}

	return v, nil
}

var (
	statusToGRCP = map[clusters.Status]chv1.Cluster_Status{
		clusters.StatusCreating:             chv1.Cluster_CREATING,
		clusters.StatusCreateError:          chv1.Cluster_ERROR,
		clusters.StatusRunning:              chv1.Cluster_RUNNING,
		clusters.StatusModifying:            chv1.Cluster_UPDATING,
		clusters.StatusModifyError:          chv1.Cluster_ERROR,
		clusters.StatusStopping:             chv1.Cluster_STOPPING,
		clusters.StatusStopped:              chv1.Cluster_STOPPED,
		clusters.StatusStopError:            chv1.Cluster_ERROR,
		clusters.StatusStarting:             chv1.Cluster_STARTING,
		clusters.StatusStartError:           chv1.Cluster_ERROR,
		clusters.StatusMaintainingOffline:   chv1.Cluster_UPDATING,
		clusters.StatusMaintainOfflineError: chv1.Cluster_ERROR,
	}
)

func StatusToGRPC(status clusters.Status) chv1.Cluster_Status {
	v, ok := statusToGRCP[status]
	if !ok {
		return chv1.Cluster_STATUS_UNKNOWN
	}
	return v
}

func ClusterToGRPC(cluster chmodels.MDBCluster, saltEnvMapper grpc.SaltEnvMapper) *chv1.Cluster {
	v := &chv1.Cluster{
		Id:                 cluster.ClusterID,
		FolderId:           cluster.FolderExtID,
		CreatedAt:          grpc.TimeToGRPC(cluster.CreatedAt),
		Name:               cluster.Name,
		Description:        cluster.Description,
		Labels:             cluster.Labels,
		Environment:        chv1.Cluster_Environment(saltEnvMapper.ToGRPC(cluster.Environment)),
		Monitoring:         MonitoringToGRPC(cluster.Monitoring),
		Config:             ClusterConfigToGRPC(cluster.Config),
		NetworkId:          cluster.NetworkID,
		Health:             ClusterHealthToGRPC(cluster.Health),
		Status:             StatusToGRPC(cluster.Status),
		ServiceAccountId:   cluster.ServiceAccountID.String,
		MaintenanceWindow:  MaintenanceWindowToGRPC(cluster.MaintenanceInfo.Window),
		PlannedOperation:   MaintenanceOperationToGRPC(cluster.MaintenanceInfo.Operation),
		SecurityGroupIds:   cluster.SecurityGroupIDs,
		DeletionProtection: cluster.DeletionProtection,
	}

	return v
}

func ClustersToGRPC(clusters []chmodels.MDBCluster, saltEnvMapper grpc.SaltEnvMapper) []*chv1.Cluster {
	var v []*chv1.Cluster
	for _, cluster := range clusters {
		v = append(v, ClusterToGRPC(cluster, saltEnvMapper))
	}
	return v
}

func ModifyClusterArgsFromGRPC(req *chv1.UpdateClusterRequest) (clickhouse.UpdateMDBClusterArgs, error) {
	mask := grpc.NewFieldPaths(req.GetUpdateMask().GetPaths())

	updateArgs := clickhouse.UpdateMDBClusterArgs{
		ClusterID: req.GetClusterId(),
	}

	if mask.Remove("name") {
		updateArgs.Name = grpc.OptionalStringFromGRPC(req.GetName())
	}
	if mask.Remove("description") {
		updateArgs.Description = grpc.OptionalStringFromGRPC(req.GetDescription())
	}
	if mask.Remove("labels") {
		updateArgs.Labels = modelsoptional.NewLabels(req.GetLabels())
	}
	if mask.Remove("maintenance_window") {
		maintenanceWindow, err := MaintenanceWindowFromGRPC(req.GetMaintenanceWindow())
		if err != nil {
			return clickhouse.UpdateMDBClusterArgs{}, err
		}

		updateArgs.MaintenanceWindow = modelsoptional.NewMaintenanceWindow(maintenanceWindow)
	}
	if mask.Remove("service_account_id") {
		updateArgs.ServiceAccountID = grpc.OptionalStringFromGRPC(req.GetServiceAccountId())
	}
	if mask.Remove("security_group_ids") {
		updateArgs.SecurityGroupIDs = optional.NewStrings(req.GetSecurityGroupIds())
	}
	if mask.Remove("deletion_protection") {
		updateArgs.DeletionProtection = optional.NewBool(req.GetDeletionProtection())
	}

	clusterSpec, err := ClusterConfigSpecFromGRPC(req.GetConfigSpec())
	if err != nil {
		return clickhouse.UpdateMDBClusterArgs{}, err
	}

	if mask.Remove("config_spec.access") {
		clusterSpec.Access = grpc.AccessFromGRPC(req.GetConfigSpec().GetAccess())
	}
	clusterSpec.ClickHouseResources = grpc.ResourcesFromGRPC(req.GetConfigSpec().GetClickhouse().GetResources(), mask.Subtree("config_spec.clickhouse.resources."))
	clusterSpec.ZookeeperResources = grpc.ResourcesFromGRPC(req.GetConfigSpec().GetZookeeper().GetResources(), mask.Subtree("config_spec.zookeeper.resources."))

	updateArgs.ConfigSpec = chmodels.MDBConfigSpecUpdate{
		MDBClusterSpec: clusterSpec,
	}

	return updateArgs, mask.MustBeEmpty()
}

func RestoreClusterConfigSpecFromGRPC(cfg *chv1.ConfigSpec) (chmodels.RestoreMDBClusterSpec, error) {
	createConfigSpec, err := ClusterConfigSpecFromGRPC(cfg)
	if err != nil {
		return chmodels.RestoreMDBClusterSpec{}, err
	}

	configSpec := chmodels.RestoreMDBClusterSpec{}
	configSpec.Version = createConfigSpec.Version
	configSpec.ShardResources = []models.ClusterResourcesSpec{createConfigSpec.ClickHouseResources}
	configSpec.ZookeeperResources = createConfigSpec.ZookeeperResources
	configSpec.BackupWindowStart = createConfigSpec.BackupWindowStart
	configSpec.AdminPassword = createConfigSpec.AdminPassword
	configSpec.SQLUserManagement = createConfigSpec.SQLUserManagement
	configSpec.SQLDatabaseManagement = createConfigSpec.SQLDatabaseManagement
	configSpec.Access = createConfigSpec.Access
	configSpec.EmbeddedKeeper = createConfigSpec.EmbeddedKeeper
	configSpec.MySQLProtocol = createConfigSpec.MySQLProtocol
	configSpec.PostgreSQLProtocol = createConfigSpec.PostgreSQLProtocol
	configSpec.EnableCloudStorage = createConfigSpec.EnableCloudStorage
	configSpec.Config = createConfigSpec.Config
	configSpec.CloudStorageConfig = createConfigSpec.CloudStorageConfig

	return configSpec, nil
}

func CloudStorageConfigFromGRPC(cloudStorage *chv1.CloudStorage) chmodels.CloudStorageConfig {
	cloudStorageConfig := chmodels.CloudStorageConfig{}

	if dataCacheEnabled := cloudStorage.GetDataCacheEnabled(); dataCacheEnabled != nil {
		cloudStorageConfig.DataCacheEnabled = grpc.OptionalBoolFromGRPC(dataCacheEnabled)
	}
	if dataCacheMaxSize := cloudStorage.GetDataCacheMaxSize(); dataCacheMaxSize != nil {
		cloudStorageConfig.DataCacheMaxSize = grpc.OptionalInt64FromGRPC(dataCacheMaxSize)
	}
	if moveFactor := cloudStorage.GetMoveFactor(); moveFactor != nil {
		cloudStorageConfig.MoveFactor = grpc.OptionalFloat64FromGRPC(moveFactor)
	}

	return cloudStorageConfig
}

func ClusterConfigSpecFromGRPC(cfg *chv1.ConfigSpec) (chmodels.MDBClusterSpec, error) {
	configSpec := chmodels.MDBClusterSpec{}
	configSpec.Version = chmodels.CutVersionToMajor(cfg.GetVersion())
	configSpec.SQLUserManagement = grpc.OptionalBoolFromGRPC(cfg.GetSqlUserManagement())
	configSpec.SQLDatabaseManagement = grpc.OptionalBoolFromGRPC(cfg.GetSqlDatabaseManagement())
	configSpec.EmbeddedKeeper = grpc.OptionalBoolFromGRPC(cfg.GetEmbeddedKeeper()).Bool
	configSpec.AdminPassword = grpc.OptionalPasswordFromGRPC(cfg.GetAdminPassword())
	configSpec.MySQLProtocol = optional.NewBool(false)
	configSpec.PostgreSQLProtocol = optional.NewBool(false)
	if cloudStorage := cfg.GetCloudStorage(); cloudStorage != nil {
		configSpec.EnableCloudStorage = optional.NewBool(cloudStorage.Enabled)
		if cloudStorage.Enabled {
			configSpec.CloudStorageConfig = CloudStorageConfigFromGRPC(cloudStorage)
		}
	}

	if ch := cfg.GetClickhouse(); ch != nil {
		configSpec.ClickHouseResources = grpc.ResourcesFromGRPC(ch.GetResources(), grpc.AllPaths())

		configs, err := ClickHouseConfigFromGRPC(ch.GetConfig())
		if err != nil {
			return chmodels.MDBClusterSpec{}, err
		}

		configSpec.Config = configs
	}

	if zk := cfg.GetZookeeper(); zk != nil {
		configSpec.ZookeeperResources = grpc.ResourcesFromGRPC(zk.GetResources(), grpc.AllPaths())
	}

	if access := cfg.GetAccess(); access != nil {
		configSpec.Access = clusters.Access{
			DataLens:     optional.NewBool(access.DataLens),
			WebSQL:       optional.NewBool(access.WebSql),
			Metrica:      optional.NewBool(access.Metrika),
			Serverless:   optional.NewBool(access.Serverless),
			DataTransfer: optional.NewBool(access.DataTransfer),
			YandexQuery:  optional.NewBool(access.YandexQuery),
		}
	}

	if cfgBackupStart := cfg.GetBackupWindowStart(); cfgBackupStart != nil {
		configSpec.BackupWindowStart.Set(bmodels.BackupWindowStart{
			Hours:   int(cfg.BackupWindowStart.Hours),
			Minutes: int(cfg.BackupWindowStart.Minutes),
			Seconds: int(cfg.BackupWindowStart.Seconds),
			Nanos:   int(cfg.BackupWindowStart.Nanos),
		})
	}

	return configSpec, nil
}

var (
	mapHostRoleToGRPC = map[hosts.Role]chv1.Host_Type{
		hosts.RoleClickHouse: chv1.Host_CLICKHOUSE,
		hosts.RoleZooKeeper:  chv1.Host_ZOOKEEPER,
	}
	mapHostRoleFromGRPC = reflectutil.ReverseMap(mapHostRoleToGRPC).(map[chv1.Host_Type]hosts.Role)
)

func HostRoleToGRPC(role hosts.Role) chv1.Host_Type {
	v, ok := mapHostRoleToGRPC[role]
	if !ok {
		return chv1.Host_TYPE_UNSPECIFIED
	}

	return v
}

func HostRoleFromGRPC(ht chv1.Host_Type) (hosts.Role, error) {
	v, ok := mapHostRoleFromGRPC[ht]
	if !ok {
		return hosts.RoleUnknown, semerr.InvalidInput("unknown host role")
	}

	return v, nil
}

func HostSpecsFromGRPC(specs []*chv1.HostSpec) ([]chmodels.HostSpec, error) {
	var result []chmodels.HostSpec
	for _, s := range specs {
		hostRole, err := HostRoleFromGRPC(s.Type)
		if err != nil {
			return nil, err
		}

		var spec = chmodels.HostSpec{
			AssignPublicIP: s.AssignPublicIp,
			HostRole:       hostRole,
			ShardName:      s.ShardName,
			SubnetID:       s.SubnetId,
			ZoneID:         s.ZoneId,
		}
		result = append(result, spec)
	}
	return result, nil
}

func UpdateHostSpecsFromGRPC(specs []*chv1.UpdateHostSpec) ([]chmodels.UpdateHostSpec, error) {
	var result []chmodels.UpdateHostSpec
	for _, s := range specs {
		mask, err := fieldmaskutils.MaskFromProtoFieldMask(s.UpdateMask, func(s string) string { return s })
		if err != nil {
			return []chmodels.UpdateHostSpec{}, semerr.WrapWithInvalidInputf(err, "invalid field mask for UpdateHostSpec")
		}
		var spec = chmodels.UpdateHostSpec{
			HostName: s.HostName,
		}
		if _, ok := mask.Filter("assign_public_ip"); ok {
			spec.AssignPublicIP = grpc.OptionalBoolFromGRPC(s.AssignPublicIp)
		}
		result = append(result, spec)
	}
	return result, nil
}

func UserSpecFromGRPC(spec *chv1.UserSpec) (chmodels.UserSpec, error) {
	us := chmodels.UserSpec{Name: spec.Name, Password: secret.NewString(spec.Password)}
	for _, perm := range spec.Permissions {
		p, err := UserPermissionFromGRPC(perm)
		if err != nil {
			return chmodels.UserSpec{}, err
		}
		us.Permissions = append(us.Permissions, p)
	}
	if spec.Settings != nil {
		settings, err := UserSettingsFromGRPC(spec.Settings)
		if err != nil {
			return chmodels.UserSpec{}, err
		}
		us.Settings = settings
	}

	quotas, err := UserQuotasFromGRPC(spec.Quotas)
	if err != nil {
		return chmodels.UserSpec{}, err
	}

	us.UserQuotas = quotas
	return us, nil
}

func UserPermissionFromGRPC(spec *chv1.Permission) (chmodels.Permission, error) {
	perm := chmodels.Permission{DatabaseName: spec.DatabaseName}
	for _, filter := range spec.DataFilters {
		perm.DataFilters = append(perm.DataFilters, chmodels.DataFilter{
			Filter:    filter.Filter,
			TableName: filter.TableName,
		})
	}

	return perm, nil
}

func UserSpecsFromGRPC(specs []*chv1.UserSpec) ([]chmodels.UserSpec, error) {
	var v []chmodels.UserSpec
	for _, rawSpec := range specs {
		if rawSpec == nil {
			continue
		}
		spec, err := UserSpecFromGRPC(rawSpec)
		if err != nil {
			return nil, err
		}
		v = append(v, spec)
	}
	return v, nil
}

func UserSettingsFromGRPC(settings *chv1.UserSettings) (chmodels.UserSettings, error) {
	if settings == nil {
		return chmodels.UserSettings{}, nil
	}
	distributedProductMode, err := DistributedProductModeFromGRPC(settings.DistributedProductMode)
	if err != nil {
		return chmodels.UserSettings{}, err
	}
	readOverflowMode, err := OverflowModeFromGRPC(settings.ReadOverflowMode)
	if err != nil {
		return chmodels.UserSettings{}, err
	}
	groupByOverflowMode, err := GroupByOverflowModeFromGRPC(settings.GroupByOverflowMode)
	if err != nil {
		return chmodels.UserSettings{}, err
	}
	sortOverflowMode, err := OverflowModeFromGRPC(settings.SortOverflowMode)
	if err != nil {
		return chmodels.UserSettings{}, err
	}
	resultOverflowMode, err := OverflowModeFromGRPC(settings.ResultOverflowMode)
	if err != nil {
		return chmodels.UserSettings{}, err
	}
	distinctOverflowMode, err := OverflowModeFromGRPC(settings.DistinctOverflowMode)
	if err != nil {
		return chmodels.UserSettings{}, err
	}
	transferOverflowMode, err := OverflowModeFromGRPC(settings.TransferOverflowMode)
	if err != nil {
		return chmodels.UserSettings{}, err
	}
	timeoutOverflowMode, err := OverflowModeFromGRPC(settings.TimeoutOverflowMode)
	if err != nil {
		return chmodels.UserSettings{}, err
	}
	setOverflowMode, err := OverflowModeFromGRPC(settings.SetOverflowMode)
	if err != nil {
		return chmodels.UserSettings{}, err
	}
	joinOverflowMode, err := OverflowModeFromGRPC(settings.JoinOverflowMode)
	if err != nil {
		return chmodels.UserSettings{}, err
	}
	quotaMode, err := QuotaModeFromGRPC(settings.QuotaMode)
	if err != nil {
		return chmodels.UserSettings{}, err
	}
	countDistinctImplementation, err := CountDistinctImplementationFromGRPC(settings.CountDistinctImplementation)
	if err != nil {
		return chmodels.UserSettings{}, err
	}

	res := chmodels.UserSettings{
		// Permissions
		Readonly: grpc.OptionalInt64FromGRPC(settings.Readonly),
		AllowDDL: grpc.OptionalInt64FromBoolGRPC(settings.AllowDdl),

		// Timeouts
		ConnectTimeout: grpc.OptionalSecondsFromGRPCMilliseconds(settings.ConnectTimeout),
		ReceiveTimeout: grpc.OptionalSecondsFromGRPCMilliseconds(settings.ReceiveTimeout),
		SendTimeout:    grpc.OptionalSecondsFromGRPCMilliseconds(settings.SendTimeout),

		// Replication settings
		InsertQuorum:                   grpc.OptionalInt64FromGRPC(settings.InsertQuorum),
		InsertQuorumTimeout:            grpc.OptionalSecondsFromGRPCMilliseconds(settings.InsertQuorumTimeout),
		SelectSequentialConsistency:    grpc.OptionalInt64FromBoolGRPC(settings.SelectSequentialConsistency),
		ReplicationAlterPartitionsSync: grpc.OptionalInt64FromGRPC(settings.ReplicationAlterPartitionsSync),

		// Settings of distributed queries
		MaxReplicaDelayForDistributedQueries:         grpc.OptionalSecondsFromGRPCMilliseconds(settings.MaxReplicaDelayForDistributedQueries),
		FallbackToStaleReplicasForDistributedQueries: grpc.OptionalInt64FromBoolGRPC(settings.FallbackToStaleReplicasForDistributedQueries),
		DistributedProductMode:                       distributedProductMode,
		DistributedAggregationMemoryEfficient:        grpc.OptionalInt64FromBoolGRPC(settings.DistributedAggregationMemoryEfficient),
		DistributedDDLTaskTimeout:                    grpc.OptionalSecondsFromGRPCMilliseconds(settings.DistributedDdlTaskTimeout),
		SkipUnavailableShards:                        grpc.OptionalInt64FromBoolGRPC(settings.SkipUnavailableShards),

		// Query and expression compilations
		Compile:                     grpc.OptionalInt64FromBoolGRPC(settings.Compile),
		MinCountToCompile:           grpc.OptionalInt64FromGRPC(settings.MinCountToCompile),
		CompileExpressions:          grpc.OptionalInt64FromBoolGRPC(settings.CompileExpressions),
		MinCountToCompileExpression: grpc.OptionalInt64FromGRPC(settings.MinCountToCompileExpression),

		// I/O settings
		MaxBlockSize:                       grpc.OptionalInt64FromGRPC(settings.MaxBlockSize),
		MinInsertBlockSizeRows:             grpc.OptionalInt64FromGRPC(settings.MinInsertBlockSizeRows),
		MinInsertBlockSizeBytes:            grpc.OptionalInt64FromGRPC(settings.MinInsertBlockSizeBytes),
		MaxInsertBlockSize:                 grpc.OptionalInt64FromGRPC(settings.MaxInsertBlockSize),
		MinBytesToUseDirectIO:              grpc.OptionalInt64FromGRPC(settings.MinBytesToUseDirectIo),
		UseUncompressedCache:               grpc.OptionalInt64FromBoolGRPC(settings.UseUncompressedCache),
		MergeTreeMaxRowsToUseCache:         grpc.OptionalInt64FromGRPC(settings.MergeTreeMaxRowsToUseCache),
		MergeTreeMaxBytesToUseCache:        grpc.OptionalInt64FromGRPC(settings.MergeTreeMaxBytesToUseCache),
		MergeTreeMinRowsForConcurrentRead:  grpc.OptionalInt64FromGRPC(settings.MergeTreeMinRowsForConcurrentRead),
		MergeTreeMinBytesForConcurrentRead: grpc.OptionalInt64FromGRPC(settings.MergeTreeMinBytesForConcurrentRead),
		MaxBytesBeforeExternalGroupBy:      grpc.OptionalInt64FromGRPC(settings.MaxBytesBeforeExternalGroupBy),
		MaxBytesBeforeExternalSort:         grpc.OptionalInt64FromGRPC(settings.MaxBytesBeforeExternalSort),
		GroupByTwoLevelThreshold:           grpc.OptionalInt64FromGRPC(settings.GroupByTwoLevelThreshold),
		GroupByTwoLevelThresholdBytes:      grpc.OptionalInt64FromGRPC(settings.GroupByTwoLevelThresholdBytes),

		// Resource usage limits and query priorities
		Priority:                   grpc.OptionalInt64FromGRPC(settings.Priority),
		MaxThreads:                 grpc.OptionalInt64FromGRPC(settings.MaxThreads),
		MaxMemoryUsage:             grpc.OptionalInt64FromGRPC(settings.MaxMemoryUsage),
		MaxMemoryUsageForUser:      grpc.OptionalInt64FromGRPC(settings.MaxMemoryUsageForUser),
		MaxNetworkBandwidth:        grpc.OptionalInt64FromGRPC(settings.MaxNetworkBandwidth),
		MaxNetworkBandwidthForUser: grpc.OptionalInt64FromGRPC(settings.MaxNetworkBandwidthForUser),

		// Query complexity limits
		ForceIndexByDate:            grpc.OptionalInt64FromBoolGRPC(settings.ForceIndexByDate),
		ForcePrimaryKey:             grpc.OptionalInt64FromBoolGRPC(settings.ForcePrimaryKey),
		MaxRowsToRead:               grpc.OptionalInt64FromGRPC(settings.MaxRowsToRead),
		MaxBytesToRead:              grpc.OptionalInt64FromGRPC(settings.MaxBytesToRead),
		ReadOverflowMode:            readOverflowMode,
		MaxRowsToGroupBy:            grpc.OptionalInt64FromGRPC(settings.MaxRowsToGroupBy),
		GroupByOverflowMode:         groupByOverflowMode,
		MaxRowsToSort:               grpc.OptionalInt64FromGRPC(settings.MaxRowsToSort),
		MaxBytesToSort:              grpc.OptionalInt64FromGRPC(settings.MaxBytesToSort),
		SortOverflowMode:            sortOverflowMode,
		MaxResultRows:               grpc.OptionalInt64FromGRPC(settings.MaxResultRows),
		MaxResultBytes:              grpc.OptionalInt64FromGRPC(settings.MaxResultBytes),
		ResultOverflowMode:          resultOverflowMode,
		MaxRowsInDistinct:           grpc.OptionalInt64FromGRPC(settings.MaxRowsInDistinct),
		MaxBytesInDistinct:          grpc.OptionalInt64FromGRPC(settings.MaxBytesInDistinct),
		DistinctOverflowMode:        distinctOverflowMode,
		MaxRowsToTransfer:           grpc.OptionalInt64FromGRPC(settings.MaxRowsToTransfer),
		MaxBytesToTransfer:          grpc.OptionalInt64FromGRPC(settings.MaxBytesToTransfer),
		TransferOverflowMode:        transferOverflowMode,
		MaxExecutionTime:            grpc.OptionalSecondsFromGRPCMilliseconds(settings.MaxExecutionTime),
		TimeoutOverflowMode:         timeoutOverflowMode,
		MaxRowsInSet:                grpc.OptionalInt64FromGRPC(settings.MaxRowsInSet),
		MaxBytesInSet:               grpc.OptionalInt64FromGRPC(settings.MaxBytesInSet),
		SetOverflowMode:             setOverflowMode,
		MaxRowsInJoin:               grpc.OptionalInt64FromGRPC(settings.MaxRowsInJoin),
		MaxBytesInJoin:              grpc.OptionalInt64FromGRPC(settings.MaxBytesInJoin),
		JoinOverflowMode:            joinOverflowMode,
		MaxColumnsToRead:            grpc.OptionalInt64FromGRPC(settings.MaxColumnsToRead),
		MaxTemporaryColumns:         grpc.OptionalInt64FromGRPC(settings.MaxTemporaryColumns),
		MaxTemporaryNonConstColumns: grpc.OptionalInt64FromGRPC(settings.MaxTemporaryNonConstColumns),
		MaxQuerySize:                grpc.OptionalInt64FromGRPC(settings.MaxQuerySize),
		MaxAstDepth:                 grpc.OptionalInt64FromGRPC(settings.MaxAstDepth),
		MaxAstElements:              grpc.OptionalInt64FromGRPC(settings.MaxAstElements),
		MaxExpandedAstElements:      grpc.OptionalInt64FromGRPC(settings.MaxExpandedAstElements),
		MinExecutionSpeed:           grpc.OptionalInt64FromGRPC(settings.MinExecutionSpeed),
		MinExecutionSpeedBytes:      grpc.OptionalInt64FromGRPC(settings.MinExecutionSpeedBytes),

		// Settings of input and output formats
		InputFormatValuesInterpretExpressions: grpc.OptionalInt64FromBoolGRPC(settings.InputFormatValuesInterpretExpressions),
		InputFormatDefaultsForOmittedFields:   grpc.OptionalInt64FromBoolGRPC(settings.InputFormatDefaultsForOmittedFields),
		OutputFormatJSONQuote64bitIntegers:    grpc.OptionalInt64FromBoolGRPC(settings.OutputFormatJsonQuote_64BitIntegers),
		OutputFormatJSONQuoteDenormals:        grpc.OptionalInt64FromBoolGRPC(settings.OutputFormatJsonQuoteDenormals),
		LowCardinalityAllowInNativeFormat:     grpc.OptionalInt64FromBoolGRPC(settings.LowCardinalityAllowInNativeFormat),
		EmptyResultForAggregationByEmptySet:   grpc.OptionalInt64FromBoolGRPC(settings.EmptyResultForAggregationByEmptySet),

		// HTTP-specific settings
		HTTPConnectionTimeout:       grpc.OptionalSecondsFromGRPCMilliseconds(settings.HttpConnectionTimeout),
		HTTPReceiveTimeout:          grpc.OptionalSecondsFromGRPCMilliseconds(settings.HttpReceiveTimeout),
		HTTPSendTimeout:             grpc.OptionalSecondsFromGRPCMilliseconds(settings.HttpSendTimeout),
		EnableHTTPCompression:       grpc.OptionalInt64FromBoolGRPC(settings.EnableHttpCompression),
		SendProgressInHTTPHeaders:   grpc.OptionalInt64FromBoolGRPC(settings.SendProgressInHttpHeaders),
		HTTPHeadersProgressInterval: grpc.OptionalInt64FromGRPC(settings.HttpHeadersProgressInterval),
		AddHTTPCorsHeader:           grpc.OptionalInt64FromBoolGRPC(settings.AddHttpCorsHeader),

		// Quoting settings
		QuotaMode: quotaMode,

		//// Other settings
		CountDistinctImplementation: countDistinctImplementation,
		JoinedSubqueryRequiresAlias: grpc.OptionalInt64FromBoolGRPC(settings.JoinedSubqueryRequiresAlias),
		JoinUseNulls:                grpc.OptionalInt64FromBoolGRPC(settings.JoinUseNulls),
		TransformNullIn:             grpc.OptionalInt64FromBoolGRPC(settings.TransformNullIn),
	}

	return res, nil
}

func UserSettingsToGRPC(settings chmodels.UserSettings) *chv1.UserSettings {
	return &chv1.UserSettings{
		// Permissions
		Readonly: grpc.OptionalInt64ToGRPC(settings.Readonly),
		AllowDdl: grpc.OptionalInt64ToBoolGRPC(settings.AllowDDL),

		// Timeouts
		ConnectTimeout: grpc.OptionalSecondsToGRPCMilliseconds(settings.ConnectTimeout),
		ReceiveTimeout: grpc.OptionalSecondsToGRPCMilliseconds(settings.ReceiveTimeout),
		SendTimeout:    grpc.OptionalSecondsToGRPCMilliseconds(settings.SendTimeout),

		// Replication settings
		InsertQuorum:                   grpc.OptionalInt64ToGRPC(settings.InsertQuorum),
		InsertQuorumTimeout:            grpc.OptionalSecondsToGRPCMilliseconds(settings.InsertQuorumTimeout),
		SelectSequentialConsistency:    grpc.OptionalInt64ToBoolGRPC(settings.SelectSequentialConsistency),
		ReplicationAlterPartitionsSync: grpc.OptionalInt64ToGRPC(settings.ReplicationAlterPartitionsSync),

		// Settings of distributed queries
		MaxReplicaDelayForDistributedQueries:         grpc.OptionalSecondsToGRPCMilliseconds(settings.MaxReplicaDelayForDistributedQueries),
		FallbackToStaleReplicasForDistributedQueries: grpc.OptionalInt64ToBoolGRPC(settings.FallbackToStaleReplicasForDistributedQueries),
		DistributedProductMode:                       DistributedProductModeToGRPC(settings.DistributedProductMode),
		DistributedAggregationMemoryEfficient:        grpc.OptionalInt64ToBoolGRPC(settings.DistributedAggregationMemoryEfficient),
		DistributedDdlTaskTimeout:                    grpc.OptionalSecondsToGRPCMilliseconds(settings.DistributedDDLTaskTimeout),
		SkipUnavailableShards:                        grpc.OptionalInt64ToBoolGRPC(settings.SkipUnavailableShards),

		// Query and expression compilations
		Compile:                     grpc.OptionalInt64ToBoolGRPC(settings.Compile),
		MinCountToCompile:           grpc.OptionalInt64ToGRPC(settings.MinCountToCompile),
		CompileExpressions:          grpc.OptionalInt64ToBoolGRPC(settings.CompileExpressions),
		MinCountToCompileExpression: grpc.OptionalInt64ToGRPC(settings.MinCountToCompileExpression),

		// I/O settings
		MaxBlockSize:                       grpc.OptionalInt64ToGRPC(settings.MaxBlockSize),
		MinInsertBlockSizeRows:             grpc.OptionalInt64ToGRPC(settings.MinInsertBlockSizeRows),
		MinInsertBlockSizeBytes:            grpc.OptionalInt64ToGRPC(settings.MinInsertBlockSizeBytes),
		MaxInsertBlockSize:                 grpc.OptionalInt64ToGRPC(settings.MaxInsertBlockSize),
		MinBytesToUseDirectIo:              grpc.OptionalInt64ToGRPC(settings.MinBytesToUseDirectIO),
		UseUncompressedCache:               grpc.OptionalInt64ToBoolGRPC(settings.UseUncompressedCache),
		MergeTreeMaxRowsToUseCache:         grpc.OptionalInt64ToGRPC(settings.MergeTreeMaxRowsToUseCache),
		MergeTreeMaxBytesToUseCache:        grpc.OptionalInt64ToGRPC(settings.MergeTreeMaxBytesToUseCache),
		MergeTreeMinRowsForConcurrentRead:  grpc.OptionalInt64ToGRPC(settings.MergeTreeMinRowsForConcurrentRead),
		MergeTreeMinBytesForConcurrentRead: grpc.OptionalInt64ToGRPC(settings.MergeTreeMinBytesForConcurrentRead),
		MaxBytesBeforeExternalGroupBy:      grpc.OptionalInt64ToGRPC(settings.MaxBytesBeforeExternalGroupBy),
		MaxBytesBeforeExternalSort:         grpc.OptionalInt64ToGRPC(settings.MaxBytesBeforeExternalSort),
		GroupByTwoLevelThreshold:           grpc.OptionalInt64ToGRPC(settings.GroupByTwoLevelThreshold),
		GroupByTwoLevelThresholdBytes:      grpc.OptionalInt64ToGRPC(settings.GroupByTwoLevelThresholdBytes),

		// Resource usage limits and query priorities
		Priority:                   grpc.OptionalInt64ToGRPC(settings.Priority),
		MaxThreads:                 grpc.OptionalInt64ToGRPC(settings.MaxThreads),
		MaxMemoryUsage:             grpc.OptionalInt64ToGRPC(settings.MaxMemoryUsage),
		MaxMemoryUsageForUser:      grpc.OptionalInt64ToGRPC(settings.MaxMemoryUsageForUser),
		MaxNetworkBandwidth:        grpc.OptionalInt64ToGRPC(settings.MaxNetworkBandwidth),
		MaxNetworkBandwidthForUser: grpc.OptionalInt64ToGRPC(settings.MaxNetworkBandwidthForUser),

		// Query complexity limits
		ForceIndexByDate:            grpc.OptionalInt64ToBoolGRPC(settings.ForceIndexByDate),
		ForcePrimaryKey:             grpc.OptionalInt64ToBoolGRPC(settings.ForcePrimaryKey),
		MaxRowsToRead:               grpc.OptionalInt64ToGRPC(settings.MaxRowsToRead),
		MaxBytesToRead:              grpc.OptionalInt64ToGRPC(settings.MaxBytesToRead),
		ReadOverflowMode:            OverflowModeToGRPC(settings.ReadOverflowMode),
		MaxRowsToGroupBy:            grpc.OptionalInt64ToGRPC(settings.MaxRowsToGroupBy),
		GroupByOverflowMode:         GroupByOverflowModeToGRPC(settings.GroupByOverflowMode),
		MaxRowsToSort:               grpc.OptionalInt64ToGRPC(settings.MaxRowsToSort),
		MaxBytesToSort:              grpc.OptionalInt64ToGRPC(settings.MaxBytesToSort),
		SortOverflowMode:            OverflowModeToGRPC(settings.SortOverflowMode),
		MaxResultRows:               grpc.OptionalInt64ToGRPC(settings.MaxResultRows),
		MaxResultBytes:              grpc.OptionalInt64ToGRPC(settings.MaxResultBytes),
		ResultOverflowMode:          OverflowModeToGRPC(settings.ResultOverflowMode),
		MaxRowsInDistinct:           grpc.OptionalInt64ToGRPC(settings.MaxRowsInDistinct),
		MaxBytesInDistinct:          grpc.OptionalInt64ToGRPC(settings.MaxBytesInDistinct),
		DistinctOverflowMode:        OverflowModeToGRPC(settings.DistinctOverflowMode),
		MaxRowsToTransfer:           grpc.OptionalInt64ToGRPC(settings.MaxRowsToTransfer),
		MaxBytesToTransfer:          grpc.OptionalInt64ToGRPC(settings.MaxBytesToTransfer),
		TransferOverflowMode:        OverflowModeToGRPC(settings.TransferOverflowMode),
		MaxExecutionTime:            grpc.OptionalSecondsToGRPCMilliseconds(settings.MaxExecutionTime),
		TimeoutOverflowMode:         OverflowModeToGRPC(settings.TimeoutOverflowMode),
		MaxRowsInSet:                grpc.OptionalInt64ToGRPC(settings.MaxRowsInSet),
		MaxBytesInSet:               grpc.OptionalInt64ToGRPC(settings.MaxBytesInSet),
		SetOverflowMode:             OverflowModeToGRPC(settings.SetOverflowMode),
		MaxRowsInJoin:               grpc.OptionalInt64ToGRPC(settings.MaxRowsInJoin),
		MaxBytesInJoin:              grpc.OptionalInt64ToGRPC(settings.MaxBytesInJoin),
		JoinOverflowMode:            OverflowModeToGRPC(settings.JoinOverflowMode),
		MaxColumnsToRead:            grpc.OptionalInt64ToGRPC(settings.MaxColumnsToRead),
		MaxTemporaryColumns:         grpc.OptionalInt64ToGRPC(settings.MaxTemporaryColumns),
		MaxTemporaryNonConstColumns: grpc.OptionalInt64ToGRPC(settings.MaxTemporaryNonConstColumns),
		MaxQuerySize:                grpc.OptionalInt64ToGRPC(settings.MaxQuerySize),
		MaxAstDepth:                 grpc.OptionalInt64ToGRPC(settings.MaxAstDepth),
		MaxAstElements:              grpc.OptionalInt64ToGRPC(settings.MaxAstElements),
		MaxExpandedAstElements:      grpc.OptionalInt64ToGRPC(settings.MaxExpandedAstElements),
		MinExecutionSpeed:           grpc.OptionalInt64ToGRPC(settings.MinExecutionSpeed),
		MinExecutionSpeedBytes:      grpc.OptionalInt64ToGRPC(settings.MinExecutionSpeedBytes),

		// Settings of input and output formats
		InputFormatValuesInterpretExpressions: grpc.OptionalInt64ToBoolGRPC(settings.InputFormatValuesInterpretExpressions),
		InputFormatDefaultsForOmittedFields:   grpc.OptionalInt64ToBoolGRPC(settings.InputFormatDefaultsForOmittedFields),
		OutputFormatJsonQuote_64BitIntegers:   grpc.OptionalInt64ToBoolGRPC(settings.OutputFormatJSONQuote64bitIntegers),
		OutputFormatJsonQuoteDenormals:        grpc.OptionalInt64ToBoolGRPC(settings.OutputFormatJSONQuoteDenormals),
		LowCardinalityAllowInNativeFormat:     grpc.OptionalInt64ToBoolGRPC(settings.LowCardinalityAllowInNativeFormat),
		EmptyResultForAggregationByEmptySet:   grpc.OptionalInt64ToBoolGRPC(settings.EmptyResultForAggregationByEmptySet),

		// HTTP-specific settings
		HttpConnectionTimeout:       grpc.OptionalSecondsToGRPCMilliseconds(settings.HTTPConnectionTimeout),
		HttpReceiveTimeout:          grpc.OptionalSecondsToGRPCMilliseconds(settings.HTTPReceiveTimeout),
		HttpSendTimeout:             grpc.OptionalSecondsToGRPCMilliseconds(settings.HTTPSendTimeout),
		EnableHttpCompression:       grpc.OptionalInt64ToBoolGRPC(settings.EnableHTTPCompression),
		SendProgressInHttpHeaders:   grpc.OptionalInt64ToBoolGRPC(settings.SendProgressInHTTPHeaders),
		HttpHeadersProgressInterval: grpc.OptionalInt64ToGRPC(settings.HTTPHeadersProgressInterval),
		AddHttpCorsHeader:           grpc.OptionalInt64ToBoolGRPC(settings.AddHTTPCorsHeader),

		// Quoting settings
		QuotaMode: QuotaModeToGRPC(settings.QuotaMode),

		// Other settings
		CountDistinctImplementation: CountDistinctImplementationToGRPC(settings.CountDistinctImplementation),
		JoinedSubqueryRequiresAlias: grpc.OptionalInt64ToBoolGRPC(settings.JoinedSubqueryRequiresAlias),
		JoinUseNulls:                grpc.OptionalInt64ToBoolGRPC(settings.JoinUseNulls),
		TransformNullIn:             grpc.OptionalInt64ToBoolGRPC(settings.TransformNullIn),
	}
}

func UserQuotasFromGRPC(quotas []*chv1.UserQuota) ([]chmodels.UserQuota, error) {
	var result []chmodels.UserQuota

	for _, quota := range quotas {
		if quota.IntervalDuration == nil {
			return nil, xerrors.Errorf("quota interval duration must be set")
		}

		result = append(result, chmodels.UserQuota{
			IntervalDuration: time.Millisecond * time.Duration(quota.IntervalDuration.Value),
			Queries:          grpc.OptionalInt64FromGRPC(quota.Queries),
			Errors:           grpc.OptionalInt64FromGRPC(quota.Errors),
			ResultRows:       grpc.OptionalInt64FromGRPC(quota.ResultRows),
			ReadRows:         grpc.OptionalInt64FromGRPC(quota.ReadRows),
			ExecutionTime:    grpc.OptionalDurationFromGRPC(quota.ExecutionTime),
		})
	}

	return result, nil
}

func UserToGRPC(user chmodels.User) *chv1.User {
	v := &chv1.User{Name: user.Name, ClusterId: user.ClusterID, Settings: UserSettingsToGRPC(user.Settings)}
	for _, perm := range user.Permissions {
		v.Permissions = append(v.Permissions, &chv1.Permission{
			DatabaseName: perm.DatabaseName,
		})
	}

	for _, quota := range user.UserQuotas {
		v.Quotas = append(v.Quotas, &chv1.UserQuota{
			IntervalDuration: &wrappers.Int64Value{Value: quota.IntervalDuration.Milliseconds()},
			Queries:          grpc.OptionalInt64ToGRPC(quota.Queries),
			Errors:           grpc.OptionalInt64ToGRPC(quota.Errors),
			ResultRows:       grpc.OptionalInt64ToGRPC(quota.ResultRows),
			ReadRows:         grpc.OptionalInt64ToGRPC(quota.ReadRows),
			ExecutionTime:    grpc.OptionalDurationToGRPC(quota.ExecutionTime),
		})
	}

	return v
}

func UsersToGRPC(users []chmodels.User) []*chv1.User {
	var v []*chv1.User
	for _, user := range users {
		v = append(v, UserToGRPC(user))
	}
	return v
}

func UserUpdateFromGRPC(req *chv1.UpdateUserRequest) (chmodels.UpdateUserArgs, error) {
	mask, err := fieldmaskutils.MaskFromProtoFieldMask(req.UpdateMask, func(s string) string { return s })
	if err != nil {
		return chmodels.UpdateUserArgs{}, semerr.WrapWithInvalidInputf(err, "invalid field mask for UpdateUserRequest")
	}

	spec := chmodels.UpdateUserArgs{}

	if _, ok := mask.Filter("password"); ok {
		pass := secret.NewString(req.Password)
		spec.Password = &pass
	}

	if _, ok := mask.Filter("permissions"); ok {
		var permissions []chmodels.Permission
		for _, perm := range req.Permissions {
			p, err := UserPermissionFromGRPC(perm)
			if err != nil {
				return chmodels.UpdateUserArgs{}, err
			}

			permissions = append(permissions, p)
		}

		spec.Permissions = &permissions
	}

	if _, ok := mask.Filter("quotas"); ok {
		quotas, err := UserQuotasFromGRPC(req.Quotas)
		if err != nil {
			return chmodels.UpdateUserArgs{}, err
		}

		spec.UserQuotas = &quotas
	}

	if submask, ok := mask.Filter("settings"); ok {
		settings, err := UserSettingsFromGRPC(req.Settings)
		if err != nil {
			return chmodels.UpdateUserArgs{}, err
		}

		spec.Settings = &settings
		spec.SettingsMask = submask
	}

	return spec, nil
}

func DatabaseSpecFromGRPC(spec *chv1.DatabaseSpec) chmodels.DatabaseSpec {
	return chmodels.DatabaseSpec{Name: spec.Name}
}

func DatabaseSpecsFromGRPC(specs []*chv1.DatabaseSpec) []chmodels.DatabaseSpec {
	var v []chmodels.DatabaseSpec
	for _, spec := range specs {
		if spec != nil {
			v = append(v, DatabaseSpecFromGRPC(spec))
		}
	}
	return v
}

func DatabaseToGRPC(db chmodels.Database) *chv1.Database {
	return &chv1.Database{Name: db.Name, ClusterId: db.ClusterID}
}

func DatabasesToGRPC(dbs []chmodels.Database) []*chv1.Database {
	var v []*chv1.Database
	for _, db := range dbs {
		v = append(v, DatabaseToGRPC(db))
	}
	return v
}

var (
	mapMlModelTypeToGRPC = map[chmodels.MLModelType]chv1.MlModelType{
		chmodels.CatBoostMLModel: chv1.MlModelType_ML_MODEL_TYPE_CATBOOST,
	}
	mapMlModelTypeFromGRPC = reflectutil.ReverseMap(mapMlModelTypeToGRPC).(map[chv1.MlModelType]chmodels.MLModelType)
)

func MlModelTypeFromGRPC(t chv1.MlModelType) (chmodels.MLModelType, error) {
	v, ok := mapMlModelTypeFromGRPC[t]
	if ok {
		return v, nil
	}

	return v, semerr.InvalidInputf("invalid ml model type %q", t.String())
}

func MLModelToGRPC(model chmodels.MLModel) (*chv1.MlModel, error) {
	modelType, ok := mapMlModelTypeToGRPC[model.Type]
	if !ok {
		return nil, semerr.Internalf("invalid ml model type %q", model.Type)
	}

	v := &chv1.MlModel{
		ClusterId: model.ClusterID,
		Name:      model.Name,
		Type:      modelType,
		Uri:       model.URI,
	}
	return v, nil
}

func MLModelsToGRPC(models []chmodels.MLModel) ([]*chv1.MlModel, error) {
	v := make([]*chv1.MlModel, 0, len(models))
	for _, model := range models {
		m, err := MLModelToGRPC(model)
		if err != nil {
			return nil, err
		}

		v = append(v, m)
	}
	return v, nil
}

var (
	mapFormatSchemaTypeToGRPC = map[chmodels.FormatSchemaType]chv1.FormatSchemaType{
		chmodels.ProtobufFormatSchema:  chv1.FormatSchemaType_FORMAT_SCHEMA_TYPE_PROTOBUF,
		chmodels.CapNProtoFormatSchema: chv1.FormatSchemaType_FORMAT_SCHEMA_TYPE_CAPNPROTO,
	}
	mapFormatSchemaTypeFromGRPC = reflectutil.ReverseMap(mapFormatSchemaTypeToGRPC).(map[chv1.FormatSchemaType]chmodels.FormatSchemaType)
)

func FormatSchemaTypeFromGRPC(t chv1.FormatSchemaType) (chmodels.FormatSchemaType, error) {
	v, ok := mapFormatSchemaTypeFromGRPC[t]
	if ok {
		return v, nil
	}

	return v, semerr.InvalidInputf("invalid format schema type %q", t.String())
}

func FormatSchemaToGRPC(schema chmodels.FormatSchema) (*chv1.FormatSchema, error) {
	modelType, ok := mapFormatSchemaTypeToGRPC[schema.Type]
	if !ok {
		return nil, semerr.Internalf("invalid format schema type %q", schema.Type)
	}

	v := &chv1.FormatSchema{
		ClusterId: schema.ClusterID,
		Name:      schema.Name,
		Type:      modelType,
		Uri:       schema.URI,
	}
	return v, nil
}

func FormatSchemasToGRPC(schemas []chmodels.FormatSchema) ([]*chv1.FormatSchema, error) {
	v := make([]*chv1.FormatSchema, 0, len(schemas))
	for _, schema := range schemas {
		s, err := FormatSchemaToGRPC(schema)
		if err != nil {
			return nil, err
		}

		v = append(v, s)
	}

	return v, nil
}

func VersionToGRPC(version logic.Version) *chv1.Version {
	return &chv1.Version{Id: chmodels.CutVersionToMajor(version.ID), Name: version.Name, Deprecated: version.Deprecated,
		UpdatableTo: chmodels.CutVersionsToMajors(version.UpdatableTo)}
}

func VersionsToGRPC(versions []logic.Version) []*chv1.Version {
	v := make([]*chv1.Version, 0, len(versions))
	for _, version := range versions {
		v = append(v, VersionToGRPC(version))
	}
	return v
}

func ShardGroupToGRPC(group chmodels.ShardGroup) *chv1.ShardGroup {
	v := &chv1.ShardGroup{
		ClusterId:   group.ClusterID,
		Name:        group.Name,
		Description: group.Description,
		ShardNames:  group.ShardNames,
	}
	return v
}

func ShardGroupsToGRPC(groups []chmodels.ShardGroup) []*chv1.ShardGroup {
	var v []*chv1.ShardGroup
	for _, group := range groups {
		v = append(v, ShardGroupToGRPC(group))
	}
	return v
}

func ShardGroupUpdateFromGRPC(request *chv1.UpdateClusterShardGroupRequest) (chmodels.UpdateShardGroupArgs, error) {
	mask, err := fieldmaskutils.MaskFromProtoFieldMask(request.UpdateMask, func(s string) string { return s })
	if err != nil {
		return chmodels.UpdateShardGroupArgs{}, semerr.WrapWithInvalidInputf(err, "invalid field mask for UpdateClusterShardGroupRequest")
	}
	groupUpdate := chmodels.UpdateShardGroupArgs{
		Name:      request.ShardGroupName,
		ClusterID: request.ClusterId,
	}
	if _, ok := mask.Filter("description"); ok {
		groupUpdate.Description.Set(request.Description)
	}
	if _, ok := mask.Filter("shard_names"); ok {
		groupUpdate.ShardNames.Set(request.ShardNames)
	}

	return groupUpdate, nil
}

func HostRolesToGRPC(hostRoles []hosts.Role) []chv1.Host_Type {
	var res []chv1.Host_Type

	if len(hostRoles) < 1 {
		return append(res, chv1.Host_TYPE_UNSPECIFIED)
	}

	for _, hostRole := range hostRoles {
		res = append(res, HostRoleToGRPC(hostRole))
	}

	return res
}

var (
	clusterHealthToGRPC = map[clusters.Health]chv1.Cluster_Health{
		clusters.HealthUnknown:  chv1.Cluster_HEALTH_UNKNOWN,
		clusters.HealthAlive:    chv1.Cluster_ALIVE,
		clusters.HealthDead:     chv1.Cluster_DEAD,
		clusters.HealthDegraded: chv1.Cluster_DEGRADED,
	}
)

func ClusterHealthToGRPC(health clusters.Health) chv1.Cluster_Health {
	v, ok := clusterHealthToGRPC[health]
	if !ok {
		return chv1.Cluster_HEALTH_UNKNOWN
	}
	return v
}

var (
	hostHealthToGRPC = map[hosts.Status]chv1.Host_Health{
		hosts.StatusAlive:    chv1.Host_ALIVE,
		hosts.StatusDegraded: chv1.Host_DEGRADED,
		hosts.StatusDead:     chv1.Host_DEAD,
		hosts.StatusUnknown:  chv1.Host_UNKNOWN,
	}
)

func HostHealthToGRPC(hh hosts.Health) chv1.Host_Health {
	v, ok := hostHealthToGRPC[hh.Status]
	if !ok {
		return chv1.Host_UNKNOWN
	}
	return v
}

var (
	serviceTypeMap = map[services.Type]chv1.Service_Type{
		services.TypeClickHouse: chv1.Service_CLICKHOUSE,
		services.TypeZooKeeper:  chv1.Service_ZOOKEEPER,
		services.TypeUnknown:    chv1.Service_TYPE_UNSPECIFIED,
	}
)

func serviceTypeToGRPC(t services.Type) chv1.Service_Type {
	v, ok := serviceTypeMap[t]
	if !ok {
		return chv1.Service_TYPE_UNSPECIFIED
	}
	return v
}

var (
	serviceHealthMap = map[services.Status]chv1.Service_Health{
		services.StatusAlive:   chv1.Service_ALIVE,
		services.StatusDead:    chv1.Service_DEAD,
		services.StatusUnknown: chv1.Service_UNKNOWN,
	}
)

func serviceHealthToGRPC(s services.Status) chv1.Service_Health {
	v, ok := serviceHealthMap[s]
	if !ok {
		return chv1.Service_UNKNOWN
	}
	return v
}

func hostServicesToGRPC(hh hosts.Health) []*chv1.Service {
	var result []*chv1.Service
	for _, service := range hh.Services {
		result = append(result, &chv1.Service{
			Type:   serviceTypeToGRPC(service.Type),
			Health: serviceHealthToGRPC(service.Status),
		})
	}

	return result
}

func cpuMetricToGRPC(cpu *system.CPUMetric) *chv1.Host_CPUMetric {
	if cpu == nil {
		return nil
	}

	return &chv1.Host_CPUMetric{
		Timestamp: cpu.Timestamp,
		Used:      cpu.Used,
	}
}

func memoryMetricToGRPC(memory *system.MemoryMetric) *chv1.Host_MemoryMetric {
	if memory == nil {
		return nil
	}

	return &chv1.Host_MemoryMetric{
		Timestamp: memory.Timestamp,
		Used:      memory.Used,
		Total:     memory.Total,
	}
}

func diskMetricToGRPC(disk *system.DiskMetric) *chv1.Host_DiskMetric {
	if disk == nil {
		return nil
	}

	return &chv1.Host_DiskMetric{
		Timestamp: disk.Timestamp,
		Used:      disk.Used,
		Total:     disk.Total,
	}
}

func HostSystemMetricsToGRPC(sm *system.Metrics) *chv1.Host_SystemMetrics {
	if sm == nil {
		return nil
	}

	return &chv1.Host_SystemMetrics{
		Cpu:    cpuMetricToGRPC(sm.CPU),
		Memory: memoryMetricToGRPC(sm.Memory),
		Disk:   diskMetricToGRPC(sm.Disk),
	}
}

func HostToGRPC(host hosts.HostExtended) *chv1.Host {
	h := &chv1.Host{
		Name:      host.FQDN,
		ClusterId: host.ClusterID,
		ZoneId:    host.ZoneID,
		Type:      HostRolesToGRPC(host.Roles)[0],
		Resources: &chv1.Resources{
			ResourcePresetId: host.ResourcePresetExtID,
			DiskSize:         host.SpaceLimit,
			DiskTypeId:       host.DiskTypeExtID,
		},
		Services:       hostServicesToGRPC(host.Health),
		Health:         HostHealthToGRPC(host.Health),
		System:         HostSystemMetricsToGRPC(host.Health.System),
		ShardName:      host.ShardID.String,
		SubnetId:       host.SubnetID,
		AssignPublicIp: host.AssignPublicIP,
	}
	return h
}

func HostsToGRPC(hosts []hosts.HostExtended) []*chv1.Host {
	var v []*chv1.Host
	for _, host := range hosts {
		v = append(v, HostToGRPC(host))
	}
	return v
}

func BackupToGRPC(backup bmodels.Backup) *chv1.Backup {
	return &chv1.Backup{
		Id:               backup.GlobalBackupID(),
		FolderId:         backup.FolderID,
		SourceClusterId:  backup.SourceClusterID,
		SourceShardNames: backup.SourceShardNames,
		CreatedAt:        grpc.TimeToGRPC(backup.CreatedAt),
		StartedAt:        grpc.OptionalTimeToGRPC(backup.StartedAt),
	}
}

func BackupsToGRPC(backups []bmodels.Backup) []*chv1.Backup {
	v := make([]*chv1.Backup, 0, len(backups))
	for _, b := range backups {
		v = append(v, BackupToGRPC(b))
	}
	return v
}

func CreateShardArgsFromGRPC(req *chv1.AddClusterShardRequest) (clickhouse.CreateShardArgs, error) {
	configSpec, err := ShardConfigSpecFromGRPC(req.GetConfigSpec())
	if err != nil {
		return clickhouse.CreateShardArgs{}, err
	}

	hostSpecs, err := HostSpecsFromGRPC(req.GetHostSpecs())
	if err != nil {
		return clickhouse.CreateShardArgs{}, err
	}

	args := clickhouse.CreateShardArgs{
		Name:        req.GetShardName(),
		ConfigSpec:  configSpec,
		HostSpecs:   hostSpecs,
		ShardGroups: req.GetShardGroupNames(),
		CopySchema:  req.CopySchema.GetValue(),
	}

	return args, nil
}

func ShardConfigSpecFromGRPC(spec *chv1.ShardConfigSpec) (clickhouse.ShardConfigSpec, error) {
	shardConfig := clickhouse.ShardConfigSpec{}

	if spec == nil || spec.GetClickhouse() == nil {
		return shardConfig, nil
	}

	chSpec := spec.GetClickhouse()

	chConfig, err := ClickHouseConfigFromGRPC(chSpec.GetConfig())
	if err != nil {
		return shardConfig, err
	}
	err = chConfig.ValidateAndSane()
	if err != nil {
		return shardConfig, err
	}

	shardConfig.Config = chConfig

	if res := chSpec.GetResources(); res != nil {
		shardConfig.Resources = grpc.ResourcesFromGRPC(res, grpc.AllPaths())
	}

	weight := grpc.OptionalInt64FromGRPC(chSpec.GetWeight())
	if weight.Valid {
		shardConfig.Weight = weight
	}

	return shardConfig, nil
}

func operationWithEmptyResult(opGrpc *operation.Operation) (*operation.Operation, error) {
	opGrpc.Result = &operation.Operation_Response{}
	return opGrpc, nil
}

func operationWithResult(opGrpc *operation.Operation, message proto.Message) (*operation.Operation, error) {
	r, err := ptypes.MarshalAny(message)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal operation %+v response: %w", opGrpc, err)
	}
	opGrpc.Result = &operation.Operation_Response{Response: r}
	return opGrpc, nil
}

func operationWithClusterResult(ctx context.Context, opGrpc *operation.Operation, ch clickhouse.ClickHouse, cid string, saltEnvMapper grpc.SaltEnvMapper) (*operation.Operation, error) {
	cluster, err := ch.MDBCluster(ctx, cid)
	if err != nil {
		return opGrpc, nil
	}
	return operationWithResult(opGrpc, ClusterToGRPC(cluster, saltEnvMapper))
}

func operationWithShardResult(ctx context.Context, opGrpc *operation.Operation, ch clickhouse.ClickHouse, cid, shardName string) (*operation.Operation, error) {
	s, err := ch.GetShard(ctx, cid, shardName)
	if err != nil {
		return opGrpc, nil
	}
	return operationWithResult(opGrpc, ShardToGRPC(s))
}

func operationWithShardGroupResult(ctx context.Context, opGrpc *operation.Operation, ch clickhouse.ClickHouse, cid, shardGroupName string) (*operation.Operation, error) {
	sg, err := ch.ShardGroup(ctx, cid, shardGroupName)
	if err != nil {
		return opGrpc, nil
	}
	return operationWithResult(opGrpc, ShardGroupToGRPC(sg))
}

func operationWithUserResult(ctx context.Context, opGrpc *operation.Operation, ch clickhouse.ClickHouse, cid, userName string) (*operation.Operation, error) {
	user, err := ch.User(ctx, cid, userName)
	if err != nil {
		return opGrpc, nil
	}
	return operationWithResult(opGrpc, UserToGRPC(user))
}

func operationWithDatabaseResult(ctx context.Context, opGrpc *operation.Operation, ch clickhouse.ClickHouse, cid, dbName string) (*operation.Operation, error) {
	db, err := ch.Database(ctx, cid, dbName)
	if err != nil {
		return opGrpc, nil
	}
	return operationWithResult(opGrpc, DatabaseToGRPC(db))
}

func operationWithFormatSchemaResult(ctx context.Context, opGrpc *operation.Operation, ch clickhouse.ClickHouse, cid, formatSchemaName string) (*operation.Operation, error) {
	fs, err := ch.FormatSchema(ctx, cid, formatSchemaName)
	if err != nil {
		return opGrpc, nil
	}
	message, err := FormatSchemaToGRPC(fs)
	if err != nil {
		return opGrpc, nil
	}
	return operationWithResult(opGrpc, message)
}

func operationWithMLModelResult(ctx context.Context, opGrpc *operation.Operation, ch clickhouse.ClickHouse, cid, mlModelName string) (*operation.Operation, error) {
	model, err := ch.MLModel(ctx, cid, mlModelName)
	if err != nil {
		return opGrpc, nil
	}
	message, err := MLModelToGRPC(model)
	if err != nil {
		return opGrpc, nil
	}
	return operationWithResult(opGrpc, message)
}

func operationToGRPC(ctx context.Context, op operations.Operation, ch clickhouse.ClickHouse, saltEnvMapper grpc.SaltEnvMapper, l log.Logger) (*operation.Operation, error) {
	opGrpc, err := grpc.OperationToGRPC(ctx, op, l)
	if err != nil {
		return nil, err
	}

	if op.Status != operations.StatusDone {
		return opGrpc, nil
	}

	switch v := op.MetaData.(type) {

	// All delete operations must return empty metadata response
	case chmodels.MetadataDeleteCluster,
		chmodels.MetadataDeleteClusterShardGroup,
		chmodels.MetadataDeleteDatabase,
		chmodels.MetadataDeleteFormatSchema,
		chmodels.MetadataDeleteHosts,
		chmodels.MetadataDeleteMLModel,
		chmodels.MetadataDeleteShard,
		chmodels.MetadataDeleteUser:
		return operationWithEmptyResult(opGrpc)

	case chmodels.MetadataCreateCluster,
		chmodels.MetadataModifyCluster,
		chmodels.MetadataAddZookeeper,
		chmodels.MetadataStartCluster,
		chmodels.MetadataStopCluster,
		chmodels.MetadataMoveCluster,
		chmodels.MetadataBackupCluster,
		chmodels.MetadataRestoreCluster,
		chmodels.MetadataCreateDictionary,
		chmodels.MetadataUpdateDictionary,
		chmodels.MetadataDeleteDictionary,
		chmodels.MetadataRescheduleMaintenance:
		return operationWithClusterResult(ctx, opGrpc, ch, op.ClusterID, saltEnvMapper)

	case chmodels.MetadataCreateHosts:
		return operationWithEmptyResult(opGrpc)

	// TODO: add MetadataModifyShard when it's ready
	case chmodels.MetadataCreateShard:
		return operationWithShardResult(ctx, opGrpc, ch, op.ClusterID, v.ShardName)

	case chmodels.MetadataCreateClusterShardGroup:
		return operationWithShardGroupResult(ctx, opGrpc, ch, op.ClusterID, v.ShardGroupName)
	case chmodels.MetadataUpdateClusterShardGroup:
		return operationWithShardGroupResult(ctx, opGrpc, ch, op.ClusterID, v.ShardGroupName)

	case chmodels.MetadataCreateUser:
		return operationWithUserResult(ctx, opGrpc, ch, op.ClusterID, v.UserName)
	case chmodels.MetadataModifyUser:
		return operationWithUserResult(ctx, opGrpc, ch, op.ClusterID, v.UserName)
	case chmodels.MetadataGrantUserPermission:
		return operationWithUserResult(ctx, opGrpc, ch, op.ClusterID, v.UserName)
	case chmodels.MetadataRevokeUserPermission:
		return operationWithUserResult(ctx, opGrpc, ch, op.ClusterID, v.UserName)

	case chmodels.MetadataCreateDatabase:
		return operationWithDatabaseResult(ctx, opGrpc, ch, op.ClusterID, v.DatabaseName)

	case chmodels.MetadataCreateFormatSchema:
		return operationWithFormatSchemaResult(ctx, opGrpc, ch, op.ClusterID, v.FormatSchemaName)
	case chmodels.MetadataUpdateFormatSchema:
		return operationWithFormatSchemaResult(ctx, opGrpc, ch, op.ClusterID, v.FormatSchemaName)

	case chmodels.MetadataCreateMLModel:
		return operationWithMLModelResult(ctx, opGrpc, ch, op.ClusterID, v.MLModelName)
	case chmodels.MetadataUpdateMLModel:
		return operationWithMLModelResult(ctx, opGrpc, ch, op.ClusterID, v.MLModelName)

	default:
		return operationWithEmptyResult(opGrpc)
	}
}

func OperationsToGRPC(ctx context.Context, ops []operations.Operation, ch clickhouse.ClickHouse, saltEnvMapper grpc.SaltEnvMapper, l log.Logger) ([]*operation.Operation, error) {
	res := make([]*operation.Operation, 0, len(ops))
	for _, op := range ops {
		opgrpc, err := operationToGRPC(ctx, op, ch, saltEnvMapper, l)
		if err != nil {
			return nil, err
		}

		res = append(res, opgrpc)
	}
	return res, nil
}

func ResourcePresetToGRPC(rp chmodels.ResourcePreset) *chv1.ResourcePreset {
	var res = &chv1.ResourcePreset{
		Id:        rp.ID,
		ZoneIds:   rp.ZoneIds,
		Cores:     rp.Cores,
		Memory:    rp.Memory,
		HostTypes: HostRolesToGRPC(rp.HostRoles),
	}
	return res
}

func ResourcePresetsToGRPC(rps []chmodels.ResourcePreset) []*chv1.ResourcePreset {
	resourcePresets := rps
	sort.Slice(resourcePresets, func(i, j int) bool {
		return resourcePresets[i].ID < resourcePresets[j].ID
	})

	var res []*chv1.ResourcePreset
	for _, rp := range resourcePresets {
		res = append(res, ResourcePresetToGRPC(rp))
	}
	return res
}

func MonitoringToGRPC(monitoring monitoring.Monitoring) []*chv1.Monitoring {
	var res []*chv1.Monitoring

	for _, chart := range monitoring.Charts {
		res = append(res,
			&chv1.Monitoring{
				Name:        chart.Name,
				Description: chart.Description,
				Link:        chart.Link,
			},
		)
	}

	return res
}

var (
	mapMaintenanceWindowDayStringToGRPC = map[string]chv1.WeeklyMaintenanceWindow_WeekDay{
		"MON": chv1.WeeklyMaintenanceWindow_MON,
		"TUE": chv1.WeeklyMaintenanceWindow_TUE,
		"WED": chv1.WeeklyMaintenanceWindow_WED,
		"THU": chv1.WeeklyMaintenanceWindow_THU,
		"FRI": chv1.WeeklyMaintenanceWindow_FRI,
		"SAT": chv1.WeeklyMaintenanceWindow_SAT,
		"SUN": chv1.WeeklyMaintenanceWindow_SUN,
	}
	mapMaintenanceWindowDayStringFromGRPC = reflectutil.ReverseMap(mapMaintenanceWindowDayStringToGRPC).(map[chv1.WeeklyMaintenanceWindow_WeekDay]string)
)

func MaintenanceWindowToGRPC(window clusters.MaintenanceWindow) *chv1.MaintenanceWindow {
	if window.Anytime() {
		return &chv1.MaintenanceWindow{
			Policy: &chv1.MaintenanceWindow_Anytime{
				Anytime: &chv1.AnytimeMaintenanceWindow{},
			},
		}
	}

	day, found := mapMaintenanceWindowDayStringToGRPC[window.Day]
	if !found {
		day = chv1.WeeklyMaintenanceWindow_WEEK_DAY_UNSPECIFIED
	}

	return &chv1.MaintenanceWindow{
		Policy: &chv1.MaintenanceWindow_WeeklyMaintenanceWindow{
			WeeklyMaintenanceWindow: &chv1.WeeklyMaintenanceWindow{
				Day:  day,
				Hour: int64(window.Hour),
			},
		},
	}
}

func MaintenanceWindowFromGRPC(window *chv1.MaintenanceWindow) (clusters.MaintenanceWindow, error) {
	if window == nil {
		return clusters.NewAnytimeMaintenanceWindow(), nil
	}

	switch p := window.GetPolicy().(type) {
	case *chv1.MaintenanceWindow_Anytime:
		return clusters.NewAnytimeMaintenanceWindow(), nil
	case *chv1.MaintenanceWindow_WeeklyMaintenanceWindow:
		day, ok := mapMaintenanceWindowDayStringFromGRPC[p.WeeklyMaintenanceWindow.GetDay()]
		if !ok {
			return clusters.MaintenanceWindow{}, semerr.InvalidInputf("invalid maintenance window data: %q", p.WeeklyMaintenanceWindow.GetDay().String())
		}

		return clusters.NewWeeklyMaintenanceWindow(day, int(p.WeeklyMaintenanceWindow.GetHour())), nil
	default:
		return clusters.MaintenanceWindow{}, semerr.InvalidInputf("invalid maintenance window type")
	}
}

func MaintenanceOperationToGRPC(op clusters.MaintenanceOperation) *chv1.MaintenanceOperation {
	if !op.Valid() {
		return nil
	}

	return &chv1.MaintenanceOperation{
		Info:                      op.Info,
		DelayedUntil:              grpc.TimeToGRPC(op.DelayedUntil),
		LatestMaintenanceTime:     grpc.TimeToGRPC(op.LatestMaintenanceTime),
		NextMaintenanceWindowTime: grpc.TimePtrToGRPC(op.NearestMaintenanceWindow),
	}
}

func ShardToGRPC(shard chmodels.Shard) *chv1.Shard {
	shardConfig := chv1.ShardConfig{
		Clickhouse: &chv1.ShardConfig_Clickhouse{
			Config: &config.ClickhouseConfigSet{
				EffectiveConfig: ClickhouseConfigToGRPC(shard.Config.ConfigSet.Effective),
				UserConfig:      ClickhouseConfigToGRPC(shard.Config.ConfigSet.User),
				DefaultConfig:   ClickhouseConfigToGRPC(shard.Config.ConfigSet.Default),
			},
			Resources: ClusterResourcesToGRPC(shard.Config.Resources),
			Weight: &wrapperspb.Int64Value{
				Value: shard.Config.Weight,
			},
		},
	}

	res := chv1.Shard{
		Name:      shard.Name,
		ClusterId: shard.ClusterID,
		Config:    &shardConfig,
	}

	return &res
}

func ShardsToGRPC(shards []chmodels.Shard) []*chv1.Shard {
	res := make([]*chv1.Shard, len(shards))
	for i, shard := range shards {
		res[i] = ShardToGRPC(shard)
	}
	return res
}

var (
	mapRescheduleTypeFromGRPC = map[chv1.RescheduleMaintenanceRequest_RescheduleType]clusters.RescheduleType{
		chv1.RescheduleMaintenanceRequest_RESCHEDULE_TYPE_UNSPECIFIED: clusters.RescheduleTypeUnspecified,
		chv1.RescheduleMaintenanceRequest_IMMEDIATE:                   clusters.RescheduleTypeImmediate,
		chv1.RescheduleMaintenanceRequest_NEXT_AVAILABLE_WINDOW:       clusters.RescheduleTypeNextAvailableWindow,
		chv1.RescheduleMaintenanceRequest_SPECIFIC_TIME:               clusters.RescheduleTypeSpecificTime,
	}
)

func RescheduleTypeFromGRPC(rescheduleType chv1.RescheduleMaintenanceRequest_RescheduleType) (clusters.RescheduleType, error) {
	v, ok := mapRescheduleTypeFromGRPC[rescheduleType]
	if !ok {
		return clusters.RescheduleTypeUnspecified, semerr.InvalidInput("unknown reschedule type")
	}

	return v, nil
}

func ClustersConfigToGRPC(clusterConfig chmodels.ClickhouseClustersConfig) *chv1console.ClustersConfig {
	return &chv1console.ClustersConfig{
		ClusterName:       NameValidatorToGRPC(clusterConfig.ClusterNameValidator),
		DbName:            NameValidatorToGRPC(clusterConfig.DatabaseNameValidator),
		UserName:          NameValidatorToGRPC(clusterConfig.UserNameValidator),
		Password:          NameValidatorToGRPC(clusterConfig.PasswordValidator),
		MlModelName:       NameValidatorToGRPC(clusterConfig.MLModelName),
		MlModelUri:        NameValidatorToGRPC(clusterConfig.MLModelURI),
		FormatSchemaName:  NameValidatorToGRPC(clusterConfig.FormatSchemaName),
		FormatSchemaUri:   NameValidatorToGRPC(clusterConfig.FormatSchemaURI),
		HostCountLimits:   HostCountLimitsToGRPC(clusterConfig.HostCountLimits),
		HostTypes:         HostTypeSToGRPC(clusterConfig.HostTypes),
		Versions:          clusterConfig.Versions,
		AvailableVersions: VersionsToGRPC(clusterConfig.AvailableVersions),
		DefaultVersion:    clusterConfig.DefaultVersion.ID,
	}
}

func NameValidatorToGRPC(validator consolemodels.NameValidator) *chv1console.NameValidator {
	return &chv1console.NameValidator{
		Regexp:    validator.Regexp,
		Min:       validator.MinLength,
		Max:       validator.MaxLength,
		Blacklist: validator.ForbiddenNames,
	}
}

func HostCountLimitsToGRPC(limits consolemodels.HostCountLimits) *chv1console.ClustersConfig_HostCountLimits {
	return &chv1console.ClustersConfig_HostCountLimits{
		MinHostCount:         limits.MinHostCount,
		MaxHostCount:         limits.MaxHostCount,
		HostCountPerDiskType: HostCountPerDiskTypeToGRPC(limits.HostCountPerDiskType),
	}
}

func HostCountPerDiskTypeToGRPC(hostCount []consolemodels.DiskTypeHostCount) (hostCountRes []*chv1console.ClustersConfig_HostCountLimits_HostCountPerDiskType) {
	for _, hc := range hostCount {
		hostCountRes = append(hostCountRes, DiskTypeHostCountToGRPC(hc))
	}

	return hostCountRes
}

func DiskTypeHostCountToGRPC(hostCount consolemodels.DiskTypeHostCount) *chv1console.ClustersConfig_HostCountLimits_HostCountPerDiskType {
	return &chv1console.ClustersConfig_HostCountLimits_HostCountPerDiskType{
		DiskTypeId:   hostCount.DiskTypeID,
		MinHostCount: hostCount.MinHostCount,
	}
}

func HostTypeSToGRPC(hostTypes []consolemodels.HostType) (hostTypesRes []*chv1console.ClustersConfig_HostType) {
	for _, hostType := range hostTypes {
		hostTypesRes = append(hostTypesRes, HostTypeToGRPC(hostType))
	}

	return hostTypesRes
}

func HostTypeToGRPC(hostType consolemodels.HostType) *chv1console.ClustersConfig_HostType {
	return &chv1console.ClustersConfig_HostType{
		Type:             HostRoleToGRPC(hostType.Type),
		ResourcePresets:  HostTypeResourcePresetsToGRPC(hostType.ResourcePresets),
		DefaultResources: HostTypeDefaultResourcesToGRPC(hostType.DefaultResources),
	}
}

func HostTypeResourcePresetsToGRPC(presets []consolemodels.HostTypeResourcePreset) (presetsRes []*chv1console.ClustersConfig_HostType_ResourcePreset) {
	for _, preset := range presets {
		presetsRes = append(presetsRes, HostTypeResourcePresetToGRPC(preset))
	}

	return presetsRes
}

func HostTypeResourcePresetToGRPC(preset consolemodels.HostTypeResourcePreset) *chv1console.ClustersConfig_HostType_ResourcePreset {
	return &chv1console.ClustersConfig_HostType_ResourcePreset{
		PresetId:       preset.ID,
		CpuLimit:       preset.CPULimit,
		MemoryLimit:    preset.RAMLimit,
		Generation:     preset.Generation,
		GenerationName: preset.GenerationName,
		Type:           preset.FlavorType,
		CpuFraction:    preset.CPUFraction,
		Decomissioning: preset.Decomissioning,
		Zones:          HostTypeZonesToGRPC(preset.Zones),
	}
}

func HostTypeZonesToGRPC(zones []consolemodels.HostTypeZone) (zonesRes []*chv1console.ClustersConfig_HostType_ResourcePreset_Zone) {
	for _, zone := range zones {
		zonesRes = append(zonesRes, HostTypeZoneToGRPC(zone))
	}

	return zonesRes
}

func HostTypeZoneToGRPC(zone consolemodels.HostTypeZone) *chv1console.ClustersConfig_HostType_ResourcePreset_Zone {
	return &chv1console.ClustersConfig_HostType_ResourcePreset_Zone{
		ZoneId:    zone.ID,
		DiskTypes: HostTypeDiskTypeSToGRPC(zone.DiskTypes),
	}
}

func HostTypeDiskTypeSToGRPC(diskTypes []consolemodels.HostTypeDiskType) (diskTypesRes []*chv1console.ClustersConfig_HostType_ResourcePreset_Zone_DiskType) {
	for _, diskType := range diskTypes {
		diskTypesRes = append(diskTypesRes, HostTypeDiskTypeToGRPC(diskType))
	}

	return diskTypesRes
}

func HostTypeDiskTypeToGRPC(diskType consolemodels.HostTypeDiskType) *chv1console.ClustersConfig_HostType_ResourcePreset_Zone_DiskType {
	res := chv1console.ClustersConfig_HostType_ResourcePreset_Zone_DiskType{
		DiskTypeId: diskType.ID,
		MinHosts:   diskType.MinHosts,
		MaxHosts:   diskType.MaxHosts,
	}

	if diskType.SizeRange != nil && len(diskType.Sizes) == 0 {
		res.DiskSize = &chv1console.ClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizeRange_{
			DiskSizeRange: &chv1console.ClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizeRange{
				Min: diskType.SizeRange.Lower,
				Max: diskType.SizeRange.Upper,
			},
		}
	} else if len(diskType.Sizes) > 0 && diskType.SizeRange == nil {
		res.DiskSize = &chv1console.ClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizes_{
			DiskSizes: &chv1console.ClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizes{
				Sizes: diskType.Sizes,
			},
		}
	}

	return &res
}

func HostTypeDefaultResourcesToGRPC(defaultResources consolemodels.DefaultResources) *chv1console.ClustersConfig_HostType_DefaultResources {
	return &chv1console.ClustersConfig_HostType_DefaultResources{
		ResourcePresetId: defaultResources.ResourcePresetExtID,
		DiskTypeId:       defaultResources.DiskTypeExtID,
		DiskSize:         defaultResources.DiskSize,
		Generation:       defaultResources.Generation,
		GenerationName:   defaultResources.GenerationName,
	}
}

func BillingEstimateToGRPC(estimate consolemodels.BillingEstimate) *chv1console.BillingEstimate {
	metrics := make([]*chv1console.BillingMetric, 0, len(estimate.Metrics))
	for _, metric := range estimate.Metrics {
		tags := &chv1console.BillingMetric_BillingTags{
			PublicIp:                        metric.Tags.PublicIP,
			DiskTypeId:                      metric.Tags.DiskTypeID,
			ClusterType:                     metric.Tags.ClusterType,
			DiskSize:                        metric.Tags.DiskSize,
			ResourcePresetId:                metric.Tags.ResourcePresetID,
			PlatformId:                      metric.Tags.PlatformID,
			Cores:                           metric.Tags.Cores,
			CoreFraction:                    metric.Tags.CoreFraction,
			Memory:                          metric.Tags.Memory,
			SoftwareAcceleratedNetworkCores: metric.Tags.SoftwareAcceleratedNetworkCores,
			Roles:                           metric.Tags.Roles,
			Online:                          metric.Tags.Online,
			OnDedicatedHost:                 metric.Tags.OnDedicatedHost,
		}

		metrics = append(metrics, &chv1console.BillingMetric{
			FolderId: metric.FolderID,
			Schema:   metric.Schema,
			Tags:     tags,
		})
	}

	return &chv1console.BillingEstimate{
		Metrics: metrics,
	}
}
