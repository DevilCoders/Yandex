package elasticsearch

import (
	"context"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes"

	esv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1"
	esv1config "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1/config"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/esmodels"
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
	mapHostRoleToGRPC = map[hosts.Role]esv1.Host_Type{
		hosts.RoleElasticSearchDataNode:   esv1.Host_DATA_NODE,
		hosts.RoleElasticSearchMasterNode: esv1.Host_MASTER_NODE,
	}
	mapHostRoleFromGRPC = reflectutil.ReverseMap(mapHostRoleToGRPC).(map[esv1.Host_Type]hosts.Role)
)

func HostRoleToGRPC(role hosts.Role) esv1.Host_Type {
	v, ok := mapHostRoleToGRPC[role]
	if !ok {
		return esv1.Host_TYPE_UNSPECIFIED
	}

	return v
}

func HostRoleFromGRPC(ht esv1.Host_Type) (hosts.Role, error) {
	v, ok := mapHostRoleFromGRPC[ht]
	if !ok {
		return hosts.RoleUnknown, semerr.InvalidInput("unknown host role")
	}

	return v, nil
}

func HostRolesToGRPC(hostRoles []hosts.Role) esv1.Host_Type {
	if len(hostRoles) < 1 {
		return esv1.Host_TYPE_UNSPECIFIED
	}

	return HostRoleToGRPC(hostRoles[0])
}

var (
	statusToGRCP = map[clusters.Status]esv1.Cluster_Status{
		clusters.StatusCreating:             esv1.Cluster_CREATING,
		clusters.StatusCreateError:          esv1.Cluster_ERROR,
		clusters.StatusRunning:              esv1.Cluster_RUNNING,
		clusters.StatusModifying:            esv1.Cluster_UPDATING,
		clusters.StatusModifyError:          esv1.Cluster_ERROR,
		clusters.StatusStopping:             esv1.Cluster_STOPPING,
		clusters.StatusStopped:              esv1.Cluster_STOPPED,
		clusters.StatusStopError:            esv1.Cluster_ERROR,
		clusters.StatusStarting:             esv1.Cluster_STARTING,
		clusters.StatusStartError:           esv1.Cluster_ERROR,
		clusters.StatusMaintainingOffline:   esv1.Cluster_UPDATING,
		clusters.StatusMaintainOfflineError: esv1.Cluster_ERROR,
	}
)

func StatusToGRPC(status clusters.Status) esv1.Cluster_Status {
	v, ok := statusToGRCP[status]
	if !ok {
		return esv1.Cluster_STATUS_UNKNOWN
	}
	return v
}

var (
	clusterHealthToGRPC = map[clusters.Health]esv1.Cluster_Health{
		clusters.HealthAlive:    esv1.Cluster_ALIVE,
		clusters.HealthDegraded: esv1.Cluster_DEGRADED,
		clusters.HealthDead:     esv1.Cluster_DEAD,
		clusters.HealthUnknown:  esv1.Cluster_HEALTH_UNKNOWN,
	}
)

func ClusterHealthToGRPC(env clusters.Health) esv1.Cluster_Health {
	v, ok := clusterHealthToGRPC[env]
	if !ok {
		return esv1.Cluster_HEALTH_UNKNOWN
	}
	return v
}

var (
	hostHealthToGRPC = map[hosts.Status]esv1.Host_Health{
		hosts.StatusAlive:    esv1.Host_ALIVE,
		hosts.StatusDegraded: esv1.Host_DEGRADED,
		hosts.StatusDead:     esv1.Host_DEAD,
		hosts.StatusUnknown:  esv1.Host_UNKNOWN,
	}
)

func HostHealthToGRPC(hh hosts.Health) esv1.Host_Health {
	v, ok := hostHealthToGRPC[hh.Status]
	if !ok {
		return esv1.Host_UNKNOWN
	}
	return v
}

func ClusterToGRPC(cluster esmodels.Cluster, saltEnvMapper grpc.SaltEnvMapper) *esv1.Cluster {
	v := &esv1.Cluster{
		Id:                 cluster.ClusterID,
		FolderId:           cluster.FolderExtID,
		Name:               cluster.Name,
		Description:        cluster.Description,
		Labels:             cluster.Labels,
		NetworkId:          cluster.NetworkID,
		SecurityGroupIds:   cluster.SecurityGroupIDs,
		CreatedAt:          grpc.TimeToGRPC(cluster.CreatedAt),
		Environment:        esv1.Cluster_Environment(saltEnvMapper.ToGRPC(cluster.Environment)),
		Status:             StatusToGRPC(cluster.Status),
		Health:             ClusterHealthToGRPC(cluster.Health),
		Config:             ConfigToGRPC(cluster.Config),
		ServiceAccountId:   cluster.ServiceAccountID,
		DeletionProtection: cluster.DeletionProtection,
		MaintenanceWindow:  MaintenanceWindowToGRPC(cluster.MaintenanceInfo.Window),
		PlannedOperation:   MaintenanceOperationToGRPC(cluster.MaintenanceInfo.Operation),
	}
	v.Monitoring = make([]*esv1.Monitoring, 0, len(cluster.Monitoring.Charts))
	for _, chart := range cluster.Monitoring.Charts {
		mon := &esv1.Monitoring{
			Name:        chart.Name,
			Description: chart.Description,
			Link:        chart.Link,
		}
		v.Monitoring = append(v.Monitoring, mon)
	}
	return v
}

