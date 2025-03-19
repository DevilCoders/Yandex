package opensearch

import (
	"context"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes"

	protov1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/opensearch/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/osmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/system"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	mapHostRoleToGRPC = map[hosts.Role]protov1.Host_Type{
		hosts.RoleOpenSearchDataNode:   protov1.Host_DATA_NODE,
		hosts.RoleOpenSearchMasterNode: protov1.Host_MASTER_NODE,
	}
	mapHostRoleFromGRPC = reflectutil.ReverseMap(mapHostRoleToGRPC).(map[protov1.Host_Type]hosts.Role)
)

func HostRoleToGRPC(role hosts.Role) protov1.Host_Type {
	v, ok := mapHostRoleToGRPC[role]
	if !ok {
		return protov1.Host_TYPE_UNSPECIFIED
	}

	return v
}

func HostRoleFromGRPC(ht protov1.Host_Type) (hosts.Role, error) {
	v, ok := mapHostRoleFromGRPC[ht]
	if !ok {
		return hosts.RoleUnknown, semerr.InvalidInput("unknown host role")
	}

	return v, nil
}

func HostRolesToGRPC(hostRoles []hosts.Role) protov1.Host_Type {
	if len(hostRoles) < 1 {
		return protov1.Host_TYPE_UNSPECIFIED
	}

	return HostRoleToGRPC(hostRoles[0])
}

var (
	statusToGRCP = map[clusters.Status]protov1.Cluster_Status{
		clusters.StatusCreating:             protov1.Cluster_CREATING,
		clusters.StatusCreateError:          protov1.Cluster_ERROR,
		clusters.StatusRunning:              protov1.Cluster_RUNNING,
		clusters.StatusModifying:            protov1.Cluster_UPDATING,
		clusters.StatusModifyError:          protov1.Cluster_ERROR,
		clusters.StatusStopping:             protov1.Cluster_STOPPING,
		clusters.StatusStopped:              protov1.Cluster_STOPPED,
		clusters.StatusStopError:            protov1.Cluster_ERROR,
		clusters.StatusStarting:             protov1.Cluster_STARTING,
		clusters.StatusStartError:           protov1.Cluster_ERROR,
		clusters.StatusMaintainingOffline:   protov1.Cluster_UPDATING,
		clusters.StatusMaintainOfflineError: protov1.Cluster_ERROR,
	}
)

func StatusToGRPC(status clusters.Status) protov1.Cluster_Status {
	v, ok := statusToGRCP[status]
	if !ok {
		return protov1.Cluster_STATUS_UNKNOWN
	}
	return v
}

var (
	clusterHealthToGRPC = map[clusters.Health]protov1.Cluster_Health{
		clusters.HealthAlive:    protov1.Cluster_ALIVE,
		clusters.HealthDegraded: protov1.Cluster_DEGRADED,
		clusters.HealthDead:     protov1.Cluster_DEAD,
		clusters.HealthUnknown:  protov1.Cluster_HEALTH_UNKNOWN,
	}
)

func ClusterHealthToGRPC(env clusters.Health) protov1.Cluster_Health {
	v, ok := clusterHealthToGRPC[env]
	if !ok {
		return protov1.Cluster_HEALTH_UNKNOWN
	}
	return v
}

var (
	hostHealthToGRPC = map[hosts.Status]protov1.Host_Health{
		hosts.StatusAlive:    protov1.Host_ALIVE,
		hosts.StatusDegraded: protov1.Host_DEGRADED,
		hosts.StatusDead:     protov1.Host_DEAD,
		hosts.StatusUnknown:  protov1.Host_UNKNOWN,
	}
)

func HostHealthToGRPC(hh hosts.Health) protov1.Host_Health {
	v, ok := hostHealthToGRPC[hh.Status]
	if !ok {
		return protov1.Host_UNKNOWN
	}
	return v
}

