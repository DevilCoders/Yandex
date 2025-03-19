package sqlserver

import (
	"context"

	ssv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	opmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type OperationService struct {
	ssv1.UnimplementedOperationServiceServer

	SQLServer     sqlserver.SQLServer
	Operations    common.Operations
	SaltEnvMapper grpcapi.SaltEnvMapper
	L             log.Logger
}

var _ ssv1.OperationServiceServer = &OperationService{}

func NewOperationService(ops common.Operations, ss sqlserver.SQLServer, saltEnvsCfg logic.SaltEnvsConfig, l log.Logger) *OperationService {
	return &OperationService{
		SQLServer:  ss,
		Operations: ops,
		SaltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(ssv1.Cluster_PRODUCTION),
			int64(ssv1.Cluster_PRESTABLE),
			int64(ssv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
		L: l,
	}
}

func (os *OperationService) Get(ctx context.Context, req *ssv1.GetOperationRequest) (*operation.Operation, error) {
	op, err := os.Operations.Operation(ctx, req.GetOperationId())
	if err != nil {
		return nil, err
	}

	db, err := op.Type.Database()
	if err != nil {
		return nil, err
	}
	if db != opmodels.DatabaseSqlserver {
		return nil, xerrors.Errorf("wrong operation database %+v, must be sqlserver operation", db)
	}

	return operationToGRPC(ctx, op, os.SQLServer, os.SaltEnvMapper, os.L)
}