func ClustersToGRPC(clusters []esmodels.Cluster, saltEnvMapper grpc.SaltEnvMapper) []*esv1.Cluster {
	var v []*esv1.Cluster
	for _, cluster := range clusters {
		v = append(v, ClusterToGRPC(cluster, saltEnvMapper))
	}
	return v
}

func HostsFromGRPC(hostSpec []*esv1.HostSpec) ([]esmodels.Host, error) {
	var hosts []esmodels.Host
	for _, host := range hostSpec {
		hostRole, err := HostRoleFromGRPC(host.Type)
		if err != nil {
			return []esmodels.Host{}, err
		}
		var subnetID optional.String
		if host.SubnetId == "" {
			subnetID = optional.String{}
		} else {
			subnetID = optional.NewString(host.SubnetId)
		}
		hosts = append(hosts, esmodels.Host{
			ZoneID:         host.ZoneId,
			Role:           hostRole,
			AssignPublicIP: host.AssignPublicIp,
			ShardName:      host.ShardName,
			SubnetID:       subnetID,
		})
	}
	return hosts, nil
}

func cpuMetricToGRPC(cpu *system.CPUMetric) *esv1.Host_CPUMetric {
	if cpu == nil {
		return nil
	}

	return &esv1.Host_CPUMetric{
		Timestamp: cpu.Timestamp,
		Used:      cpu.Used,
	}
}

func memoryMetricToGRPC(memory *system.MemoryMetric) *esv1.Host_MemoryMetric {
	if memory == nil {
		return nil
	}

	return &esv1.Host_MemoryMetric{
		Timestamp: memory.Timestamp,
		Used:      memory.Used,
		Total:     memory.Total,
	}
}

func diskMetricToGRPC(disk *system.DiskMetric) *esv1.Host_DiskMetric {
	if disk == nil {
		return nil
	}

	return &esv1.Host_DiskMetric{
		Timestamp: disk.Timestamp,
		Used:      disk.Used,
		Total:     disk.Total,
	}
}

func HostSystemMetricsToGRPC(sm *system.Metrics) *esv1.Host_SystemMetrics {
	if sm == nil {
		return nil
	}

	return &esv1.Host_SystemMetrics{
		Cpu:    cpuMetricToGRPC(sm.CPU),
		Memory: memoryMetricToGRPC(sm.Memory),
		Disk:   diskMetricToGRPC(sm.Disk),
	}
}

