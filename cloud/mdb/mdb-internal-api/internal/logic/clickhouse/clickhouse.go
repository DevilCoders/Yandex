package clickhouse

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	modelsoptional "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/slices"
)

//go:generate ../../../../scripts/mockgen.sh ClickHouse

type ClickHouse interface {
	DataCloudCluster(ctx context.Context, cid string, sensitive bool) (chmodels.DataCloudCluster, error)
	DataCloudClusters(ctx context.Context, folderExtID string, pageSize int64, pageToken clusters.ClusterPageToken) ([]chmodels.DataCloudCluster, error)
	CreateDataCloudCluster(ctx context.Context, args CreateDataCloudClusterArgs) (operations.Operation, error)
	RestoreDataCloudCluster(ctx context.Context, globalBackupID string, args CreateDataCloudClusterArgs) (operations.Operation, error)
	ModifyDataCloudCluster(ctx context.Context, args UpdateDataCloudClusterArgs) (operations.Operation, error)

	MDBCluster(ctx context.Context, cid string) (chmodels.MDBCluster, error)
	MDBClusters(ctx context.Context, folderExtID string, pageSize int64, pageToken clusters.ClusterPageToken) ([]chmodels.MDBCluster, error)
	CreateMDBCluster(ctx context.Context, args CreateMDBClusterArgs) (operations.Operation, error)
	RestoreMDBCluster(ctx context.Context, globalBackupIDs []string, args RestoreMDBClusterArgs) (operations.Operation, error)
	UpdateMDBCluster(ctx context.Context, args UpdateMDBClusterArgs) (operations.Operation, error)
	DeleteCluster(ctx context.Context, cid string) (operations.Operation, error)
	AddZookeeper(ctx context.Context, cid string, resources models.ClusterResourcesSpec, hostSpecs []chmodels.HostSpec) (operations.Operation, error)
	StartCluster(ctx context.Context, cid string) (operations.Operation, error)
	StopCluster(ctx context.Context, cid string) (operations.Operation, error)
	MoveCluster(ctx context.Context, cid, destinationFolderID string) (operations.Operation, error)
	RescheduleMaintenance(ctx context.Context, cid string, rescheduleType clusters.RescheduleType, delayedUntil optional.Time) (operations.Operation, error)

	Backup(ctx context.Context, backupID string) (backups.Backup, error)
	FolderBackups(ctx context.Context, fid string, pageToken backups.BackupsPageToken, pageSize int64) ([]backups.Backup, backups.BackupsPageToken, error)
	ClusterBackups(ctx context.Context, cid string, pageToken backups.BackupsPageToken, pageSize int64) ([]backups.Backup, backups.BackupsPageToken, error)
	BackupCluster(ctx context.Context, cid string, name optional.String) (operations.Operation, error)

	ListHosts(ctx context.Context, cid string, pageSize, offset int64) ([]hosts.HostExtended, pagination.OffsetPageToken, error)
	AddHosts(ctx context.Context, cid string, specs []chmodels.HostSpec, copySchema bool) (operations.Operation, error)
	UpdateHosts(ctx context.Context, cid string, specs []chmodels.UpdateHostSpec) (operations.Operation, error)
	AddClickHouseHost(ctx context.Context, cid string, spec chmodels.HostSpec, copySchema bool) (operations.Operation, error)
	AddZookeeperHost(ctx context.Context, cid string, spec chmodels.HostSpec) (operations.Operation, error)
	DeleteHosts(ctx context.Context, cid string, fqdns []string) (operations.Operation, error)

	GetShard(ctx context.Context, cid string, name string) (chmodels.Shard, error)
	ListShards(ctx context.Context, cid string, pageSize int64, offset int64) ([]chmodels.Shard, pagination.OffsetPageToken, error)
	AddShard(ctx context.Context, cid string, args CreateShardArgs) (operations.Operation, error)
	DeleteShard(ctx context.Context, cid, name string) (operations.Operation, error)

	ShardGroup(ctx context.Context, cid, name string) (chmodels.ShardGroup, error)
	ShardGroups(ctx context.Context, cid string, pageSize, offset int64) ([]chmodels.ShardGroup, pagination.OffsetPageToken, error)
	CreateShardGroup(ctx context.Context, shardGroup chmodels.ShardGroup) (operations.Operation, error)
	UpdateShardGroup(ctx context.Context, update chmodels.UpdateShardGroupArgs) (operations.Operation, error)
	DeleteShardGroup(ctx context.Context, cid string, name string) (operations.Operation, error)

	CreateExternalDictionary(ctx context.Context, cid string, spec chmodels.Dictionary) (operations.Operation, error)
	UpdateExternalDictionary(ctx context.Context, cid string, spec chmodels.Dictionary) (operations.Operation, error)
	DeleteExternalDictionary(ctx context.Context, cid string, name string) (operations.Operation, error)

	User(ctx context.Context, cid, name string) (chmodels.User, error)
	Users(ctx context.Context, cid string, pageSize, offset int64) ([]chmodels.User, pagination.OffsetPageToken, error)
	CreateUser(ctx context.Context, cid string, spec chmodels.UserSpec) (operations.Operation, error)
	UpdateUser(ctx context.Context, cid string, name string, spec chmodels.UpdateUserArgs) (operations.Operation, error)
	DeleteUser(ctx context.Context, cid string, name string) (operations.Operation, error)
	GrantPermission(ctx context.Context, cid, name string, permission chmodels.Permission) (operations.Operation, error)
	RevokePermission(ctx context.Context, cid, name, database string) (operations.Operation, error)
	ResetCredentials(ctx context.Context, cid string) (operations.Operation, error)

	Database(ctx context.Context, cid, name string) (chmodels.Database, error)
	Databases(ctx context.Context, cid string, pageSize, offset int64) ([]chmodels.Database, pagination.OffsetPageToken, error)
	CreateDatabase(ctx context.Context, cid string, spec chmodels.DatabaseSpec) (operations.Operation, error)
	DeleteDatabase(ctx context.Context, cid string, name string) (operations.Operation, error)

	MLModel(ctx context.Context, cid, name string) (chmodels.MLModel, error)
	MLModels(ctx context.Context, cid string, pageSize, offset int64) ([]chmodels.MLModel, pagination.OffsetPageToken, error)
	CreateMLModel(ctx context.Context, cid string, name string, modelType chmodels.MLModelType, uri string) (operations.Operation, error)
	UpdateMLModel(ctx context.Context, cid string, name string, uri string) (operations.Operation, error)
	DeleteMLModel(ctx context.Context, cid string, name string) (operations.Operation, error)

	FormatSchema(ctx context.Context, cid, name string) (chmodels.FormatSchema, error)
	FormatSchemas(ctx context.Context, cid string, pageSize, offset int64) ([]chmodels.FormatSchema, pagination.OffsetPageToken, error)
	CreateFormatSchema(ctx context.Context, cid string, name string, modelType chmodels.FormatSchemaType, uri string) (operations.Operation, error)
	UpdateFormatSchema(ctx context.Context, cid string, name string, uri string) (operations.Operation, error)
	DeleteFormatSchema(ctx context.Context, cid string, name string) (operations.Operation, error)

	VersionIDs() []string
	Versions() []logic.Version
	DefaultVersion() logic.Version
	Version(ctx context.Context, metadb metadb.Backend, cid string) (string, error)

	EstimateCreateMDBCluster(ctx context.Context, args CreateMDBClusterArgs) (console.BillingEstimate, error)
	EstimateCreateDCCluster(ctx context.Context, args CreateDataCloudClusterArgs) (console.BillingEstimate, error)
}

