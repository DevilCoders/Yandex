package elasticsearch

import (
	"context"

	esv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch"
	"a.yandex-team.ru/library/go/core/log"
)

// UserService implements gRPC methods for user management.
type UserService struct {
	esv1.UnimplementedUserServiceServer

	Elasticsearch elasticsearch.ElasticSearch
	L             log.Logger
}

var _ esv1.UserServiceServer = &UserService{}

// Returns the specified Elasticsearch user.
func (us *UserService) Get(ctx context.Context, req *esv1.GetUserRequest) (*esv1.User, error) {
	user, err := us.Elasticsearch.User(ctx, req.GetClusterId(), req.GetUserName())
	if err != nil {
		return nil, err
	}

	return UserToGRPC(user), nil
}

func (us *UserService) List(ctx context.Context, req *esv1.ListUsersRequest) (*esv1.ListUsersResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	users, err := us.Elasticsearch.Users(ctx, req.GetClusterId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	return &esv1.ListUsersResponse{Users: UsersToGRPC(users)}, nil
}

func (us *UserService) Create(ctx context.Context, req *esv1.CreateUserRequest) (*operation.Operation, error) {
	op, err := us.Elasticsearch.CreateUser(ctx, req.GetClusterId(), UserSpecFromGRPC(req.GetUserSpec()))
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}

func (us *UserService) Update(ctx context.Context, req *esv1.UpdateUserRequest) (*operation.Operation, error) {
	args, err := updateUserArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}
	op, err := us.Elasticsearch.UpdateUser(ctx, args)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}

func (us *UserService) Delete(ctx context.Context, req *esv1.DeleteUserRequest) (*operation.Operation, error) {
	op, err := us.Elasticsearch.DeleteUser(ctx, req.GetClusterId(), req.GetUserName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}