func HostToGRPC(host hosts.HostExtended) *esv1.Host {
	return &esv1.Host{
		Name:      host.FQDN,
		ClusterId: host.ClusterID,
		ZoneId:    host.ZoneID,
		Type:      HostRolesToGRPC(host.Roles),
		Resources: &esv1.Resources{
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

func HostsToGRPC(hosts []hosts.HostExtended) []*esv1.Host {
	var v []*esv1.Host
	for _, host := range hosts {
		v = append(v, HostToGRPC(host))
	}
	return v
}

func ConfigFromGRPC(configSpec *esv1.ConfigSpec, supportedVersions esmodels.SupportedVersions) (esmodels.ClusterConfigSpec, error) {
	edition := esmodels.OptionalEdition{}
	if configSpec.GetEdition() != "" {
		v, err := esmodels.ParseEdition(configSpec.GetEdition())
		if err != nil {
			return esmodels.ClusterConfigSpec{}, semerr.InvalidInput(err.Error())
		}
		edition.Set(v)
	}

	version := esmodels.OptionalVersion{}
	if configSpec.GetVersion() != "" {
		v, err := supportedVersions.ParseVersion(configSpec.GetVersion())
		if err != nil {
			return esmodels.ClusterConfigSpec{}, semerr.InvalidInput(err.Error())
		}
		version.Set(v)
	}

	adminPassword := esmodels.OptionalSecretString{}
	if configSpec.AdminPassword != "" {
		adminPassword.Set(configSpec.AdminPassword)
	}

	access := AccessFromGRPC(configSpec.Access)

	result := esmodels.ClusterConfigSpec{
		Version:       version,
		Edition:       edition,
		AdminPassword: adminPassword,
		Access:        access,
	}
	// TODO: handle versioned config
	c := configSpec.GetElasticsearchSpec().GetDataNode().GetElasticsearchConfig_7()
	dataCfg := esmodels.DataNodeConfig{
		MaxClauseCount:         grpc.OptionalInt64FromGRPC(c.GetMaxClauseCount()),
		ReindexRemoteWhitelist: grpc.OptionalStringFromGRPC(c.GetReindexRemoteWhitelist()),
		ReindexSSLCAPath:       grpc.OptionalStringFromGRPC(c.GetReindexSslCaPath()),
	}
	if c.GetFielddataCacheSize() != "" {
		dataCfg.FielddataCacheSize.Set(c.GetFielddataCacheSize())
	}

	dataResources := grpc.ResourcesFromGRPC(configSpec.GetElasticsearchSpec().GetDataNode().GetResources(), grpc.AllPaths())

	masterNode := esmodels.OptionalMasterNodeSpec{}
	masterResourcesGRPC := configSpec.GetElasticsearchSpec().GetMasterNode().GetResources()
	if masterResourcesGRPC != nil {
		masterNode.Set(esmodels.MasterNodeSpec{Resources: grpc.ResourcesFromGRPC(masterResourcesGRPC, grpc.AllPaths())})
		masterNode.Valid = true
	}

	result.Config.Set(
		esmodels.ConfigSpec{
			DataNode: esmodels.DataNodeSpec{
				Config:    dataCfg,
				Resources: dataResources,
			},
			MasterNode: masterNode,
			Plugins:    configSpec.GetElasticsearchSpec().GetPlugins(),
		})

	return result, nil
}

var maintenanceWindowDayStringToGRPCMap = map[string]esv1.WeeklyMaintenanceWindow_WeekDay{
	"MON": esv1.WeeklyMaintenanceWindow_MON,
	"TUE": esv1.WeeklyMaintenanceWindow_TUE,
	"WED": esv1.WeeklyMaintenanceWindow_WED,
	"THU": esv1.WeeklyMaintenanceWindow_THU,
	"FRI": esv1.WeeklyMaintenanceWindow_FRI,
	"SAT": esv1.WeeklyMaintenanceWindow_SAT,
	"SUN": esv1.WeeklyMaintenanceWindow_SUN,
}

func MaintenanceWindowToGRPC(window clusters.MaintenanceWindow) *esv1.MaintenanceWindow {
	if window.Anytime() {
		return &esv1.MaintenanceWindow{
			Policy: &esv1.MaintenanceWindow_Anytime{
				Anytime: &esv1.AnytimeMaintenanceWindow{},
			},
		}
	}

	return &esv1.MaintenanceWindow{
		Policy: &esv1.MaintenanceWindow_WeeklyMaintenanceWindow{
			WeeklyMaintenanceWindow: &esv1.WeeklyMaintenanceWindow{
				Day:  mapStringMaintenanceWindowDayToGRPC(window.Day),
				Hour: int64(window.Hour),
			},
		},
	}
}

func MaintenanceWindowFromGRPC(window *esv1.MaintenanceWindow) (clusters.MaintenanceWindow, error) {
	if window == nil {
		return clusters.NewAnytimeMaintenanceWindow(), nil
	}

	switch p := window.GetPolicy().(type) {
	case *esv1.MaintenanceWindow_Anytime:
		return clusters.NewAnytimeMaintenanceWindow(), nil
	case *esv1.MaintenanceWindow_WeeklyMaintenanceWindow:
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

func mapGRPCMaintenanceWindowDayToString(day esv1.WeeklyMaintenanceWindow_WeekDay) (string, bool) {
	rev := reflectutil.ReverseMap(maintenanceWindowDayStringToGRPCMap).(map[esv1.WeeklyMaintenanceWindow_WeekDay]string)
	v, ok := rev[day]
	return v, ok
}

func mapStringMaintenanceWindowDayToGRPC(dayStr string) esv1.WeeklyMaintenanceWindow_WeekDay {
	day, ok := maintenanceWindowDayStringToGRPCMap[dayStr]
	if !ok {
		day = esv1.WeeklyMaintenanceWindow_WEEK_DAY_UNSPECIFIED
	}
	return day
}

func MaintenanceOperationToGRPC(operation clusters.MaintenanceOperation) *esv1.MaintenanceOperation {
	if !operation.Valid() {
		return nil
	}

	return &esv1.MaintenanceOperation{
		Info:                      operation.Info,
		DelayedUntil:              grpc.TimeToGRPC(operation.DelayedUntil),
		LatestMaintenanceTime:     grpc.TimeToGRPC(operation.LatestMaintenanceTime),
		NextMaintenanceWindowTime: grpc.TimePtrToGRPC(operation.NearestMaintenanceWindow),
	}
}

func ResourcesToGRPC(res models.ClusterResources) *esv1.Resources {
	return &esv1.Resources{
		ResourcePresetId: res.ResourcePresetExtID,
		DiskTypeId:       res.DiskTypeExtID,
		DiskSize:         res.DiskSize,
	}
}

// TODO: handle version config
func ConfigToGRPC(clusterConfig esmodels.ClusterConfig) *esv1.ClusterConfig {
	result := &esv1.ClusterConfig{
		Version: clusterConfig.Version.ID,
		Edition: clusterConfig.Edition.String(),
	}

	config := clusterConfig.Config
	result.Elasticsearch = &esv1.Elasticsearch{
		DataNode: &esv1.Elasticsearch_DataNode{
			Config:    dataNodeConfigSet7ToGRPC(config.DataNode.ConfigSet),
			Resources: ResourcesToGRPC(config.DataNode.Resources),
		},
		Plugins: config.Plugins,
	}
	if config.MasterNode.Valid {
		result.Elasticsearch.MasterNode = &esv1.Elasticsearch_MasterNode{
			Resources: ResourcesToGRPC(config.MasterNode.Value.Resources),
		}
	}
	result.Access = accessToGRPC(clusterConfig.Access)

	return result
}

func accessToGRPC(a clusters.Access) *esv1.Access {
	if a.DataTransfer.Valid || a.Serverless.Valid || a.WebSQL.Valid {
		return &esv1.Access{
			DataTransfer: a.DataTransfer.Valid && a.DataTransfer.Bool,
			WebSql:       a.WebSQL.Valid && a.WebSQL.Bool,
			Serverless:   a.Serverless.Valid && a.Serverless.Bool,
		}
	}
	return nil

}

func dataNodeConfigSet7ToGRPC(configSet esmodels.DataNodeConfigSet) *esv1.Elasticsearch_DataNode_ElasticsearchConfigSet_7 {
	return &esv1.Elasticsearch_DataNode_ElasticsearchConfigSet_7{
		ElasticsearchConfigSet_7: &esv1config.ElasticsearchConfigSet7{
			EffectiveConfig: dataNodeConfig7ToGRPC(configSet.EffectiveConfig),
			DefaultConfig:   dataNodeConfig7ToGRPC(configSet.DefaultConfig),
			UserConfig:      dataNodeConfig7ToGRPC(configSet.UserConfig),
		},
	}
}

func dataNodeConfig7ToGRPC(config esmodels.DataNodeConfig) *esv1config.ElasticsearchConfig7 {
	var result = &esv1config.ElasticsearchConfig7{}

	if config.FielddataCacheSize.Valid {
		result.FielddataCacheSize = config.FielddataCacheSize.String
	}
	if config.MaxClauseCount.Valid {
		result.MaxClauseCount = grpc.OptionalInt64ToGRPC(config.MaxClauseCount)
	}
	if config.ReindexRemoteWhitelist.Valid {
		result.ReindexRemoteWhitelist = config.ReindexRemoteWhitelist.String
	}
	if config.ReindexSSLCAPath.Valid {
		result.ReindexSslCaPath = config.ReindexSSLCAPath.String
	}
	return result
}

func UserSpecFromGRPC(spec *esv1.UserSpec) esmodels.UserSpec {
	return esmodels.UserSpec{Name: spec.Name, Password: secret.NewString(spec.Password)}
}

func UserSpecsFromGRPC(specs []*esv1.UserSpec) []esmodels.UserSpec {
	var v []esmodels.UserSpec
	for _, spec := range specs {
		if spec != nil {
			v = append(v, UserSpecFromGRPC(spec))
		}
	}
	return v
}

func UserToGRPC(user esmodels.User) *esv1.User {
	return &esv1.User{Name: user.Name, ClusterId: user.ClusterID}
}

func UsersToGRPC(users []esmodels.User) []*esv1.User {
	var v []*esv1.User
	for _, user := range users {
		v = append(v, UserToGRPC(user))
	}
	return v
}

func updateUserArgsFromGRPC(request *esv1.UpdateUserRequest) (elasticsearch.UpdateUserArgs, error) {
	if request.GetClusterId() == "" {
		return elasticsearch.UpdateUserArgs{}, semerr.InvalidInput("missing required argument: cluster_id")
	}

	if request.GetUserName() == "" {
		return elasticsearch.UpdateUserArgs{}, semerr.InvalidInput("missing required argument: name")
	}

	args := elasticsearch.UpdateUserArgs{
		ClusterID: request.GetClusterId(),
		Name:      request.GetUserName(),
	}
	paths := grpc.NewFieldPaths(request.UpdateMask.GetPaths())
	paths.Remove("cluster_id") // just ignore cluster_id
	paths.Remove("name")       // just ignore name

	if paths.Empty() {
		return elasticsearch.UpdateUserArgs{}, semerr.InvalidInput("no fields to change in update request")
	}

	if paths.Remove("password") {
		args.Password = secret.NewString(request.GetPassword())
	}

	err := paths.MustBeEmpty()
	if err != nil {
		return elasticsearch.UpdateUserArgs{}, err
	}

	return args, nil
}

func modifyClusterArgsFromGRPC(request *esv1.UpdateClusterRequest, supportedVersions esmodels.SupportedVersions) (elasticsearch.ModifyClusterArgs, error) {
	if request.GetClusterId() == "" {
		return elasticsearch.ModifyClusterArgs{}, semerr.InvalidInput("missing required argument: cluster_id")
	}

	args := elasticsearch.ModifyClusterArgs{ClusterID: request.GetClusterId()}
	paths := grpc.NewFieldPaths(request.UpdateMask.GetPaths())
	paths.Remove("cluster_id") // just ignore cluster_id

	if paths.Empty() {
		return elasticsearch.ModifyClusterArgs{}, semerr.InvalidInput("no fields to change in update request")
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
			return elasticsearch.ModifyClusterArgs{}, err
		}
		args.MaintenanceWindow.Set(wnd)
	}

	configSpecPaths := paths.Subtree("config_spec.")
	if !configSpecPaths.Empty() && request.ConfigSpec != nil {
		configSpec, err := clusterConfigSpecUpdateFromGRPC(request.ConfigSpec, configSpecPaths, supportedVersions)
		if err != nil {
			return elasticsearch.ModifyClusterArgs{}, err
		}
		args.ConfigSpec = configSpec
	}

	err := paths.MustBeEmpty()
	if err != nil {
		return elasticsearch.ModifyClusterArgs{}, err
	}

	return args, nil
}

func clusterConfigSpecUpdateFromGRPC(spec *esv1.ConfigSpecUpdate, paths *grpc.FieldPaths, supportedVersions esmodels.SupportedVersions) (esmodels.ConfigSpecUpdate, error) {
	update := esmodels.ConfigSpecUpdate{}

	if paths.Remove("version") {
		version, err := supportedVersions.ParseVersion(spec.GetVersion())
		if err != nil {
			return esmodels.ConfigSpecUpdate{}, semerr.InvalidInput(err.Error())
		}
		update.Version.Set(version)
	}

	if paths.Remove("edition") {
		edition, err := esmodels.ParseEdition(spec.GetEdition())
		if err != nil {
			return esmodels.ConfigSpecUpdate{}, semerr.InvalidInput(err.Error())
		}
		update.Edition.Set(edition)
	}

	if paths.Remove("admin_password") {
		update.AdminPassword.Set(spec.GetAdminPassword())
	}

	esPaths := paths.Subtree("elasticsearch_spec.")
	if !esPaths.Empty() && spec.ElasticsearchSpec != nil {
		esConfigSpecUpdate, err := elasticsearchSpecUpdateFromGRPC(spec.ElasticsearchSpec, esPaths)
		if err != nil {
			return esmodels.ConfigSpecUpdate{}, err
		}
		update.Elasticsearch = esConfigSpecUpdate
	}

	if paths.Remove("access") {
		update.Access = AccessFromGRPC(spec.GetAccess())
	}

	return update, nil
}

func elasticsearchSpecUpdateFromGRPC(spec *esv1.ElasticsearchSpec, paths *grpc.FieldPaths) (esmodels.ElasticsearchSpecUpdate, error) {
	update := esmodels.ElasticsearchSpecUpdate{}

	dataNodePaths := paths.Subtree("data_node.")
	if !dataNodePaths.Empty() && spec.DataNode != nil {
		dataNodeUpdate, err := dataNodeUpdateFromGRPC(spec.DataNode, dataNodePaths)
		if err != nil {
			return esmodels.ElasticsearchSpecUpdate{}, err
		}
		update.DataNode.Set(dataNodeUpdate)
	}

	masterNodePaths := paths.Subtree("master_node.")
	if !masterNodePaths.Empty() && spec.MasterNode != nil {
		masterNodeUpdate, err := masterNodeUpdateFromGRPC(spec.MasterNode, masterNodePaths)
		if err != nil {
			return esmodels.ElasticsearchSpecUpdate{}, err
		}
		update.MasterNode.Set(masterNodeUpdate)
	}

	if paths.Remove("plugins") {
		update.Plugins.Set(spec.GetPlugins())
	}

	return update, nil
}

func dataNodeUpdateFromGRPC(spec *esv1.ElasticsearchSpec_DataNode, paths *grpc.FieldPaths) (esmodels.DataNodeSpec, error) {
	update := esmodels.DataNodeSpec{}

	resourcesPaths := paths.Subtree("resources.")
	if !resourcesPaths.Empty() && spec.Resources != nil {
		resourcesUpdate, err := resourcesUpdateFromGRPC(spec.Resources, resourcesPaths)
		if err != nil {
			return esmodels.DataNodeSpec{}, err
		}
		update.Resources = resourcesUpdate
	}

	if spec.Config != nil {
		switch config := spec.Config.(type) {
		case *esv1.ElasticsearchSpec_DataNode_ElasticsearchConfig_7:
			esConfigPaths := paths.Subtree("elasticsearch_config_7.")
			if !esConfigPaths.Empty() {
				esConfigUpdate, err := esConfig7UpdateFromGRPC(config, esConfigPaths)
				if err != nil {
					return esmodels.DataNodeSpec{}, err
				}
				update.Config = esConfigUpdate
			}
		default:
			return esmodels.DataNodeSpec{},
				xerrors.Errorf("unsupported data config type: %+v", spec.Config)
		}
	}
	return update, nil
}

func esConfig7UpdateFromGRPC(config *esv1.ElasticsearchSpec_DataNode_ElasticsearchConfig_7, paths *grpc.FieldPaths) (esmodels.DataNodeConfig, error) {
	update := esmodels.DataNodeConfig{}

	if paths.Remove("fielddata_cache_size") {
		update.FielddataCacheSize.Set(config.ElasticsearchConfig_7.GetFielddataCacheSize())
	}
	if paths.Remove("max_clause_count") {
		update.MaxClauseCount.Set(config.ElasticsearchConfig_7.GetMaxClauseCount().GetValue())
	}
	if paths.Remove("reindex_remote_whitelist") {
		update.ReindexRemoteWhitelist.Set(config.ElasticsearchConfig_7.GetReindexRemoteWhitelist())
	}
	if paths.Remove("reindex_ssl_ca_path") {
		update.ReindexSSLCAPath.Set(config.ElasticsearchConfig_7.GetReindexSslCaPath())
	}
	return update, nil
}

func masterNodeUpdateFromGRPC(spec *esv1.ElasticsearchSpec_MasterNode, paths *grpc.FieldPaths) (esmodels.MasterNodeSpec, error) {
	update := esmodels.MasterNodeSpec{}

	resourcesPaths := paths.Subtree("resources.")
	if !resourcesPaths.Empty() && spec.Resources != nil {
		resourcesUpdate, err := resourcesUpdateFromGRPC(spec.Resources, resourcesPaths)
		if err != nil {
			return esmodels.MasterNodeSpec{}, err
		}
		update.Resources = resourcesUpdate
	}
	return update, nil
}

func resourcesUpdateFromGRPC(resources *esv1.Resources, paths *grpc.FieldPaths) (models.ClusterResourcesSpec, error) {
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

func operationToGRPC(ctx context.Context, op operations.Operation, es elasticsearch.ElasticSearch, saltEnvMapper grpc.SaltEnvMapper, l log.Logger) (*operation.Operation, error) {
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
	case esmodels.MetadataDeleteCluster, esmodels.MetadataDeleteUser, esmodels.MetadataDeleteAuthProviders:
		return withEmptyResult(opGrpc)
	case esmodels.MetadataAddAuthProviders, esmodels.MetadataUpdateAuthProviders:
		ap, err := es.AuthProviders(ctx, op.ClusterID)
		if err != nil {
			return opGrpc, err
		}
		result, err := AuthProvidersToGRPC(ap.Providers())
		if err != nil {
			return opGrpc, err
		}
		return withResult(opGrpc, result)
	case esmodels.MetadataCreateCluster, esmodels.MetadataRestoreCluster, esmodels.MetadataModifyCluster,
		esmodels.MetadataStartCluster, esmodels.MetadataStopCluster, esmodels.MetadataBackupCluster:
		cluster, err := es.Cluster(ctx, op.ClusterID)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, ClusterToGRPC(cluster, saltEnvMapper))
	case esmodels.MetadataUser:
		userName := v.GetUsername()
		user, err := es.User(ctx, op.ClusterID, userName)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, UserToGRPC(user))
	case esmodels.MetadataCreateExtension:
		extension, err := es.Extension(ctx, op.ClusterID, v.ExtensionID)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, ExtensionToGRPC(op.ClusterID, extension))
	case esmodels.MetadataUpdateExtension:
		extension, err := es.Extension(ctx, op.ClusterID, v.ExtensionID)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, ExtensionToGRPC(op.ClusterID, extension))
	}

	return opGrpc, nil
}