func ClusterToGRPC(cluster osmodels.Cluster, saltEnvMapper grpc.SaltEnvMapper) *protov1.Cluster {
	v := &protov1.Cluster{
		Id:                 cluster.ClusterID,
		FolderId:           cluster.FolderExtID,
		Name:               cluster.Name,
		Description:        cluster.Description,
		Labels:             cluster.Labels,
		NetworkId:          cluster.NetworkID,
		SecurityGroupIds:   cluster.SecurityGroupIDs,
		CreatedAt:          grpc.TimeToGRPC(cluster.CreatedAt),
		Environment:        protov1.Cluster_Environment(saltEnvMapper.ToGRPC(cluster.Environment)),
		Status:             StatusToGRPC(cluster.Status),
		Health:             ClusterHealthToGRPC(cluster.Health),
		Config:             ConfigToGRPC(cluster.Config),
		ServiceAccountId:   cluster.ServiceAccountID,
		DeletionProtection: cluster.DeletionProtection,
		MaintenanceWindow:  MaintenanceWindowToGRPC(cluster.MaintenanceInfo.Window),
		PlannedOperation:   MaintenanceOperationToGRPC(cluster.MaintenanceInfo.Operation),
	}
	v.Monitoring = make([]*protov1.Monitoring, 0, len(cluster.Monitoring.Charts))
	for _, chart := range cluster.Monitoring.Charts {
		mon := &protov1.Monitoring{
			Name:        chart.Name,
			Description: chart.Description,
			Link:        chart.Link,
		}
		v.Monitoring = append(v.Monitoring, mon)
	}
	return v
}

func ClustersToGRPC(clusters []osmodels.Cluster, saltEnvMapper grpc.SaltEnvMapper) []*protov1.Cluster {
	var v []*protov1.Cluster
	for _, cluster := range clusters {
		v = append(v, ClusterToGRPC(cluster, saltEnvMapper))
	}
	return v
}

func HostsFromGRPC(hostSpec []*protov1.HostSpec) ([]osmodels.Host, error) {
	var hosts []osmodels.Host
	for _, host := range hostSpec {
		hostRole, err := HostRoleFromGRPC(host.Type)
		if err != nil {
			return []osmodels.Host{}, err
		}
		var subnetID optional.String
		if host.SubnetId == "" {
			subnetID = optional.String{}
		} else {
			subnetID = optional.NewString(host.SubnetId)
		}
		hosts = append(hosts, osmodels.Host{
			ZoneID:         host.ZoneId,
			Role:           hostRole,
			AssignPublicIP: host.AssignPublicIp,
			ShardName:      host.ShardName,
			SubnetID:       subnetID,
		})
	}
	return hosts, nil
}

func cpuMetricToGRPC(cpu *system.CPUMetric) *protov1.Host_CPUMetric {
	if cpu == nil {
		return nil
	}

	return &protov1.Host_CPUMetric{
		Timestamp: cpu.Timestamp,
		Used:      cpu.Used,
	}
}

func memoryMetricToGRPC(memory *system.MemoryMetric) *protov1.Host_MemoryMetric {
	if memory == nil {
		return nil
	}

	return &protov1.Host_MemoryMetric{
		Timestamp: memory.Timestamp,
		Used:      memory.Used,
		Total:     memory.Total,
	}
}

func diskMetricToGRPC(disk *system.DiskMetric) *protov1.Host_DiskMetric {
	if disk == nil {
		return nil
	}

	return &protov1.Host_DiskMetric{
		Timestamp: disk.Timestamp,
		Used:      disk.Used,
		Total:     disk.Total,
	}
}

func HostSystemMetricsToGRPC(sm *system.Metrics) *protov1.Host_SystemMetrics {
	if sm == nil {
		return nil
	}

	return &protov1.Host_SystemMetrics{
		Cpu:    cpuMetricToGRPC(sm.CPU),
		Memory: memoryMetricToGRPC(sm.Memory),
		Disk:   diskMetricToGRPC(sm.Disk),
	}
}

func HostToGRPC(host hosts.HostExtended) *protov1.Host {
	return &protov1.Host{
		Name:      host.FQDN,
		ClusterId: host.ClusterID,
		ZoneId:    host.ZoneID,
		Type:      HostRolesToGRPC(host.Roles),
		Resources: &protov1.Resources{
			ResourcePresetId: host.ResourcePresetExtID,
			DiskSize:         host.SpaceLimit,
			DiskTypeId:       host.DiskTypeExtID,
		},
		Health:         HostHealthToGRPC(host.Health),
		System:         HostSystemMetricsToGRPC(host.Health.System),
		SubnetId:       host.SubnetID,
		AssignPublicIp: host.AssignPublicIP,
	}
}

