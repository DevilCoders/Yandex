package mongodb

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

//go:generate ../../../../scripts/mockgen.sh MongoDB

type MongoDB interface {
	Cluster(ctx context.Context, cid string) (clusters.ClusterExtended, error)
	Clusters(ctx context.Context, folderExtID string, limit, offset int64) ([]clusters.ClusterExtended, error)
	CreateCluster(ctx context.Context, args CreateClusterArgs) (operations.Operation, error)
	DeleteCluster(ctx context.Context, cid string) (operations.Operation, error)

	Database(ctx context.Context, cid string, dbname string) (mongomodels.Database, error)
	Databases(ctx context.Context, cid string, limit, offset int64) ([]mongomodels.Database, error)
	CreateDatabase(ctx context.Context, cid string, spec mongomodels.DatabaseSpec) (operations.Operation, error)

	User(ctx context.Context, cid, username string) (mongomodels.User, error)
	Users(ctx context.Context, cid string, limit, offset int64) ([]mongomodels.User, error)
	CreateUser(ctx context.Context, cid string, spec mongomodels.UserSpec) (operations.Operation, error)
	DeleteUser(ctx context.Context, cid string, username string) (operations.Operation, error)

	ResetupHosts(ctx context.Context, cid string, hostNames []string) (operations.Operation, error)
	RestartHosts(ctx context.Context, cid string, hostNames []string) (operations.Operation, error)
	StepdownHosts(ctx context.Context, cid string, hostNames []string) (operations.Operation, error)

	DeleteBackup(ctx context.Context, backupID string) (operations.Operation, error)
}

type CreateClusterArgs struct {
	FolderExtID        string
	Name               string
	Description        string
	Labels             map[string]string
	Environment        environment.SaltEnv
	NetworkID          string
	SecurityGroupIDs   []string
	UserSpecs          []mongomodels.UserSpec
	DatabaseSpecs      []mongomodels.DatabaseSpec
	DeletionProtection bool
	// TODO: add ClusterConfigSpec and HostSpecs
}
