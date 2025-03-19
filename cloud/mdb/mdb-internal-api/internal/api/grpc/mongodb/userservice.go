package mongodb

import (
	"context"

	mongov1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mongodb/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb"
	"a.yandex-team.ru/library/go/core/log"
)

// DatabaseService implements DB-specific gRPC methods
type UserService struct {
	mongov1.UnimplementedUserServiceServer

	MongoDB mongodb.MongoDB
	L       log.Logger
}

var _ mongov1.UserServiceServer = &UserService{}

func (us *UserService) Get(ctx context.Context, req *mongov1.GetUserRequest) (*mongov1.User, error) {
	user, err := us.MongoDB.User(ctx, req.GetClusterId(), req.GetUserName())
	if err != nil {
		return nil, err
	}

	return UserToGRPC(user), nil
}

func (us *UserService) List(ctx context.Context, req *mongov1.ListUsersRequest) (*mongov1.ListUsersResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	users, err := us.MongoDB.Users(ctx, req.GetClusterId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	return &mongov1.ListUsersResponse{Users: UsersToGRPC(users)}, nil
}

func (us *UserService) Create(ctx context.Context, req *mongov1.CreateUserRequest) (*operation.Operation, error) {
	op, err := us.MongoDB.CreateUser(ctx, req.GetClusterId(), UserSpecFromGRPC(req.GetUserSpec()))
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}

func (us *UserService) Delete(ctx context.Context, req *mongov1.DeleteUserRequest) (*operation.Operation, error) {
	op, err := us.MongoDB.DeleteUser(ctx, req.GetClusterId(), req.GetUserName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}