func HostsToGRPC(hosts []hosts.HostExtended) []*protov1.Host {
	var v []*protov1.Host
	for _, host := range hosts {
		v = append(v, HostToGRPC(host))
	}
	return v
}

func ConfigFromGRPC(configSpec *protov1.ConfigSpec, supportedVersions osmodels.SupportedVersions) (osmodels.ClusterConfigSpec, error) {

	version := osmodels.OptionalVersion{}
	if configSpec.GetVersion() != "" {
		v, err := supportedVersions.ParseVersion(configSpec.GetVersion())
		if err != nil {
			return osmodels.ClusterConfigSpec{}, semerr.InvalidInput(err.Error())
		}
		version.Set(v)
	}

	adminPassword := osmodels.OptionalSecretString{}
	if configSpec.AdminPassword != "" {
		adminPassword.Set(configSpec.AdminPassword)
	}

	access := AccessFromGRPC(configSpec.Access)

	result := osmodels.ClusterConfigSpec{
		Version:       version,
		AdminPassword: adminPassword,
		Access:        access,
	}

	dataResources := grpc.ResourcesFromGRPC(configSpec.GetOpensearchSpec().GetDataNode().GetResources(), grpc.AllPaths())

	masterNode := osmodels.OptionalMasterNodeSpec{}
	masterResourcesGRPC := configSpec.GetOpensearchSpec().GetMasterNode().GetResources()
	if masterResourcesGRPC != nil {
		masterNode.Set(osmodels.MasterNodeSpec{Resources: grpc.ResourcesFromGRPC(masterResourcesGRPC, grpc.AllPaths())})
		masterNode.Valid = true
	}

	result.Config.Set(
		osmodels.ConfigSpec{
			DataNode: osmodels.DataNodeSpec{
				Resources: dataResources,
			},
			MasterNode: masterNode,
			Plugins:    configSpec.GetOpensearchSpec().GetPlugins(),
		})

	return result, nil
}

var maintenanceWindowDayStringToGRPCMap = map[string]protov1.WeeklyMaintenanceWindow_WeekDay{
	"MON": protov1.WeeklyMaintenanceWindow_MON,
	"TUE": protov1.WeeklyMaintenanceWindow_TUE,
	"WED": protov1.WeeklyMaintenanceWindow_WED,
	"THU": protov1.WeeklyMaintenanceWindow_THU,
	"FRI": protov1.WeeklyMaintenanceWindow_FRI,
	"SAT": protov1.WeeklyMaintenanceWindow_SAT,
	"SUN": protov1.WeeklyMaintenanceWindow_SUN,
}

func MaintenanceWindowToGRPC(window clusters.MaintenanceWindow) *protov1.MaintenanceWindow {
	if window.Anytime() {
		return &protov1.MaintenanceWindow{
			Policy: &protov1.MaintenanceWindow_Anytime{
				Anytime: &protov1.AnytimeMaintenanceWindow{},
			},
		}
	}

	return &protov1.MaintenanceWindow{
		Policy: &protov1.MaintenanceWindow_WeeklyMaintenanceWindow{
			WeeklyMaintenanceWindow: &protov1.WeeklyMaintenanceWindow{
				Day:  mapStringMaintenanceWindowDayToGRPC(window.Day),
				Hour: int64(window.Hour),
			},
		},
	}
}

func MaintenanceWindowFromGRPC(window *protov1.MaintenanceWindow) (clusters.MaintenanceWindow, error) {
	if window == nil {
		return clusters.NewAnytimeMaintenanceWindow(), nil
	}

	switch p := window.GetPolicy().(type) {
	case *protov1.MaintenanceWindow_Anytime:
		return clusters.NewAnytimeMaintenanceWindow(), nil
	case *protov1.MaintenanceWindow_WeeklyMaintenanceWindow:
		day := p.WeeklyMaintenanceWindow.GetDay()

		dayStr, ok := mapGRPCMaintenanceWindowDayToString(day)
		if !ok {
			return clusters.MaintenanceWindow{}, semerr.InvalidInputf("invalid day %q", day)
		}

		return clusters.NewWeeklyMaintenanceWindow(dayStr, int(p.WeeklyMaintenanceWindow.GetHour())), nil
	default:
		return clusters.MaintenanceWindow{}, semerr.InvalidInputf("invalid type")
	}
}

