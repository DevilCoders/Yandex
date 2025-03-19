package elasticsearch

import (
	"context"

	esv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch"
	"a.yandex-team.ru/library/go/core/log"
)

// ExtensionService implements gRPC methods for extensions management.
type ExtensionService struct {
	esv1.UnimplementedExtensionServiceServer

	es elasticsearch.ElasticSearch
	L  log.Logger
}

var _ esv1.ExtensionServiceServer = &ExtensionService{}

func NewExtensionService(es elasticsearch.ElasticSearch, l log.Logger) *ExtensionService {
	return &ExtensionService{es: es, L: l}
}

func (es *ExtensionService) Get(ctx context.Context, req *esv1.GetExtensionRequest) (*esv1.Extension, error) {
	extension, err := es.es.Extension(ctx, req.GetClusterId(), req.GetExtensionId())
	if err != nil {
		return nil, err
	}

	return ExtensionToGRPC(req.GetClusterId(), extension), nil
}

func (es *ExtensionService) List(ctx context.Context, req *esv1.ListExtensionsRequest) (*esv1.ListExtensionsResponse, error) {
	extensions, err := es.es.Extensions(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return &esv1.ListExtensionsResponse{Extensions: ExtensionsToGRPC(req.GetClusterId(), extensions)}, nil
}

func (es *ExtensionService) Create(ctx context.Context, req *esv1.CreateExtensionRequest) (*operation.Operation, error) {
	op, err := es.es.CreateExtension(ctx, req.GetClusterId(), req.GetName(), req.GetUri(), req.GetDisabled())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, es.L)
}

func (es *ExtensionService) Delete(ctx context.Context, req *esv1.DeleteExtensionRequest) (*operation.Operation, error) {
	op, err := es.es.DeleteExtension(ctx, req.GetClusterId(), req.GetExtensionId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, es.L)
}

func (es *ExtensionService) Update(ctx context.Context, req *esv1.UpdateExtensionRequest) (*operation.Operation, error) {
	op, err := es.es.UpdateExtension(ctx, req.GetClusterId(), req.GetExtensionId(), req.GetActive())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, es.L)
}