func ListLogsServiceTypeToGRPC(st logs.ServiceType) esv1.ListClusterLogsRequest_ServiceType {
	switch st {
	case logs.ServiceTypeElasticSearch:
		return esv1.ListClusterLogsRequest_ELASTICSEARCH
	case logs.ServiceTypeKibana:
		return esv1.ListClusterLogsRequest_KIBANA
	}

	return esv1.ListClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
}

func ListLogsServiceTypeFromGRPC(st esv1.ListClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	switch st {
	case esv1.ListClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED:
		return logs.ServiceTypeElasticSearch, nil
	case esv1.ListClusterLogsRequest_ELASTICSEARCH:
		return logs.ServiceTypeElasticSearch, nil
	case esv1.ListClusterLogsRequest_KIBANA:
		return logs.ServiceTypeKibana, nil
	}

	return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
}

func StreamLogsServiceTypeToGRPC(st logs.ServiceType) esv1.StreamClusterLogsRequest_ServiceType {
	switch st {
	case logs.ServiceTypeElasticSearch:
		return esv1.StreamClusterLogsRequest_ELASTICSEARCH
	case logs.ServiceTypeKibana:
		return esv1.StreamClusterLogsRequest_KIBANA
	}

	return esv1.StreamClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
}

func StreamLogsServiceTypeFromGRPC(st esv1.StreamClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	switch st {
	case esv1.StreamClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED:
		return logs.ServiceTypeElasticSearch, nil
	case esv1.StreamClusterLogsRequest_ELASTICSEARCH:
		return logs.ServiceTypeElasticSearch, nil
	case esv1.StreamClusterLogsRequest_KIBANA:
		return logs.ServiceTypeKibana, nil
	}

	return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
}

