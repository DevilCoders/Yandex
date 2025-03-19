package sqlserver

import (
	"context"
	"reflect"
	"sort"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes"
	fieldmaskutils "github.com/mennanov/fieldmask-utils"
	"google.golang.org/genproto/googleapis/type/timeofday"

	ssv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1"
	config "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1/config"
	ssv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1/console"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil/converters"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/ssmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/services"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/system"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var databaseRoleToGRPC = map[ssmodels.DatabaseRoleType]ssv1.Permission_Role{
	ssmodels.DatabaseRoleOwner:          ssv1.Permission_DB_OWNER,
	ssmodels.DatabaseRoleSecurityAdmin:  ssv1.Permission_DB_SECURITYADMIN,
	ssmodels.DatabaseRoleAccessAdmin:    ssv1.Permission_DB_ACCESSADMIN,
	ssmodels.DatabaseRoleBackupOperator: ssv1.Permission_DB_BACKUPOPERATOR,
	ssmodels.DatabaseRoleDDLAdmin:       ssv1.Permission_DB_DDLADMIN,
	ssmodels.DatabaseRoleDataWriter:     ssv1.Permission_DB_DATAWRITER,
	ssmodels.DatabaseRoleDataReader:     ssv1.Permission_DB_DATAREADER,
	ssmodels.DatabaseRoleDenyDataWriter: ssv1.Permission_DB_DENYDATAWRITER,
	ssmodels.DatabaseRoleDenyDataReader: ssv1.Permission_DB_DENYDATAREADER,
}

var serverRoleToGRPC = map[ssmodels.ServerRoleType]ssv1.ServerRole{
	ssmodels.ServerRoleMdbMonitor:  ssv1.ServerRole_MDB_MONITOR,
	ssmodels.ServerRoleUnspecified: ssv1.ServerRole_SERVER_ROLE_UNSPECIFIED,
}

var (
	serviceTypeMap = map[services.Type]ssv1.Service_Type{
		services.TypeSQLServer:          ssv1.Service_SQLSERVER,
		services.TypeWindowsWitnessNode: ssv1.Service_WITNESS,
		services.TypeUnknown:            ssv1.Service_TYPE_UNSPECIFIED,
	}
)

var (
	serviceHealthMap = map[services.Status]ssv1.Service_Health{
		services.StatusAlive:   ssv1.Service_ALIVE,
		services.StatusDead:    ssv1.Service_DEAD,
		services.StatusUnknown: ssv1.Service_HEALTH_UNKNOWN,
	}
)

var databaseRoleFromGRPC = reflectutil.ReverseMap(databaseRoleToGRPC).(map[ssv1.Permission_Role]ssmodels.DatabaseRoleType)

var serverRoleFromGRPC = reflectutil.ReverseMap(serverRoleToGRPC).(map[ssv1.ServerRole]ssmodels.ServerRoleType)

func databaseRolesToGRPC(roles []ssmodels.DatabaseRoleType) (res []ssv1.Permission_Role) {
	for _, role := range roles {
		if v, ok := databaseRoleToGRPC[role]; ok {
			res = append(res, v)
		}
	}
	return
}

func databaseRolesFromGRPC(roles []ssv1.Permission_Role) (res []ssmodels.DatabaseRoleType) {
	for _, role := range roles {
		if v, ok := databaseRoleFromGRPC[role]; ok {
			res = append(res, v)
		}
	}
	return
}

func serverRolesToGRPC(roles []ssmodels.ServerRoleType) (res []ssv1.ServerRole) {
	for _, role := range roles {
		if v, ok := serverRoleToGRPC[role]; ok {
			res = append(res, v)
		}
	}
	return
}

func serverRolesFromGRPC(roles []ssv1.ServerRole) (res []ssmodels.ServerRoleType) {
	for _, role := range roles {
		if v, ok := serverRoleFromGRPC[role]; ok {
			res = append(res, v)
		}
	}
	return
}

var (
	mapClusterHealthToGRPC = map[clusters.Health]ssv1.Cluster_Health{
		clusters.HealthAlive:    ssv1.Cluster_ALIVE,
		clusters.HealthDegraded: ssv1.Cluster_DEGRADED,
		clusters.HealthDead:     ssv1.Cluster_DEAD,
		clusters.HealthUnknown:  ssv1.Cluster_HEALTH_UNKNOWN,
	}
)

func ClusterHealthToGRPC(env clusters.Health) ssv1.Cluster_Health {
	v, ok := mapClusterHealthToGRPC[env]
	if !ok {
		return ssv1.Cluster_HEALTH_UNKNOWN
	}
	return v
}

var (
	mapStatusToGRCP = map[clusters.Status]ssv1.Cluster_Status{
		clusters.StatusCreating:             ssv1.Cluster_CREATING,
		clusters.StatusCreateError:          ssv1.Cluster_ERROR,
		clusters.StatusRunning:              ssv1.Cluster_RUNNING,
		clusters.StatusModifying:            ssv1.Cluster_UPDATING,
		clusters.StatusModifyError:          ssv1.Cluster_ERROR,
		clusters.StatusStopping:             ssv1.Cluster_STOPPING,
		clusters.StatusStopped:              ssv1.Cluster_STOPPED,
		clusters.StatusStopError:            ssv1.Cluster_ERROR,
		clusters.StatusStarting:             ssv1.Cluster_STARTING,
		clusters.StatusStartError:           ssv1.Cluster_ERROR,
		clusters.StatusMaintainingOffline:   ssv1.Cluster_UPDATING,
		clusters.StatusMaintainOfflineError: ssv1.Cluster_ERROR,
	}
)

func StatusToGRPC(status clusters.Status) ssv1.Cluster_Status {
	v, ok := mapStatusToGRCP[status]
	if !ok {
		return ssv1.Cluster_STATUS_UNKNOWN
	}
	return v
}

func clusterToGRPC(cluster ssmodels.Cluster, saltEnvMapper grpcapi.SaltEnvMapper) *ssv1.Cluster {
	v := &ssv1.Cluster{
		Id:                 cluster.ClusterID,
		FolderId:           cluster.FolderExtID,
		CreatedAt:          grpcapi.TimeToGRPC(cluster.CreatedAt),
		Config:             clusterConfigToGRPC(cluster.Config),
		Name:               cluster.Name,
		Description:        cluster.Description,
		Labels:             cluster.Labels,
		Environment:        ssv1.Cluster_Environment(saltEnvMapper.ToGRPC(cluster.Environment)),
		NetworkId:          cluster.NetworkID,
		SecurityGroupIds:   cluster.SecurityGroupIDs,
		Sqlcollation:       cluster.SQLCollation,
		Health:             ClusterHealthToGRPC(cluster.Health),
		Status:             StatusToGRPC(cluster.Status),
		DeletionProtection: cluster.DeletionProtection,
		HostGroupIds:       cluster.HostGroupIDs,
		ServiceAccountId:   cluster.ServiceAccountID,
	}
	v.Monitoring = make([]*ssv1.Monitoring, 0, len(cluster.Monitoring.Charts))
	for _, chart := range cluster.Monitoring.Charts {
		mon := &ssv1.Monitoring{
			Name:        chart.Name,
			Description: chart.Description,
			Link:        chart.Link,
		}
		v.Monitoring = append(v.Monitoring, mon)
	}
	// TODO: set Config
	return v
}

func clustersToGRPC(clusters []ssmodels.Cluster, saltEnvMapper grpcapi.SaltEnvMapper) []*ssv1.Cluster {
	v := make([]*ssv1.Cluster, 0, len(clusters))
	for _, cluster := range clusters {
		v = append(v, clusterToGRPC(cluster, saltEnvMapper))
	}
	return v
}

func userSpecFromGRPC(spec *ssv1.UserSpec) ssmodels.UserSpec {
	us := ssmodels.UserSpec{Name: spec.Name, Password: secret.NewString(spec.Password)}
	for _, perm := range spec.Permissions {
		us.Permissions = append(us.Permissions, ssmodels.Permission{
			DatabaseName: perm.GetDatabaseName(),
			Roles:        databaseRolesFromGRPC(perm.GetRoles()),
		})
	}
	us.ServerRoles = serverRolesFromGRPC(spec.ServerRoles)
	return us
}

func userSpecsFromGRPC(specs []*ssv1.UserSpec) []ssmodels.UserSpec {
	v := make([]ssmodels.UserSpec, 0, len(specs))
	for _, spec := range specs {
		if spec != nil {
			v = append(v, userSpecFromGRPC(spec))
		}
	}
	return v
}

func userToGRPC(user ssmodels.User) *ssv1.User {
	v := &ssv1.User{Name: user.Name, ClusterId: user.ClusterID}
	for _, perm := range user.Permissions {
		v.Permissions = append(v.Permissions, &ssv1.Permission{
			DatabaseName: perm.DatabaseName,
			Roles:        databaseRolesToGRPC(perm.Roles),
		})
	}
	v.ServerRoles = serverRolesToGRPC(user.ServerRoles)
	return v
}

func usersToGRPC(users []ssmodels.User) []*ssv1.User {
	v := make([]*ssv1.User, 0, len(users))
	for _, user := range users {
		v = append(v, userToGRPC(user))
	}
	return v
}

func databaseSpecFromGRPC(spec *ssv1.DatabaseSpec) ssmodels.DatabaseSpec {
	return ssmodels.DatabaseSpec{
		Name: spec.GetName(),
	}
}

func restoreDatabaseSpecFromGRPC(req *ssv1.RestoreDatabaseRequest) ssmodels.RestoreDatabaseSpec {
	DBSpec := ssmodels.DatabaseSpec{
		Name: req.DatabaseName,
	}
	spec := ssmodels.RestoreDatabaseSpec{
		DatabaseSpec: DBSpec,
		FromDatabase: req.FromDatabase,
		BackupID:     req.BackupId,
		Time:         req.Time.AsTime(),
	}
	return spec
}

func importDatabaseBackupSpecFromGRPC(req *ssv1.ImportDatabaseBackupRequest) ssmodels.ImportDatabaseBackupSpec {
	DBSpec := ssmodels.DatabaseSpec{
		Name: req.DatabaseName,
	}
	spec := ssmodels.ImportDatabaseBackupSpec{
		DatabaseSpec: DBSpec,
		S3Bucket:     req.S3Bucket,
		S3Path:       req.S3Path,
		Files:        req.GetFiles(),
	}
	if spec.S3Path == "" {
		spec.S3Path = "/"
	}
	return spec
}

func exportDatabaseBackupSpecFromGRPC(req *ssv1.ExportDatabaseBackupRequest) ssmodels.ExportDatabaseBackupSpec {
	DBSpec := ssmodels.DatabaseSpec{
		Name: req.DatabaseName,
	}
	spec := ssmodels.ExportDatabaseBackupSpec{
		DatabaseSpec: DBSpec,
		S3Bucket:     req.S3Bucket,
		S3Path:       req.S3Path,
		Prefix:       req.Prefix,
	}
	if spec.S3Path == "" {
		spec.S3Path = "/"
	}
	if spec.Prefix == "" {
		spec.Prefix = req.DatabaseName
	}
	return spec
}

func databaseSpecsFromGRPC(specs []*ssv1.DatabaseSpec) []ssmodels.DatabaseSpec {
	v := make([]ssmodels.DatabaseSpec, 0, len(specs))
	for _, spec := range specs {
		if spec != nil {
			v = append(v, databaseSpecFromGRPC(spec))
		}
	}
	return v
}

func databaseToGRPC(db ssmodels.Database) *ssv1.Database {
	return &ssv1.Database{
		Name:      db.Name,
		ClusterId: db.ClusterID,
	}
}

func databasesToGRPC(dbs []ssmodels.Database) []*ssv1.Database {
	v := make([]*ssv1.Database, 0, len(dbs))
	for _, tp := range dbs {
		v = append(v, databaseToGRPC(tp))
	}
	return v
}

func configFromGRPC(spec, conf interface{}, names []string) {
	reflectutil.CopyStructFields(spec, conf, reflectutil.CopyStructFieldsConfig{
		IsValidField:        reflectutil.FieldsInListAreValid(names),
		Convert:             converters.OptionalFromGRPC,
		PanicOnMissingField: reflectutil.IgnoreMissingFields,
		CaseInsensitive:     true,
	})
}

func configToGRPC(conf, spec interface{}) {
	reflectutil.CopyStructFields(conf, spec, reflectutil.CopyStructFieldsConfig{
		IsValidField:        reflectutil.ValidOptionalFieldsAreValid,
		Convert:             converters.OptionalToGRPC,
		PanicOnMissingField: reflectutil.IgnoreMissingFields,
		CaseInsensitive:     true,
	})
}

func configSetToGRPC(configSet ssmodels.ConfigSetSQLServer, spec interface{}) {
	csv := reflect.ValueOf(configSet)
	sv := reflect.ValueOf(spec).Elem()
	fields := []string{"DefaultConfig", "UserConfig", "EffectiveConfig"}
	for _, field := range fields {
		if !csv.FieldByName(field).IsNil() {
			csfv := csv.FieldByName(field).Elem()
			sfv := sv.FieldByName(field)
			sfv.Set(reflect.New(sfv.Type().Elem()))
			configToGRPC(csfv.Interface(), sfv.Interface())
		}
	}
}

func secondaryConnectionsToGRPC(mode ssmodels.SecondaryConnectionsMode) ssv1.ClusterConfig_SecondaryConnections {
	if mode == ssmodels.SecondaryConnectionsReadOnly {
		return ssv1.ClusterConfig_SECONDARY_CONNECTIONS_READ_ONLY
	}
	return ssv1.ClusterConfig_SECONDARY_CONNECTIONS_OFF
}

func secondaryConnectionsFromGRPC(mode ssv1.ClusterConfig_SecondaryConnections) ssmodels.SecondaryConnectionsMode {
	if mode == ssv1.ClusterConfig_SECONDARY_CONNECTIONS_READ_ONLY {
		return ssmodels.SecondaryConnectionsReadOnly
	}
	return ssmodels.SecondaryConnectionsOff
}

func clusterConfigSpecFromGRPC(cfg *ssv1.ConfigSpec, paths *grpcapi.FieldPaths) (ssmodels.ClusterConfigSpec, error) {
	ccs := ssmodels.ClusterConfigSpec{}

	if paths.Remove("version") {
		ccs.Version.Set(cfg.GetVersion())
	}

	resourcePaths := paths.Subtree("resources.")
	if cfg.GetResources() != nil && !resourcePaths.Empty() {
		ccs.Resources = grpcapi.ResourcesFromGRPC(cfg.GetResources(), resourcePaths)
	}

	if cfg.BackupWindowStart != nil && paths.Remove("backup_window_start") {
		ccs.BackupWindowStart.Set(backups.BackupWindowStart{
			Hours:   int(cfg.BackupWindowStart.Hours),
			Minutes: int(cfg.BackupWindowStart.Minutes),
			Seconds: int(cfg.BackupWindowStart.Seconds),
			Nanos:   int(cfg.BackupWindowStart.Nanos),
		})
	}
	if paths.Remove("secondary_connections") {
		ccs.SecondaryConnections.Set(secondaryConnectionsFromGRPC(cfg.SecondaryConnections))
	}
	if cfg.Access != nil && paths.Remove("access.data_lens") {
		ccs.Access.DataLens = optional.NewBool(cfg.Access.DataLens)
	}
	if cfg.Access != nil && paths.Remove("access.data_transfer") {
		ccs.Access.DataTransfer = optional.NewBool(cfg.Access.DataTransfer)
	}

	switch sscfg := cfg.GetSqlserverConfig(); sscfg.(type) {
	case *ssv1.ConfigSpec_SqlserverConfig_2016Sp2Dev:
		cfgPaths := paths.Subtree("sqlserver_config_2016sp2dev.")
		if !cfgPaths.Empty() {
			sscfg2016sp2dev := sscfg.(*ssv1.ConfigSpec_SqlserverConfig_2016Sp2Dev).SqlserverConfig_2016Sp2Dev
			ccs.Config = configFromPaths(sscfg2016sp2dev, &ssmodels.ConfigBase{}, cfgPaths)
		}
	case *ssv1.ConfigSpec_SqlserverConfig_2016Sp2Std:
		cfgPaths := paths.Subtree("sqlserver_config_2016sp2std.")
		if !cfgPaths.Empty() {
			sscfg2016sp2std := sscfg.(*ssv1.ConfigSpec_SqlserverConfig_2016Sp2Std).SqlserverConfig_2016Sp2Std
			ccs.Config = configFromPaths(sscfg2016sp2std, &ssmodels.ConfigBase{}, cfgPaths)
		}
	case *ssv1.ConfigSpec_SqlserverConfig_2016Sp2Ent:
		cfgPaths := paths.Subtree("sqlserver_config_2016sp2ent.")
		if !cfgPaths.Empty() {
			sscfg2016sp2ent := sscfg.(*ssv1.ConfigSpec_SqlserverConfig_2016Sp2Ent).SqlserverConfig_2016Sp2Ent
			ccs.Config = configFromPaths(sscfg2016sp2ent, &ssmodels.ConfigBase{}, cfgPaths)
		}
	case *ssv1.ConfigSpec_SqlserverConfig_2017Dev:
		cfgPaths := paths.Subtree("sqlserver_config_2017dev.")
		if !cfgPaths.Empty() {
			sscfg2017dev := sscfg.(*ssv1.ConfigSpec_SqlserverConfig_2017Dev).SqlserverConfig_2017Dev
			ccs.Config = configFromPaths(sscfg2017dev, &ssmodels.ConfigBase{}, cfgPaths)
		}
	case *ssv1.ConfigSpec_SqlserverConfig_2017Std:
		cfgPaths := paths.Subtree("sqlserver_config_2017std.")
		if !cfgPaths.Empty() {
			sscfg2017std := sscfg.(*ssv1.ConfigSpec_SqlserverConfig_2017Std).SqlserverConfig_2017Std
			ccs.Config = configFromPaths(sscfg2017std, &ssmodels.ConfigBase{}, cfgPaths)
		}
	case *ssv1.ConfigSpec_SqlserverConfig_2017Ent:
		cfgPaths := paths.Subtree("sqlserver_config_2017ent.")
		if !cfgPaths.Empty() {
			sscfg2017ent := sscfg.(*ssv1.ConfigSpec_SqlserverConfig_2017Ent).SqlserverConfig_2017Ent
			ccs.Config = configFromPaths(sscfg2017ent, &ssmodels.ConfigBase{}, cfgPaths)
		}
	case *ssv1.ConfigSpec_SqlserverConfig_2019Dev:
		cfgPaths := paths.Subtree("sqlserver_config_2019dev.")
		if !cfgPaths.Empty() {
			sscfg2019dev := sscfg.(*ssv1.ConfigSpec_SqlserverConfig_2019Dev).SqlserverConfig_2019Dev
			ccs.Config = configFromPaths(sscfg2019dev, &ssmodels.ConfigBase{}, cfgPaths)
		}
	case *ssv1.ConfigSpec_SqlserverConfig_2019Std:
		cfgPaths := paths.Subtree("sqlserver_config_2019std.")
		if !cfgPaths.Empty() {
			sscfg2019std := sscfg.(*ssv1.ConfigSpec_SqlserverConfig_2019Std).SqlserverConfig_2019Std
			ccs.Config = configFromPaths(sscfg2019std, &ssmodels.ConfigBase{}, cfgPaths)
		}
	case *ssv1.ConfigSpec_SqlserverConfig_2019Ent:
		cfgPaths := paths.Subtree("sqlserver_config_2019ent.")
		if !cfgPaths.Empty() {
			sscfg2019ent := sscfg.(*ssv1.ConfigSpec_SqlserverConfig_2019Ent).SqlserverConfig_2019Ent
			ccs.Config = configFromPaths(sscfg2019ent, &ssmodels.ConfigBase{}, cfgPaths)
		}
	default:
		// do nothing, validate later
	}
	return ccs, nil
}

func configFromPaths(spec interface{}, conf ssmodels.SQLServerConfig, cfgPaths *grpcapi.FieldPaths) ssmodels.OptionalSQLServerConfig {
	fields := grpcapi.FieldNamesFromGRPCPaths(spec, cfgPaths)
	configFromGRPC(spec, conf, fields)
	return ssmodels.NewOptionalSQLServerConfig(conf, fields)
}

func clusterConfigToGRPC(clusterConfig ssmodels.ClusterConfig) *ssv1.ClusterConfig {
	cc := &ssv1.ClusterConfig{
		Version:         clusterConfig.Version,
		SqlserverConfig: nil,
		Resources:       nil,
		BackupWindowStart: &timeofday.TimeOfDay{
			Hours:   int32(clusterConfig.BackupWindowStart.Hours),
			Minutes: int32(clusterConfig.BackupWindowStart.Minutes),
			Seconds: int32(clusterConfig.BackupWindowStart.Seconds),
			Nanos:   int32(clusterConfig.BackupWindowStart.Nanos),
		},
		SecondaryConnections: secondaryConnectionsToGRPC(clusterConfig.SecondaryConnections),
		Access: &ssv1.Access{
			DataLens:     clusterConfig.Access.DataLens,
			DataTransfer: clusterConfig.Access.DataTransfer,
		},
	}
	cc.Resources = &ssv1.Resources{
		ResourcePresetId: clusterConfig.Resources.ResourcePresetExtID,
		DiskSize:         clusterConfig.Resources.DiskSize,
		DiskTypeId:       clusterConfig.Resources.DiskTypeExtID,
	}
	cc.Version = clusterConfig.Version
	switch cc.Version {
	case ssmodels.Version2016sp2dev:
		cfgset := &config.SQLServerConfigSet2016Sp2Dev{}
		configSetToGRPC(clusterConfig.ConfigSet, cfgset)
		cc.SqlserverConfig = &ssv1.ClusterConfig_SqlserverConfig_2016Sp2Dev{
			SqlserverConfig_2016Sp2Dev: cfgset,
		}
	case ssmodels.Version2016sp2std:
		cfgset := &config.SQLServerConfigSet2016Sp2Std{}
		configSetToGRPC(clusterConfig.ConfigSet, cfgset)
		cc.SqlserverConfig = &ssv1.ClusterConfig_SqlserverConfig_2016Sp2Std{
			SqlserverConfig_2016Sp2Std: cfgset,
		}
	case ssmodels.Version2016sp2ent:
		cfgset := &config.SQLServerConfigSet2016Sp2Ent{}
		configSetToGRPC(clusterConfig.ConfigSet, cfgset)
		cc.SqlserverConfig = &ssv1.ClusterConfig_SqlserverConfig_2016Sp2Ent{
			SqlserverConfig_2016Sp2Ent: cfgset,
		}
	case ssmodels.Version2017dev:
		cfgset := &config.SQLServerConfigSet2017Dev{}
		configSetToGRPC(clusterConfig.ConfigSet, cfgset)
		cc.SqlserverConfig = &ssv1.ClusterConfig_SqlserverConfig_2017Dev{
			SqlserverConfig_2017Dev: cfgset,
		}
	case ssmodels.Version2017std:
		cfgset := &config.SQLServerConfigSet2017Std{}
		configSetToGRPC(clusterConfig.ConfigSet, cfgset)
		cc.SqlserverConfig = &ssv1.ClusterConfig_SqlserverConfig_2017Std{
			SqlserverConfig_2017Std: cfgset,
		}
	case ssmodels.Version2017ent:
		cfgset := &config.SQLServerConfigSet2017Ent{}
		configSetToGRPC(clusterConfig.ConfigSet, cfgset)
		cc.SqlserverConfig = &ssv1.ClusterConfig_SqlserverConfig_2017Ent{
			SqlserverConfig_2017Ent: cfgset,
		}
	case ssmodels.Version2019dev:
		cfgset := &config.SQLServerConfigSet2019Dev{}
		configSetToGRPC(clusterConfig.ConfigSet, cfgset)
		cc.SqlserverConfig = &ssv1.ClusterConfig_SqlserverConfig_2019Dev{
			SqlserverConfig_2019Dev: cfgset,
		}
	case ssmodels.Version2019std:
		cfgset := &config.SQLServerConfigSet2019Std{}
		configSetToGRPC(clusterConfig.ConfigSet, cfgset)
		cc.SqlserverConfig = &ssv1.ClusterConfig_SqlserverConfig_2019Std{
			SqlserverConfig_2019Std: cfgset,
		}
	case ssmodels.Version2019ent:
		cfgset := &config.SQLServerConfigSet2019Ent{}
		configSetToGRPC(clusterConfig.ConfigSet, cfgset)
		cc.SqlserverConfig = &ssv1.ClusterConfig_SqlserverConfig_2019Ent{
			SqlserverConfig_2019Ent: cfgset,
		}
	default:
		// we don't fail here without config and version, because we fail creation clusters without it on validation
	}
	return cc
}

func hostSpecsFromGRPC(specs []*ssv1.HostSpec) []ssmodels.HostSpec {
	v := make([]ssmodels.HostSpec, 0, len(specs))
	for _, spec := range specs {
		v = append(v, hostSpecFromGRPC(spec))
	}
	return v
}

func hostSpecFromGRPC(spec *ssv1.HostSpec) ssmodels.HostSpec {
	return ssmodels.HostSpec{
		ZoneID:         spec.ZoneId,
		SubnetID:       optional.String{String: spec.SubnetId, Valid: spec.SubnetId != ""},
		AssignPublicIP: spec.AssignPublicIp,
	}
}

func UpdateHostSpecsFromGRPC(specs []*ssv1.UpdateHostSpec) ([]ssmodels.UpdateHostSpec, error) {
	var result []ssmodels.UpdateHostSpec
	for _, s := range specs {
		mask, err := fieldmaskutils.MaskFromProtoFieldMask(s.UpdateMask, func(s string) string { return s })
		if err != nil {
			return []ssmodels.UpdateHostSpec{}, semerr.WrapWithInvalidInputf(err, "invalid field mask for UpdateHostSpec")
		}
		var spec = ssmodels.UpdateHostSpec{
			HostName: s.HostName,
		}
		if _, ok := mask.Filter("assign_public_ip"); ok {
			spec.AssignPublicIP = s.AssignPublicIp
		}
		result = append(result, spec)
	}
	return result, nil
}

var (
	mapHostStatusToGRPC = map[hosts.Status]ssv1.Host_Health{
		hosts.StatusAlive:    ssv1.Host_ALIVE,
		hosts.StatusDegraded: ssv1.Host_DEGRADED,
		hosts.StatusDead:     ssv1.Host_DEAD,
		hosts.StatusUnknown:  ssv1.Host_HEALTH_UNKNOWN,
	}
)

func HostStatusToGRPC(h hosts.Health) ssv1.Host_Health {
	v, ok := mapHostStatusToGRPC[h.Status]
	if !ok {
		return ssv1.Host_HEALTH_UNKNOWN
	}
	return v
}

func HostRoleToGRPC(h hosts.Health) ssv1.Host_Role {
	for _, svc := range h.Services {
		if svc.Type == services.TypeSQLServer {
			switch svc.Role {
			case services.RoleMaster:
				return ssv1.Host_MASTER
			case services.RoleReplica:
				return ssv1.Host_REPLICA
			default:
				return ssv1.Host_ROLE_UNKNOWN
			}
		}
	}
	return ssv1.Host_ROLE_UNKNOWN
}

func cpuMetricToGRPC(cpu *system.CPUMetric) *ssv1.Host_CPUMetric {
	if cpu == nil {
		return nil
	}

	return &ssv1.Host_CPUMetric{
		Timestamp: cpu.Timestamp,
		Used:      cpu.Used,
	}
}

func memoryMetricToGRPC(memory *system.MemoryMetric) *ssv1.Host_MemoryMetric {
	if memory == nil {
		return nil
	}

	return &ssv1.Host_MemoryMetric{
		Timestamp: memory.Timestamp,
		Used:      memory.Used,
		Total:     memory.Total,
	}
}

func diskMetricToGRPC(disk *system.DiskMetric) *ssv1.Host_DiskMetric {
	if disk == nil {
		return nil
	}

	return &ssv1.Host_DiskMetric{
		Timestamp: disk.Timestamp,
		Used:      disk.Used,
		Total:     disk.Total,
	}
}

func HostSystemMetricsToGRPC(sm *system.Metrics) *ssv1.Host_SystemMetrics {
	if sm == nil {
		return nil
	}

	return &ssv1.Host_SystemMetrics{
		Cpu:    cpuMetricToGRPC(sm.CPU),
		Memory: memoryMetricToGRPC(sm.Memory),
		Disk:   diskMetricToGRPC(sm.Disk),
	}
}

func serviceTypeToGRPC(t services.Type) ssv1.Service_Type {
	v, ok := serviceTypeMap[t]
	if !ok {
		return ssv1.Service_TYPE_UNSPECIFIED
	}
	return v
}

func serviceHealthToGRPC(s services.Status) ssv1.Service_Health {
	v, ok := serviceHealthMap[s]
	if !ok {
		return ssv1.Service_HEALTH_UNKNOWN
	}
	return v
}

func hostServicesToGRPC(hh hosts.Health) []*ssv1.Service {
	var result []*ssv1.Service
	for _, service := range hh.Services {
		result = append(result, &ssv1.Service{
			Type:   serviceTypeToGRPC(service.Type),
			Health: serviceHealthToGRPC(service.Status),
		})
	}

	return result
}

func hostToGRPC(host hosts.HostExtended) *ssv1.Host {
	h := &ssv1.Host{
		Name:      host.FQDN,
		ClusterId: host.ClusterID,
		ZoneId:    host.ZoneID,
		Resources: &ssv1.Resources{
			ResourcePresetId: host.ResourcePresetExtID,
			DiskSize:         host.SpaceLimit,
			DiskTypeId:       host.DiskTypeExtID,
		},
		Services:       hostServicesToGRPC(host.Health),
		Role:           HostRoleToGRPC(host.Health),
		Health:         HostStatusToGRPC(host.Health),
		System:         HostSystemMetricsToGRPC(host.Health.System),
		SubnetId:       host.SubnetID,
		AssignPublicIp: host.AssignPublicIP,
	}
	return h
}

func hostsToGRPC(hosts []hosts.HostExtended) []*ssv1.Host {
	v := make([]*ssv1.Host, 0, len(hosts))
	for _, host := range hosts {
		v = append(v, hostToGRPC(host))
	}
	return v
}

func userUpdateFromGRPC(request *ssv1.UpdateUserRequest) (sqlserver.UserArgs, error) {
	usrUpdate := sqlserver.UserArgs{
		Name:      request.GetUserName(),
		ClusterID: request.GetClusterId(),
	}
	paths := grpcapi.NewFieldPaths(request.UpdateMask.GetPaths())
	if paths.Empty() {
		return sqlserver.UserArgs{}, semerr.InvalidInput("no changes detected")
	}

	if paths.Remove("password") {
		usrUpdate.Password.Set(secret.NewString(request.GetPassword()))
	}

	if paths.Remove("permissions") {
		reqPermissions := make([]ssmodels.Permission, 0, len(request.GetPermissions()))
		for _, p := range request.GetPermissions() {
			reqPermissions = append(reqPermissions, ssmodels.Permission{
				DatabaseName: p.GetDatabaseName(),
				Roles:        databaseRolesFromGRPC(p.GetRoles()),
			})
		}
		usrUpdate.Permissions.Set(reqPermissions)
	}

	if paths.Remove("server_roles") {
		usrUpdate.ServerRoles = ssmodels.NewOptionalServerRoles(serverRolesFromGRPC(request.GetServerRoles()))
	}

	err := paths.MustBeEmpty()
	if err != nil {
		return sqlserver.UserArgs{}, err
	}
	return usrUpdate, nil
}

func backupToGRPC(backup backups.Backup) *ssv1.Backup {
	b := &ssv1.Backup{
		Id:              backup.GlobalBackupID(),
		SourceClusterId: backup.SourceClusterID,
		Databases:       backup.Databases,
		FolderId:        backup.FolderID,
		CreatedAt:       grpcapi.TimeToGRPC(backup.CreatedAt),
	}
	if backup.StartedAt.Valid {
		b.StartedAt = grpcapi.TimeToGRPC(backup.StartedAt.Must())
	}
	return b
}

func backupsToGRPC(backups []backups.Backup) []*ssv1.Backup {
	v := make([]*ssv1.Backup, 0, len(backups))
	for _, b := range backups {
		v = append(v, backupToGRPC(b))
	}
	return v
}

func clusterModifyArgsFromGRPC(request *ssv1.UpdateClusterRequest) (sqlserver.ModifyClusterArgs, error) {
	if request.GetClusterId() == "" {
		return sqlserver.ModifyClusterArgs{}, semerr.InvalidInput("cluster id need to be set in cluster update request")
	}

	modifyCluster := sqlserver.ModifyClusterArgs{ClusterID: request.GetClusterId()}
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
	if paths.Remove("deletion_protection") {
		modifyCluster.DeletionProtection.Set(request.GetDeletionProtection())
	}

	configPaths := paths.Subtree("config_spec.")
	if request.GetConfigSpec() != nil && !configPaths.Empty() {
		configSpec, err := clusterConfigSpecFromGRPC(request.GetConfigSpec(), configPaths)
		if err != nil {
			return sqlserver.ModifyClusterArgs{}, err
		}
		modifyCluster.ClusterConfigSpec = configSpec
	}

	if paths.Remove("security_group_ids") {
		modifyCluster.SecurityGroupsIDs.Set(request.GetSecurityGroupIds())
	}
	if paths.Remove("deletion_protection") {
		modifyCluster.DeletionProtection.Set(request.GetDeletionProtection())
	}
	if paths.Remove("service_account_id") {
		modifyCluster.ServiceAccountID.Set(request.GetServiceAccountId())
	}

	err := paths.MustBeEmpty()
	if err != nil {
		return sqlserver.ModifyClusterArgs{}, err
	}

	return modifyCluster, nil
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

func operationToGRPC(ctx context.Context, op operations.Operation, ss sqlserver.SQLServer, saltEnvMapper grpcapi.SaltEnvMapper, l log.Logger) (*operation.Operation, error) {
	opGrpc, err := grpcapi.OperationToGRPC(ctx, op, l)
	if err != nil {
		return nil, err
	}
	if op.Status != operations.StatusDone {
		return opGrpc, nil
	}

	switch v := op.MetaData.(type) {
	// Must be at first position before interfaces
	case ssmodels.MetadataDeleteCluster, ssmodels.MetadataDeleteDatabase, ssmodels.MetadataDeleteUser:
		return withEmptyResult(opGrpc)
	// TODO: remove type list
	case ssmodels.MetadataCreateCluster, ssmodels.MetadataRestoreCluster, ssmodels.MetadataBackupCluster, ssmodels.MetadataModifyCluster, ssmodels.MetadataStartCluster, ssmodels.MetadataStopCluster, ssmodels.MetadataStartClusterFailover:
		cluster, err := ss.Cluster(ctx, op.ClusterID)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, clusterToGRPC(cluster, saltEnvMapper))
	case ssmodels.DatabaseMetadata:
		dbName := v.GetDatabaseName()
		database, err := ss.Database(ctx, op.ClusterID, dbName)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, databaseToGRPC(database))
	case ssmodels.UserMetadata:
		userName := v.GetUserName()
		user, err := ss.User(ctx, op.ClusterID, userName)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, userToGRPC(user))
	}

	return opGrpc, nil
}

func consoleResourcePresetToSimple(crp *ssv1console.SQLServerClustersConfig_ResourcePreset) (res *ssv1.ResourcePreset) {
	zoneIds := make([]string, len(crp.Zones))
	for i, zone := range crp.Zones {
		zoneIds[i] = zone.ZoneId
	}
	sort.Strings(zoneIds)
	return &ssv1.ResourcePreset{
		Id:      crp.PresetId,
		ZoneIds: zoneIds,
		Cores:   crp.CpuLimit,
		Memory:  crp.MemoryLimit,
	}
}

func consoleResourcePresetsToSimple(crps []*ssv1console.SQLServerClustersConfig_ResourcePreset) (res []*ssv1.ResourcePreset) {
	res = make([]*ssv1.ResourcePreset, len(crps))
	for i, crp := range crps {
		res[i] = consoleResourcePresetToSimple(crp)
	}
	sort.Slice(res, func(i, j int) bool {
		return res[i].Id < res[j].Id
	})
	return res
}