func mapGRPCMaintenanceWindowDayToString(day protov1.WeeklyMaintenanceWindow_WeekDay) (string, bool) {
	rev := reflectutil.ReverseMap(maintenanceWindowDayStringToGRPCMap).(map[protov1.WeeklyMaintenanceWindow_WeekDay]string)
	v, ok := rev[day]
	return v, ok
}

func mapStringMaintenanceWindowDayToGRPC(dayStr string) protov1.WeeklyMaintenanceWindow_WeekDay {
	day, ok := maintenanceWindowDayStringToGRPCMap[dayStr]
	if !ok {
		day = protov1.WeeklyMaintenanceWindow_WEEK_DAY_UNSPECIFIED
	}
	return day
}

func MaintenanceOperationToGRPC(operation clusters.MaintenanceOperation) *protov1.MaintenanceOperation {
	if !operation.Valid() {
		return nil
	}

	return &protov1.MaintenanceOperation{
		Info:                      operation.Info,
		DelayedUntil:              grpc.TimeToGRPC(operation.DelayedUntil),
		LatestMaintenanceTime:     grpc.TimeToGRPC(operation.LatestMaintenanceTime),
		NextMaintenanceWindowTime: grpc.TimePtrToGRPC(operation.NearestMaintenanceWindow),
	}
}

func ResourcesToGRPC(res models.ClusterResources) *protov1.Resources {
	return &protov1.Resources{
		ResourcePresetId: res.ResourcePresetExtID,
		DiskTypeId:       res.DiskTypeExtID,
		DiskSize:         res.DiskSize,
	}
}

// TODO: handle version config
func ConfigToGRPC(clusterConfig osmodels.ClusterConfig) *protov1.ClusterConfig {
	result := &protov1.ClusterConfig{
		Version: clusterConfig.Version.ID,
	}

	config := clusterConfig.Config
	result.Opensearch = &protov1.OpenSearch{
		DataNode: &protov1.OpenSearch_DataNode{
			Resources: ResourcesToGRPC(config.DataNode.Resources),
		},
		Plugins: config.Plugins,
	}
	if config.MasterNode.Valid {
		result.Opensearch.MasterNode = &protov1.OpenSearch_MasterNode{
			Resources: ResourcesToGRPC(config.MasterNode.Value.Resources),
		}
	}
	result.Access = accessToGRPC(clusterConfig.Access)

	return result
}

func accessToGRPC(a clusters.Access) *protov1.Access {
	if a.DataTransfer.Valid || a.Serverless.Valid || a.WebSQL.Valid {
		return &protov1.Access{
			DataTransfer: a.DataTransfer.Valid && a.DataTransfer.Bool,
			WebSql:       a.WebSQL.Valid && a.WebSQL.Bool,
			Serverless:   a.Serverless.Valid && a.Serverless.Bool,
		}
	}
	return nil

}

func modifyClusterArgsFromGRPC(request *protov1.UpdateClusterRequest, supportedVersions osmodels.SupportedVersions) (opensearch.ModifyClusterArgs, error) {
	if request.GetClusterId() == "" {
		return opensearch.ModifyClusterArgs{}, semerr.InvalidInput("missing required argument: cluster_id")
	}

	args := opensearch.ModifyClusterArgs{ClusterID: request.GetClusterId()}
	paths := grpc.NewFieldPaths(request.UpdateMask.GetPaths())
	paths.Remove("cluster_id") // just ignore cluster_id

	if paths.Empty() {
		return opensearch.ModifyClusterArgs{}, semerr.InvalidInput("no fields to change in update request")
	}
	if paths.Remove("name") {
		args.Name.Set(request.GetName())
	}
	if paths.Remove("description") {
		args.Description.Set(request.GetDescription())
	}
	if paths.Remove("labels") {
		args.Labels.Set(request.GetLabels())
	}
	if paths.Remove("security_group_ids") {
		args.SecurityGroupIDs.Set(request.GetSecurityGroupIds())
	}
	if paths.Remove("service_account_id") {
		args.ServiceAccountID.Set(request.GetServiceAccountId())
	}
	if paths.Remove("deletion_protection") {
		args.DeletionProtection.Set(request.GetDeletionProtection())
	}
	if paths.Remove("maintenance_window") {
		wnd, err := MaintenanceWindowFromGRPC(request.GetMaintenanceWindow())
		if err != nil {
			return opensearch.ModifyClusterArgs{}, err
		}
		args.MaintenanceWindow.Set(wnd)
	}

	configSpecPaths := paths.Subtree("config_spec.")
	if !configSpecPaths.Empty() && request.ConfigSpec != nil {
		configSpec, err := clusterConfigSpecUpdateFromGRPC(request.ConfigSpec, configSpecPaths, supportedVersions)
		if err != nil {
			return opensearch.ModifyClusterArgs{}, err
		}
		args.ConfigSpec = configSpec
	}

	err := paths.MustBeEmpty()
	if err != nil {
		return opensearch.ModifyClusterArgs{}, err
	}

	return args, nil
}