func AuthProvidersToGRPC(providers []*esmodels.AuthProvider) (*esv1.AuthProviders, error) {
	var res = make([]*esv1.AuthProvider, len(providers))
	for i := range providers {
		var err error
		if res[i], err = AuthProviderToGRPC(providers[i]); err != nil {
			return nil, err
		}
	}
	return &esv1.AuthProviders{Providers: res}, nil
}

func AuthProviderToGRPC(p *esmodels.AuthProvider) (*esv1.AuthProvider, error) {
	res := &esv1.AuthProvider{
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
	case esmodels.AuthProviderTypeSaml:
		s := p.Settings.(*esmodels.SamlSettings)
		res.Settings = &esv1.AuthProvider_Saml{
			Saml: &esv1.SamlSettings{
				IdpEntityId:        s.IDPEntityID,
				IdpMetadataFile:    s.IDPMetadata,
				SpEntityId:         s.SPEntityID,
				KibanaUrl:          s.KibanaURL,
				AttributePrincipal: s.Principal,
				AttributeGroups:    s.Groups,
				AttributeName:      s.Name,
				AttributeEmail:     s.Mail,
				AttributeDn:        s.DN,
			}}
	}

	return res, nil
}

func AuthProvidersFromGRPC(providers []*esv1.AuthProvider) ([]*esmodels.AuthProvider, error) {
	var res = make([]*esmodels.AuthProvider, len(providers))
	for i := range providers {
		var err error
		if res[i], err = AuthProviderFromGRPC(providers[i]); err != nil {
			return nil, err
		}
	}
	return res, nil
}

