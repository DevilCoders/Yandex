package opensearch

import (
	"context"

	protov1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/opensearch/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch"
	opmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type OperationService struct {
	protov1.UnimplementedOperationServiceServer

	Operations    common.Operations
	OpenSearch    opensearch.OpenSearch
	SaltEnvMapper grpcapi.SaltEnvMapper
	L             log.Logger
}

var _ protov1.OperationServiceServer = &OperationService{}

func NewOperationService(ops common.Operations, es opensearch.OpenSearch, saltEnvsCfg logic.SaltEnvsConfig, l log.Logger) *OperationService {
	return &OperationService{
		OpenSearch: es,
		Operations: ops,
		SaltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(protov1.Cluster_PRODUCTION),
			int64(protov1.Cluster_PRESTABLE),
			int64(protov1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
		L: l,
	}
}

func (os *OperationService) Get(ctx context.Context, req *protov1.GetOperationRequest) (*operation.Operation, error) {
	op, err := os.Operations.Operation(ctx, req.GetOperationId())
	if err != nil {
		return nil, err
	}

	db, err := op.Type.Database()
	if err != nil {
		return nil, err
	}

	if db != opmodels.DatabaseOpenSearch {
		return nil, xerrors.Errorf("wrong operation database %+v, must be opensearch operation", db)
	}

	return operationToGRPC(ctx, op, os.OpenSearch, os.SaltEnvMapper, os.L)
}
