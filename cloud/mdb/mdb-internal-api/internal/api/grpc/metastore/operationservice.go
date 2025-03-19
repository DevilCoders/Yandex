package metastore

import (
	"context"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes"

	msv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/metastore/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore/models"
	opmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type OperationService struct {
	msv1.UnimplementedOperationServiceServer

	Metastore  metastore.Metastore
	Operations common.Operations
	L          log.Logger
}

var _ msv1.OperationServiceServer = &OperationService{}

func NewOperationService(ops common.Operations, ms metastore.Metastore, l log.Logger) *OperationService {
	return &OperationService{
		Metastore:  ms,
		Operations: ops,
		L:          l,
	}
}

func (os *OperationService) Get(ctx context.Context, req *msv1.GetOperationRequest) (*operation.Operation, error) {
	op, err := os.Operations.Operation(ctx, req.GetOperationId())
	if err != nil {
		return nil, err
	}

	db, err := op.Type.Database()
	if err != nil {
		return nil, err
	}
	if db != opmodels.DatabaseMetastore {
		return nil, xerrors.Errorf("wrong operation database %+v, must be Metastore operation", db)
	}

	return operationToGRPC(ctx, op, os.Metastore, os.L)
}

func operationToGRPC(ctx context.Context, op opmodels.Operation, ms metastore.Metastore, l log.Logger) (*operation.Operation, error) {
	opGrpc, err := grpcapi.OperationToGRPC(ctx, op, l)
	if err != nil {
		return nil, err
	}
	if op.Status != opmodels.StatusDone {
		return opGrpc, nil
	}

	switch op.MetaData.(type) {
	// Must be at first position before interfaces
	case models.MetadataDeleteCluster:
		return withEmptyResult(opGrpc)
	// TODO: remove type list
	case models.MetadataCreateCluster:
		cluster, err := ms.MDBCluster(ctx, op.ClusterID)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, ClusterToGRPC(cluster))
	}
	return opGrpc, nil
}

func withEmptyResult(operationGrpc *operation.Operation) (*operation.Operation, error) {
	operationGrpc.Result = &operation.Operation_Response{}
	return operationGrpc, nil
}

func withResult(operationGrpc *operation.Operation, message proto.Message) (*operation.Operation, error) {
	response, err := ptypes.MarshalAny(message)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal operation %+v response: %w", operationGrpc, err)
	}
	operationGrpc.Result = &operation.Operation_Response{Response: response}
	return operationGrpc, nil
}
