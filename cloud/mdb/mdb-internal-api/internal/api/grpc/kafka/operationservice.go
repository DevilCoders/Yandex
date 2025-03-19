package kafka

import (
	"context"

	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	opmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type OperationService struct {
	kfv1.UnimplementedOperationServiceServer

	Kafka         kafka.Kafka
	Operations    common.Operations
	SaltEnvMapper grpcapi.SaltEnvMapper
	L             log.Logger
}

var _ kfv1.OperationServiceServer = &OperationService{}

func NewOperationService(ops common.Operations, kf kafka.Kafka, saltEnvsCfg logic.SaltEnvsConfig, l log.Logger) *OperationService {
	return &OperationService{
		Kafka:      kf,
		Operations: ops,
		SaltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(kfv1.Cluster_PRODUCTION),
			int64(kfv1.Cluster_PRESTABLE),
			int64(kfv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
		L: l,
	}
}

func (os *OperationService) Get(ctx context.Context, req *kfv1.GetOperationRequest) (*operation.Operation, error) {
	op, err := os.Operations.Operation(ctx, req.GetOperationId())
	if err != nil {
		return nil, err
	}

	db, err := op.Type.Database()
	if err != nil {
		return nil, err
	}
	if db != opmodels.DatabaseKafka {
		return nil, xerrors.Errorf("wrong operation database %+v, must be kafka operation", db)
	}

	return operationToGRPC(ctx, op, os.Kafka, os.SaltEnvMapper, os.L)
}