func clusterConfigSpecUpdateFromGRPC(spec *protov1.ConfigSpecUpdate, paths *grpc.FieldPaths, supportedVersions osmodels.SupportedVersions) (osmodels.ConfigSpecUpdate, error) {
	update := osmodels.ConfigSpecUpdate{}

	if paths.Remove("version") {
		version, err := supportedVersions.ParseVersion(spec.GetVersion())
		if err != nil {
			return osmodels.ConfigSpecUpdate{}, semerr.InvalidInput(err.Error())
		}
		update.Version.Set(version)
	}

	if paths.Remove("admin_password") {
		update.AdminPassword.Set(spec.GetAdminPassword())
	}

	esPaths := paths.Subtree("opensearch_spec.")
	if !esPaths.Empty() && spec.OpensearchSpec != nil {
		esConfigSpecUpdate, err := openSearchSpecUpdateFromGRPC(spec.OpensearchSpec, esPaths)
		if err != nil {
			return osmodels.ConfigSpecUpdate{}, err
		}
		update.OpenSearch = esConfigSpecUpdate
	}

	if paths.Remove("access") {
		update.Access = AccessFromGRPC(spec.GetAccess())
	}

	return update, nil
}

func openSearchSpecUpdateFromGRPC(spec *protov1.OpenSearchSpec, paths *grpc.FieldPaths) (osmodels.OpenSearchSpecUpdate, error) {
	update := osmodels.OpenSearchSpecUpdate{}

	dataNodePaths := paths.Subtree("data_node.")
	if !dataNodePaths.Empty() && spec.DataNode != nil {
		dataNodeUpdate, err := dataNodeUpdateFromGRPC(spec.DataNode, dataNodePaths)
		if err != nil {
			return osmodels.OpenSearchSpecUpdate{}, err
		}
		update.DataNode.Set(dataNodeUpdate)
	}

	masterNodePaths := paths.Subtree("master_node.")
	if !masterNodePaths.Empty() && spec.MasterNode != nil {
		masterNodeUpdate, err := masterNodeUpdateFromGRPC(spec.MasterNode, masterNodePaths)
		if err != nil {
			return osmodels.OpenSearchSpecUpdate{}, err
		}
		update.MasterNode.Set(masterNodeUpdate)
	}

	if paths.Remove("plugins") {
		update.Plugins.Set(spec.GetPlugins())
	}

	return update, nil
}

func dataNodeUpdateFromGRPC(spec *protov1.OpenSearchSpec_DataNode, paths *grpc.FieldPaths) (osmodels.DataNodeSpec, error) {
	update := osmodels.DataNodeSpec{}

	resourcesPaths := paths.Subtree("resources.")
	if !resourcesPaths.Empty() && spec.Resources != nil {
		resourcesUpdate, err := resourcesUpdateFromGRPC(spec.Resources, resourcesPaths)
		if err != nil {
			return osmodels.DataNodeSpec{}, err
		}
		update.Resources = resourcesUpdate
	}

	return update, nil
}