type CreateMDBClusterArgs struct {
	FolderExtID        string
	Name               string
	Environment        environment.SaltEnv
	ClusterSpec        chmodels.MDBClusterSpec
	DatabaseSpecs      []chmodels.DatabaseSpec
	UserSpecs          []chmodels.UserSpec
	HostSpecs          []chmodels.HostSpec
	NetworkID          string
	Description        string
	Labels             map[string]string
	ShardName          string
	MaintenanceWindow  clusters.MaintenanceWindow
	ServiceAccountID   string
	SecurityGroupIDs   []string
	DeletionProtection bool
}

func (args *CreateMDBClusterArgs) ValidateAndSane() error {
	if err := models.ClusterNameValidator.ValidateString(args.Name); err != nil {
		return err
	}

	args.SecurityGroupIDs = slices.DedupStrings(args.SecurityGroupIDs)

	for _, db := range args.DatabaseSpecs {
		if err := db.Validate(); err != nil {
			return err
		}
	}

	for _, user := range args.UserSpecs {
		if err := user.Validate(); err != nil {
			return err
		}
	}

	if args.ShardName == "" {
		args.ShardName = chmodels.DefaultShardName
	}

	if err := chmodels.ValidateShardName(args.ShardName); err != nil {
		return err
	}

	return args.ClusterSpec.Config.ValidateAndSane()
}