func AuthProviderFromGRPC(p *esv1.AuthProvider) (*esmodels.AuthProvider, error) {
	if p == nil {
		return nil, semerr.InvalidInput("provider should be specified")
	}

	t, err := AuthProviderTypeFromGRPC(p.GetType())
	if err != nil {
		return nil, semerr.WrapWithInvalidInputf(err, "invalid type for provider %q", p.Name)
	}

	res := esmodels.NewAuthProvider(t, p.Name)
	res.Order = int(p.Order)
	res.Enabled = p.Enabled
	res.Hidden = p.Hidden
	res.Description = p.Description
	res.Hint = p.Hint
	res.Icon = p.Icon

	if p.Settings != nil {
		switch res.Type {
		case esmodels.AuthProviderTypeSaml:
			ss := p.GetSaml()
			if ss == nil {
				return nil, semerr.InvalidInput("mismatch provider type and settings")
			}
			rs := res.Settings.(*esmodels.SamlSettings)

			rs.IDPEntityID = ss.IdpEntityId
			rs.IDPMetadata = ss.IdpMetadataFile
			rs.SPEntityID = ss.SpEntityId
			rs.KibanaURL = ss.KibanaUrl
			rs.Principal = ss.AttributePrincipal
			rs.Groups = ss.AttributeGroups
			rs.Name = ss.AttributeName
			rs.Mail = ss.AttributeEmail
			rs.DN = ss.AttributeDn

		case esmodels.AuthProviderTypeNative,
			esmodels.AuthProviderTypeOpenID,
			esmodels.AuthProviderTypeAnonymous:
			return nil, semerr.InvalidInput("mismatch provider type and settings")
		}
	}

	return res, nil
}

