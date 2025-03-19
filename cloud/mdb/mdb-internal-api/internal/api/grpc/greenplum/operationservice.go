package greenplum

import (
	"context"

	gpv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum"
	opmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type OperationService struct {
	gpv1.UnimplementedOperationServiceServer

	Greenplum     greenplum.Greenplum
	Operations    common.Operations
	SaltEnvMapper grpcapi.SaltEnvMapper
	L             log.Logger
}

var _ gpv1.OperationServiceServer = &OperationService{}

func NewOperationService(ops common.Operations, gp greenplum.Greenplum, saltEnvsCfg logic.SaltEnvsConfig, l log.Logger) *OperationService {
	return &OperationService{
		Greenplum:  gp,
		Operations: ops,
		SaltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(gpv1.Cluster_PRODUCTION),
			int64(gpv1.Cluster_PRESTABLE),
			int64(gpv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
		L: l,
	}
}

func (os *OperationService) Get(ctx context.Context, req *gpv1.GetOperationRequest) (*operation.Operation, error) {
	op, err := os.Operations.Operation(ctx, req.GetOperationId())
	if err != nil {
		return nil, err
	}

	db, err := op.Type.Database()
	if err != nil {
		return nil, err
	}
	if db != opmodels.DatabaseGreenplum {
		return nil, xerrors.Errorf("wrong operation database %+v, must be greenplum operation", db)
	}

	return operationToGRPC(ctx, op, os.Greenplum, os.SaltEnvMapper, os.L)
}
