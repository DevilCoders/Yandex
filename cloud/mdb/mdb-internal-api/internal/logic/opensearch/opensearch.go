package opensearch

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/osmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	modelsoptional "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
)

//go:generate ../../../../scripts/mockgen.sh OpenSearch

type OpenSearch interface {
	Cluster(ctx context.Context, cid string) (osmodels.Cluster, error)
	Clusters(ctx context.Context, folderExtID string, limit, offset int64) ([]osmodels.Cluster, error)
	CreateCluster(ctx context.Context, args CreateClusterArgs) (operations.Operation, error)
	ModifyCluster(ctx context.Context, args ModifyClusterArgs) (operations.Operation, error)
	DeleteCluster(ctx context.Context, cid string) (operations.Operation, error)
	ListHosts(ctx context.Context, cid string, limit, offset int64) ([]hosts.HostExtended, pagination.OffsetPageToken, error)
	StartCluster(ctx context.Context, cid string) (operations.Operation, error)
	StopCluster(ctx context.Context, cid string) (operations.Operation, error)
	RescheduleMaintenance(ctx context.Context, cid string, rescheduleType clusters.RescheduleType, delayedUntil optional.Time) (operations.Operation, error)
	RestoreCluster(ctx context.Context, args RestoreClusterArgs) (operations.Operation, error)

	AddHosts(ctx context.Context, cid string, args []osmodels.Host) (operations.Operation, error)
	DeleteHosts(ctx context.Context, cid string, fqdns []string) (operations.Operation, error)

	EstimateCreateCluster(ctx context.Context, args CreateClusterArgs) (console.BillingEstimate, error)
	SupportedVersions(ctx context.Context) osmodels.SupportedVersions

	AuthProvider(ctx context.Context, cid string, name string) (*osmodels.AuthProvider, error)
	AuthProviders(ctx context.Context, cid string) (*osmodels.AuthProviders, error)
	AddAuthProviders(ctx context.Context, cid string, providers ...*osmodels.AuthProvider) (operations.Operation, error)
	UpdateAuthProviders(ctx context.Context, cid string, providers ...*osmodels.AuthProvider) (operations.Operation, error)
	DeleteAuthProviders(ctx context.Context, cid string, names ...string) (operations.Operation, error)
	UpdateAuthProvider(ctx context.Context, cid string, name string, provider *osmodels.AuthProvider) (operations.Operation, error)

	Backup(ctx context.Context, backupID string) (backups.Backup, error)
	FolderBackups(ctx context.Context, fid string, pageToken backups.BackupsPageToken, pageSize int64) ([]backups.Backup, backups.BackupsPageToken, error)
	ClusterBackups(ctx context.Context, cid string, pageToken backups.BackupsPageToken, pageSize int64) ([]backups.Backup, backups.BackupsPageToken, error)
	BackupCluster(ctx context.Context, cid string) (operations.Operation, error)

	Extension(ctx context.Context, cid string, extensionID string) (osmodels.Extension, error)
	Extensions(ctx context.Context, cid string) ([]osmodels.Extension, error)
	CreateExtension(ctx context.Context, cid, name, uri string, disabled bool) (operations.Operation, error)
	DeleteExtension(ctx context.Context, cid, extensionID string) (operations.Operation, error)
	UpdateExtension(ctx context.Context, cid, extensionID string, active bool) (operations.Operation, error)
}

type CreateClusterArgs struct {
	FolderExtID        string
	Name               string
	Description        string
	Labels             map[string]string
	Environment        environment.SaltEnv
	NetworkID          string
	SecurityGroupIDs   []string
	ServiceAccountID   string
	HostSpec           []osmodels.Host
	ConfigSpec         osmodels.ClusterConfigSpec
	ExtensionSpecs     []osmodels.ExtensionSpec
	DeletionProtection bool
	MaintenanceWindow  clusters.MaintenanceWindow
	RestoreFrom        string
}

type RestoreClusterArgs struct {
	CreateClusterArgs
}

type ModifyClusterArgs struct {
	ClusterID          string
	Name               optional.String
	Description        optional.String
	SecurityGroupIDs   optional.Strings
	ServiceAccountID   optional.String
	Labels             modelsoptional.Labels
	ConfigSpec         osmodels.ConfigSpecUpdate
	DeletionProtection optional.Bool
	MaintenanceWindow  modelsoptional.MaintenanceWindow
}

type UpdateUserArgs struct {
	ClusterID string
	Name      string
	Password  secret.String
}