var (
	authProviderTypeToGRPC = map[esmodels.AuthProviderType]esv1.AuthProvider_Type{
		esmodels.AuthProviderTypeNative: esv1.AuthProvider_NATIVE,
		esmodels.AuthProviderTypeSaml:   esv1.AuthProvider_SAML,
	}
	authProviderTypeFromGRPC = reflectutil.ReverseMap(authProviderTypeToGRPC).(map[esv1.AuthProvider_Type]esmodels.AuthProviderType)
)

func AuthProviderTypeFromGRPC(t esv1.AuthProvider_Type) (esmodels.AuthProviderType, error) {
	if r, ok := authProviderTypeFromGRPC[t]; ok {
		return r, nil
	}

	return "", semerr.Internal("unknown auth provider type")
}

func AuthProviderTypeToGRPC(t esmodels.AuthProviderType) esv1.AuthProvider_Type {
	if r, ok := authProviderTypeToGRPC[t]; ok {
		return r
	}
	return esv1.AuthProvider_TYPE_UNSPECIFIED
}

func BackupToGRPC(backup backups.Backup) *esv1.Backup {
	meta := backup.Metadata.(esmodels.SnapshotInfo)
	return &esv1.Backup{
		Id:                   backup.GlobalBackupID(),
		FolderId:             backup.FolderID,
		SourceClusterId:      backup.SourceClusterID,
		CreatedAt:            grpc.TimeToGRPC(backup.CreatedAt),
		StartedAt:            grpc.OptionalTimeToGRPC(backup.StartedAt),
		ElasticsearchVersion: meta.ElasticVersion,
		SizeBytes:            int64(meta.Size),
		Indices:              meta.Indices,
		IndicesTotal:         int64(meta.IndicesTotal),
	}
}

