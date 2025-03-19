package elasticsearch

import (
	"context"

	esv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch"
	"a.yandex-team.ru/library/go/core/log"
)

// AuthService implements gRPC methods for authentication management.
type AuthService struct {
	esv1.UnimplementedAuthServiceServer

	es elasticsearch.ElasticSearch
	L  log.Logger
}

var _ esv1.AuthServiceServer = &AuthService{}

func NewAuthService(es elasticsearch.ElasticSearch, l log.Logger) *AuthService {
	return &AuthService{es: es, L: l}
}

func (as *AuthService) ListProviders(ctx context.Context, req *esv1.ListAuthProvidersRequest) (*esv1.ListAuthProvidersResponse, error) {
	ap, err := as.es.AuthProviders(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	c, err := AuthProvidersToGRPC(ap.Providers())
	if err != nil {
		return nil, err
	}
	return &esv1.ListAuthProvidersResponse{Providers: c.Providers}, nil
}

func (as *AuthService) GetProvider(ctx context.Context, req *esv1.GetAuthProviderRequest) (*esv1.AuthProvider, error) {
	provider, err := as.es.AuthProvider(ctx, req.GetClusterId(), req.GetName())
	if err != nil {
		return nil, err
	}

	return AuthProviderToGRPC(provider)
}

func (as *AuthService) AddProviders(ctx context.Context, req *esv1.AddAuthProvidersRequest) (*operation.Operation, error) {
	providers, err := AuthProvidersFromGRPC(req.GetProviders())
	if err != nil {
		return nil, err
	}
	op, err := as.es.AddAuthProviders(ctx, req.GetClusterId(), providers...)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, as.L)
}

func (as *AuthService) UpdateProviders(ctx context.Context, req *esv1.UpdateAuthProvidersRequest) (*operation.Operation, error) {
	providers, err := AuthProvidersFromGRPC(req.GetProviders())
	if err != nil {
		return nil, err
	}
	op, err := as.es.UpdateAuthProviders(ctx, req.GetClusterId(), providers...)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, as.L)
}

func (as *AuthService) DeleteProviders(ctx context.Context, req *esv1.DeleteAuthProvidersRequest) (*operation.Operation, error) {
	op, err := as.es.DeleteAuthProviders(ctx, req.GetClusterId(), req.GetProviderNames()...)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, as.L)
}

func (as *AuthService) UpdateProvider(ctx context.Context, req *esv1.UpdateAuthProviderRequest) (*operation.Operation, error) {
	provider, err := AuthProviderFromGRPC(req.GetProvider())
	if err != nil {
		return nil, err
	}
	op, err := as.es.UpdateAuthProvider(ctx, req.GetClusterId(), req.GetName(), provider)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, as.L)
}

func (as *AuthService) DeleteProvider(ctx context.Context, req *esv1.DeleteAuthProviderRequest) (*operation.Operation, error) {
	op, err := as.es.DeleteAuthProviders(ctx, req.GetClusterId(), req.GetName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, as.L)
}
