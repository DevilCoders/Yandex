package clickhouse

import (
	"context"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/core/log"
)

// MLModelService implements gRPC methods for ML model management.
type MLModelService struct {
	chv1.UnimplementedMlModelServiceServer

	l  log.Logger
	ch clickhouse.ClickHouse
}

var _ chv1.MlModelServiceServer = &MLModelService{}

func NewMLModelService(ch clickhouse.ClickHouse, l log.Logger) *MLModelService {
	return &MLModelService{ch: ch, l: l}
}

// Returns the specified ClickHouse ML model.
func (mls *MLModelService) Get(ctx context.Context, req *chv1.GetMlModelRequest) (*chv1.MlModel, error) {
	mlmodel, err := mls.ch.MLModel(ctx, req.GetClusterId(), req.GetMlModelName())
	if err != nil {
		return nil, err
	}

	return MLModelToGRPC(mlmodel)
}

func (mls *MLModelService) List(ctx context.Context, req *chv1.ListMlModelsRequest) (*chv1.ListMlModelsResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	mlModels, mlModelPageToken, err := mls.ch.MLModels(ctx, req.GetClusterId(), req.GetPageSize(), pageToken.Offset)
	if err != nil {
		return nil, err
	}

	grpcMlModels, err := MLModelsToGRPC(mlModels)
	if err != nil {
		return nil, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(mlModelPageToken, false)
	if err != nil {
		return nil, err
	}

	return &chv1.ListMlModelsResponse{
		MlModels:      grpcMlModels,
		NextPageToken: nextPageToken,
	}, nil
}

func (mls *MLModelService) Create(ctx context.Context, req *chv1.CreateMlModelRequest) (*operation.Operation, error) {
	mlType, err := MlModelTypeFromGRPC(req.GetType())
	if err != nil {
		return nil, err
	}

	op, err := mls.ch.CreateMLModel(ctx, req.GetClusterId(), req.GetMlModelName(), mlType, req.GetUri())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, mls.l)
}

func (mls *MLModelService) Update(ctx context.Context, req *chv1.UpdateMlModelRequest) (*operation.Operation, error) {
	op, err := mls.ch.UpdateMLModel(ctx, req.GetClusterId(), req.GetMlModelName(), req.GetUri())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, mls.l)
}

func (mls *MLModelService) Delete(ctx context.Context, req *chv1.DeleteMlModelRequest) (*operation.Operation, error) {
	op, err := mls.ch.DeleteMLModel(ctx, req.GetClusterId(), req.GetMlModelName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, mls.l)
}