func BackupsToGRPC(backups []backups.Backup) []*esv1.Backup {
	v := make([]*esv1.Backup, 0, len(backups))
	for _, b := range backups {
		v = append(v, BackupToGRPC(b))
	}
	return v
}

func RescheduleTypeFromGRPC(rescheduleType esv1.RescheduleMaintenanceRequest_RescheduleType) (clusters.RescheduleType, error) {
	fromGRPCMap := map[esv1.RescheduleMaintenanceRequest_RescheduleType]clusters.RescheduleType{
		esv1.RescheduleMaintenanceRequest_RESCHEDULE_TYPE_UNSPECIFIED: clusters.RescheduleTypeUnspecified,
		esv1.RescheduleMaintenanceRequest_IMMEDIATE:                   clusters.RescheduleTypeImmediate,
		esv1.RescheduleMaintenanceRequest_NEXT_AVAILABLE_WINDOW:       clusters.RescheduleTypeNextAvailableWindow,
		esv1.RescheduleMaintenanceRequest_SPECIFIC_TIME:               clusters.RescheduleTypeSpecificTime,
	}

	v, ok := fromGRPCMap[rescheduleType]
	if !ok {
		return clusters.RescheduleTypeUnspecified, semerr.InvalidInput("unknown reschedule type")
	}

	return v, nil
}

func ExtensionToGRPC(cid string, extension esmodels.Extension) *esv1.Extension {
	return &esv1.Extension{
		ClusterId: cid,
		Id:        extension.ID,
		Name:      extension.Name,
		Version:   int64(extension.Version),
		Active:    extension.Active,
	}
}

func ExtensionsToGRPC(cid string, extensions []esmodels.Extension) []*esv1.Extension {
	v := make([]*esv1.Extension, 0, len(extensions))
	for _, e := range extensions {
		v = append(v, ExtensionToGRPC(cid, e))
	}
	return v
}

func ExtensionSpecFromGRPC(spec *esv1.ExtensionSpec) esmodels.ExtensionSpec {
	return esmodels.ExtensionSpec{
		Name:     spec.Name,
		URI:      spec.Uri,
		Disabled: spec.Disabled,
	}
}

func ExtensionSpecsFromGRPC(specs []*esv1.ExtensionSpec) []esmodels.ExtensionSpec {
	v := make([]esmodels.ExtensionSpec, 0, len(specs))
	for _, e := range specs {
		v = append(v, ExtensionSpecFromGRPC(e))
	}
	return v
}

func AccessFromGRPC(a *esv1.Access) (access esmodels.OptionalAccess) {
	if a.GetDataTransfer() {
		access.Value.DataTransfer = optional.NewBool(a.GetDataTransfer())
		access.Valid = true
	}
	return
}
