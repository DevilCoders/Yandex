package airflow

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/airflow/afmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

//go:generate ../../../../scripts/mockgen.sh Airflow

type Airflow interface {
	MDBCluster(ctx context.Context, cid string) (afmodels.MDBCluster, error)
	CreateMDBCluster(ctx context.Context, args CreateMDBClusterArgs) (operations.Operation, error)
	DeleteCluster(ctx context.Context, cid string) (operations.Operation, error)
}

type CreateMDBClusterArgs struct {
	FolderExtID string
	Name        string
	Description string
	Labels      map[string]string
	Environment environment.SaltEnv
	ConfigSpec  afmodels.ClusterConfigSpec
	Network     afmodels.NetworkConfig
	CodeSync    afmodels.CodeSyncConfig

	DeletionProtection bool
}

func (args CreateMDBClusterArgs) Validate() error {
	if args.FolderExtID == "" {
		return semerr.InvalidInput("folder id must be specified") // TODO use resource model when go to DataCloud
	}
	if args.Name == "" {
		return semerr.InvalidInput("cluster name must be specified")
	}
	// TODO: Validate more

	return models.ClusterNameValidator.ValidateString(args.Name)
}