type CreateDataCloudClusterArgs struct {
	ProjectID    string
	Name         string
	Description  string
	ClusterSpec  chmodels.DataCloudClusterSpec
	CloudType    environment.CloudType
	RegionID     string
	ReplicaCount int64
	ShardCount   optional.Int64
	NetworkID    optional.String
}

func (args *CreateDataCloudClusterArgs) ValidateAndSane() error {
	if err := models.ClusterNameValidator.ValidateString(args.Name); err != nil {
		return err
	}

	if !args.ShardCount.Valid {
		args.ShardCount.Set(1)
	}

	if args.ShardCount.Int64 <= 0 {
		return semerr.InvalidInput("shard count must be greater that zero")
	}

	if args.ReplicaCount <= 0 {
		return semerr.InvalidInput("replica count must be greater that zero")
	}

	return nil
}

type CreateShardArgs struct {
	Name        string
	ConfigSpec  ShardConfigSpec
	HostSpecs   []chmodels.HostSpec
	ShardGroups []string
	CopySchema  bool
}

type ShardConfigSpec struct {
	Config    chmodels.ClickHouseConfig
	Resources models.ClusterResourcesSpec
	Weight    optional.Int64
}

type UpdateDataCloudClusterArgs struct {
	ClusterID   string
	Name        optional.String
	Description optional.String
	ConfigSpec  chmodels.DataCloudConfigSpecUpdate
}

type UpdateMDBClusterArgs struct {
	ClusterID          string
	Name               optional.String
	Description        optional.String
	ConfigSpec         chmodels.MDBConfigSpecUpdate
	Labels             modelsoptional.Labels
	MaintenanceWindow  modelsoptional.MaintenanceWindow
	ServiceAccountID   optional.String
	SecurityGroupIDs   optional.Strings
	DeletionProtection optional.Bool
}

func (args *UpdateMDBClusterArgs) ValidateAndSane() error {
	if args.Name.Valid {
		if err := models.ClusterNameValidator.ValidateString(args.Name.String); err != nil {
			return err
		}
	}

	if args.ConfigSpec.EnableCloudStorage.Valid {
		if err := ValidateCloudStorageSupported(args.ConfigSpec.Version); err != nil {
			return err
		}
	}

	if args.ConfigSpec.EmbeddedKeeper {
		if err := ValidateClickHouseKeeperSupported(args.ConfigSpec.Version); err != nil {
			return err
		}
	}

	args.SecurityGroupIDs.Strings = slices.DedupStrings(args.SecurityGroupIDs.Strings)

	return args.ConfigSpec.Config.ValidateAndSane()
}

func ValidateCloudStorageSupported(version string) error {
	ok, err := chmodels.VersionGreaterOrEqual(version, 22, 3)
	if err != nil {
		return err
	}

	if !ok {
		return semerr.InvalidInput("minimum required version for clusters with cloud storage is 22.3")
	}

	return nil
}

func ValidateClickHouseKeeperSupported(version string) error {
	ok, err := chmodels.VersionGreaterOrEqual(version, 22, 3)
	if err != nil {
		return err
	}

	if !ok {
		return semerr.InvalidInput("minimum required version for clusters with ClickHouse Keeper is 22.3")
	}

	return nil
}

type RestoreMDBClusterArgs struct {
	FolderExtID        optional.String
	Name               string
	Environment        environment.SaltEnv
	ClusterSpec        chmodels.RestoreMDBClusterSpec
	DatabaseSpecs      []chmodels.DatabaseSpec
	UserSpecs          []chmodels.UserSpec
	HostSpecs          []chmodels.HostSpec
	NetworkID          string
	Description        string
	Labels             map[string]string
	ShardNames         []string
	MaintenanceWindow  clusters.MaintenanceWindow
	ServiceAccountID   string
	SecurityGroupIDs   []string
	DeletionProtection bool
}

func (args *RestoreMDBClusterArgs) ValidateAndSane() error {
	if err := models.ClusterNameValidator.ValidateString(args.Name); err != nil {
		return err
	}

	for _, db := range args.DatabaseSpecs {
		if err := db.Validate(); err != nil {
			return err
		}
	}

	for _, user := range args.UserSpecs {
		if err := user.Validate(); err != nil {
			return err
		}
	}

	for _, shardResources := range args.ClusterSpec.ShardResources {
		if err := shardResources.Validate(true); err != nil {
			return err
		}
	}

	return args.ClusterSpec.Config.ValidateAndSane()
}