func masterNodeUpdateFromGRPC(spec *protov1.OpenSearchSpec_MasterNode, paths *grpc.FieldPaths) (osmodels.MasterNodeSpec, error) {
	update := osmodels.MasterNodeSpec{}

	resourcesPaths := paths.Subtree("resources.")
	if !resourcesPaths.Empty() && spec.Resources != nil {
		resourcesUpdate, err := resourcesUpdateFromGRPC(spec.Resources, resourcesPaths)
		if err != nil {
			return osmodels.MasterNodeSpec{}, err
		}
		update.Resources = resourcesUpdate
	}
	return update, nil
}

func resourcesUpdateFromGRPC(resources *protov1.Resources, paths *grpc.FieldPaths) (models.ClusterResourcesSpec, error) {
	update := models.ClusterResourcesSpec{}

	if paths.Remove("resource_preset_id") {
		update.ResourcePresetExtID.Set(resources.GetResourcePresetId())
	}
	if paths.Remove("disk_size") {
		update.DiskSize.Set(resources.GetDiskSize())
	}
	if paths.Remove("disk_type_id") {
		update.DiskTypeExtID.Set(resources.GetDiskTypeId())
	}

	return update, nil
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

func operationToGRPC(ctx context.Context, op operations.Operation, es opensearch.OpenSearch, saltEnvMapper grpc.SaltEnvMapper, l log.Logger) (*operation.Operation, error) {
	opGrpc, err := grpc.OperationToGRPC(ctx, op, l)
	if err != nil {
		return nil, err
	}
	if op.Status != operations.StatusDone {
		return opGrpc, nil
	}

	// TODO: remove type list, make more generic solution
	switch v := op.MetaData.(type) {
	// Must be at first position before interfaces
	case osmodels.MetadataDeleteCluster, osmodels.MetadataDeleteAuthProviders:
		return withEmptyResult(opGrpc)
	case osmodels.MetadataAddAuthProviders, osmodels.MetadataUpdateAuthProviders:
		ap, err := es.AuthProviders(ctx, op.ClusterID)
		if err != nil {
			return opGrpc, err
		}
		result, err := AuthProvidersToGRPC(ap.Providers())
		if err != nil {
			return opGrpc, err
		}
		return withResult(opGrpc, result)
	case osmodels.MetadataCreateCluster, osmodels.MetadataRestoreCluster, osmodels.MetadataModifyCluster,
		osmodels.MetadataStartCluster, osmodels.MetadataStopCluster, osmodels.MetadataBackupCluster:
		cluster, err := es.Cluster(ctx, op.ClusterID)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, ClusterToGRPC(cluster, saltEnvMapper))
	case osmodels.MetadataCreateExtension:
		extension, err := es.Extension(ctx, op.ClusterID, v.ExtensionID)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, ExtensionToGRPC(op.ClusterID, extension))
	case osmodels.MetadataUpdateExtension:
		extension, err := es.Extension(ctx, op.ClusterID, v.ExtensionID)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, ExtensionToGRPC(op.ClusterID, extension))
	}

	return opGrpc, nil
}

func ListLogsServiceTypeToGRPC(st logs.ServiceType) protov1.ListClusterLogsRequest_ServiceType {
	switch st {
	case logs.ServiceTypeOpenSearch:
		return protov1.ListClusterLogsRequest_OPENSEARCH
	case logs.ServiceTypeOpenSearchDashboards:
		return protov1.ListClusterLogsRequest_DASHBOARDS
	}

	return protov1.ListClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
}

func ListLogsServiceTypeFromGRPC(st protov1.ListClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	switch st {
	case protov1.ListClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED:
		return logs.ServiceTypeOpenSearch, nil
	case protov1.ListClusterLogsRequest_OPENSEARCH:
		return logs.ServiceTypeOpenSearch, nil
	case protov1.ListClusterLogsRequest_DASHBOARDS:
		return logs.ServiceTypeOpenSearchDashboards, nil
	}

	return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
}

func StreamLogsServiceTypeToGRPC(st logs.ServiceType) protov1.StreamClusterLogsRequest_ServiceType {
	switch st {
	case logs.ServiceTypeOpenSearch:
		return protov1.StreamClusterLogsRequest_OPENSEARCH
	case logs.ServiceTypeOpenSearchDashboards:
		return protov1.StreamClusterLogsRequest_DASHBOARDS
	}

	return protov1.StreamClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
}

