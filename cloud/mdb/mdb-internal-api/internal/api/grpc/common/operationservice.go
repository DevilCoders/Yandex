package common

import (
	"context"

	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	mdbv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	opmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log"
)

type OperationService struct {
	mdbv1.UnimplementedOperationServiceServer

	Kafka      kfv1.OperationServiceServer
	Operations common.Operations
	L          log.Logger
}

var _ mdbv1.OperationServiceServer = &OperationService{}

func (os *OperationService) Get(ctx context.Context, req *mdbv1.GetOperationRequest) (*operation.Operation, error) {
	op, err := os.Operations.Operation(ctx, req.GetOperationId())
	if err != nil {
		return nil, err
	}

	db, err := op.Type.Database()
	if err != nil {
		return nil, err
	}
	if db == opmodels.DatabaseKafka {
		kafkaReq := &kfv1.GetOperationRequest{OperationId: req.GetOperationId()}
		return os.Kafka.Get(ctx, kafkaReq)
	}

	return grpcapi.OperationToGRPC(ctx, op, os.L)
}
