package elasticsearch

import (
	"context"

	esv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch"
	opmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type OperationService struct {
	esv1.UnimplementedOperationServiceServer

	Operations    common.Operations
	Elasticsearch elasticsearch.ElasticSearch
	SaltEnvMapper grpcapi.SaltEnvMapper
	L             log.Logger
}

var _ esv1.OperationServiceServer = &OperationService{}

func NewOperationService(ops common.Operations, es elasticsearch.ElasticSearch, saltEnvsCfg logic.SaltEnvsConfig, l log.Logger) *OperationService {
	return &OperationService{
		Elasticsearch: es,
		Operations:    ops,
		SaltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(esv1.Cluster_PRODUCTION),
			int64(esv1.Cluster_PRESTABLE),
			int64(esv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
		L: l,
	}
}

func (os *OperationService) Get(ctx context.Context, req *esv1.GetOperationRequest) (*operation.Operation, error) {
	op, err := os.Operations.Operation(ctx, req.GetOperationId())
	if err != nil {
		return nil, err
	}

	db, err := op.Type.Database()
	if err != nil {
		return nil, err
	}

	if db != opmodels.DatabaseElasticsearch {
		return nil, xerrors.Errorf("wrong operation database %+v, must be elasticsearch operation", db)
	}

	return operationToGRPC(ctx, op, os.Elasticsearch, os.SaltEnvMapper, os.L)
}