func StreamLogsServiceTypeFromGRPC(st protov1.StreamClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	switch st {
	case protov1.StreamClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED:
		return logs.ServiceTypeOpenSearch, nil
	case protov1.StreamClusterLogsRequest_OPENSEARCH:
		return logs.ServiceTypeOpenSearch, nil
	case protov1.StreamClusterLogsRequest_DASHBOARDS:
		return logs.ServiceTypeOpenSearchDashboards, nil
	}

	return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
}

func AuthProvidersToGRPC(providers []*osmodels.AuthProvider) (*protov1.AuthProviders, error) {
	var res = make([]*protov1.AuthProvider, len(providers))
	for i := range providers {
		var err error
		if res[i], err = AuthProviderToGRPC(providers[i]); err != nil {
			return nil, err
		}
	}
	return &protov1.AuthProviders{Providers: res}, nil
}

func AuthProviderToGRPC(p *osmodels.AuthProvider) (*protov1.AuthProvider, error) {
	res := &protov1.AuthProvider{
		Type:        AuthProviderTypeToGRPC(p.Type),
		Name:        p.Name,
		Order:       int64(p.Order),
		Enabled:     p.Enabled,
		Hidden:      p.Hidden,
		Description: p.Description,
		Hint:        p.Hint,
		Icon:        p.Icon,
	}

	switch p.Type {
	case osmodels.AuthProviderTypeSaml:
		s := p.Settings.(*osmodels.SamlSettings)
		res.Settings = &protov1.AuthProvider_Saml{
			Saml: &protov1.SamlSettings{
				IdpEntityId:        s.IDPEntityID,
				IdpMetadataFile:    s.IDPMetadata,
				SpEntityId:         s.SPEntityID,
				DashboardsUrl:      s.DashboardsURL,
				AttributePrincipal: s.Principal,
				AttributeGroups:    s.Groups,
				AttributeName:      s.Name,
				AttributeEmail:     s.Mail,
				AttributeDn:        s.DN,
			}}
	}

	return res, nil
}

func AuthProvidersFromGRPC(providers []*protov1.AuthProvider) ([]*osmodels.AuthProvider, error) {
	var res = make([]*osmodels.AuthProvider, len(providers))
	for i := range providers {
		var err error
		if res[i], err = AuthProviderFromGRPC(providers[i]); err != nil {
			return nil, err
		}
	}
	return res, nil
}

func AuthProviderFromGRPC(p *protov1.AuthProvider) (*osmodels.AuthProvider, error) {
	if p == nil {
		return nil, semerr.InvalidInput("provider should be specified")
	}

	t, err := AuthProviderTypeFromGRPC(p.GetType())
	if err != nil {
		return nil, semerr.WrapWithInvalidInputf(err, "invalid type for provider %q", p.Name)
	}

	res := osmodels.NewAuthProvider(t, p.Name)
	res.Order = int(p.Order)
	res.Enabled = p.Enabled
	res.Hidden = p.Hidden
	res.Description = p.Description
	res.Hint = p.Hint
	res.Icon = p.Icon

	if p.Settings != nil {
		switch res.Type {
		case osmodels.AuthProviderTypeSaml:
			ss := p.GetSaml()
			if ss == nil {
				return nil, semerr.InvalidInput("mismatch provider type and settings")
			}
			rs := res.Settings.(*osmodels.SamlSettings)

			rs.IDPEntityID = ss.IdpEntityId
			rs.IDPMetadata = ss.IdpMetadataFile
			rs.SPEntityID = ss.SpEntityId
			rs.DashboardsURL = ss.DashboardsUrl
			rs.Principal = ss.AttributePrincipal
			rs.Groups = ss.AttributeGroups
			rs.Name = ss.AttributeName
			rs.Mail = ss.AttributeEmail
			rs.DN = ss.AttributeDn

		case osmodels.AuthProviderTypeNative,
			osmodels.AuthProviderTypeOpenID,
			osmodels.AuthProviderTypeAnonymous:
			return nil, semerr.InvalidInput("mismatch provider type and settings")
		}
	}

	return res, nil
}

var (
	authProviderTypeToGRPC = map[osmodels.AuthProviderType]protov1.AuthProvider_Type{
		osmodels.AuthProviderTypeNative: protov1.AuthProvider_NATIVE,
		osmodels.AuthProviderTypeSaml:   protov1.AuthProvider_SAML,
	}
	authProviderTypeFromGRPC = reflectutil.ReverseMap(authProviderTypeToGRPC).(map[protov1.AuthProvider_Type]osmodels.AuthProviderType)
)

func AuthProviderTypeFromGRPC(t protov1.AuthProvider_Type) (osmodels.AuthProviderType, error) {
	if r, ok := authProviderTypeFromGRPC[t]; ok {
		return r, nil
	}

	return "", semerr.Internal("unknown auth provider type")
}

func AuthProviderTypeToGRPC(t osmodels.AuthProviderType) protov1.AuthProvider_Type {
	if r, ok := authProviderTypeToGRPC[t]; ok {
		return r
	}
	return protov1.AuthProvider_TYPE_UNSPECIFIED
}

func BackupToGRPC(backup backups.Backup) *protov1.Backup {
	meta := backup.Metadata.(osmodels.SnapshotInfo)
	return &protov1.Backup{
		Id:                backup.GlobalBackupID(),
		FolderId:          backup.FolderID,
		SourceClusterId:   backup.SourceClusterID,
		CreatedAt:         grpc.TimeToGRPC(backup.CreatedAt),
		StartedAt:         grpc.OptionalTimeToGRPC(backup.StartedAt),
		OpensearchVersion: meta.OpenSearchVersion,
		SizeBytes:         int64(meta.Size),
		Indices:           meta.Indices,
		IndicesTotal:      int64(meta.IndicesTotal),
	}
}

func BackupsToGRPC(backups []backups.Backup) []*protov1.Backup {
	v := make([]*protov1.Backup, 0, len(backups))
	for _, b := range backups {
		v = append(v, BackupToGRPC(b))
	}
	return v
}

func RescheduleTypeFromGRPC(rescheduleType protov1.RescheduleMaintenanceRequest_RescheduleType) (clusters.RescheduleType, error) {
	fromGRPCMap := map[protov1.RescheduleMaintenanceRequest_RescheduleType]clusters.RescheduleType{
		protov1.RescheduleMaintenanceRequest_RESCHEDULE_TYPE_UNSPECIFIED: clusters.RescheduleTypeUnspecified,
		protov1.RescheduleMaintenanceRequest_IMMEDIATE:                   clusters.RescheduleTypeImmediate,
		protov1.RescheduleMaintenanceRequest_NEXT_AVAILABLE_WINDOW:       clusters.RescheduleTypeNextAvailableWindow,
		protov1.RescheduleMaintenanceRequest_SPECIFIC_TIME:               clusters.RescheduleTypeSpecificTime,
	}

	v, ok := fromGRPCMap[rescheduleType]
	if !ok {
		return clusters.RescheduleTypeUnspecified, semerr.InvalidInput("unknown reschedule type")
	}

	return v, nil
}

func ExtensionToGRPC(cid string, extension osmodels.Extension) *protov1.Extension {
	return &protov1.Extension{
		ClusterId: cid,
		Id:        extension.ID,
		Name:      extension.Name,
		Version:   int64(extension.Version),
		Active:    extension.Active,
	}
}

func ExtensionsToGRPC(cid string, extensions []osmodels.Extension) []*protov1.Extension {
	v := make([]*protov1.Extension, 0, len(extensions))
	for _, e := range extensions {
		v = append(v, ExtensionToGRPC(cid, e))
	}
	return v
}

func ExtensionSpecFromGRPC(spec *protov1.ExtensionSpec) osmodels.ExtensionSpec {
	return osmodels.ExtensionSpec{
		Name:     spec.Name,
		URI:      spec.Uri,
		Disabled: spec.Disabled,
	}
}

func ExtensionSpecsFromGRPC(specs []*protov1.ExtensionSpec) []osmodels.ExtensionSpec {
	v := make([]osmodels.ExtensionSpec, 0, len(specs))
	for _, e := range specs {
		v = append(v, ExtensionSpecFromGRPC(e))
	}
	return v
}

func AccessFromGRPC(a *protov1.Access) (access osmodels.OptionalAccess) {
	if a.GetDataTransfer() {
		access.Value.DataTransfer = optional.NewBool(a.GetDataTransfer())
		access.Valid = true
	}
	return
}
