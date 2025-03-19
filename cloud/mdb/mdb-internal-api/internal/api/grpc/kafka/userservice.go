package kafka

import (
	"context"

	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/library/go/core/log"
)

// UserService implements gRPC methods for user management.
type UserService struct {
	kfv1.UnimplementedUserServiceServer

	Kafka kafka.Kafka
	L     log.Logger
}

var _ kfv1.UserServiceServer = &UserService{}

// Returns the specified Kafka user.
func (us *UserService) Get(ctx context.Context, req *kfv1.GetUserRequest) (*kfv1.User, error) {
	user, err := us.Kafka.User(ctx, req.GetClusterId(), req.GetUserName())
	if err != nil {
		return nil, err
	}

	return UserToGRPC(user), nil
}

func (us *UserService) List(ctx context.Context, req *kfv1.ListUsersRequest) (*kfv1.ListUsersResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	users, userPageToken, err := us.Kafka.Users(ctx, req.GetClusterId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(userPageToken, false)
	if err != nil {
		return nil, err
	}

	return &kfv1.ListUsersResponse{
		Users:         UsersToGRPC(users),
		NextPageToken: nextPageToken,
	}, nil
}

func (us *UserService) Create(ctx context.Context, req *kfv1.CreateUserRequest) (*operation.Operation, error) {
	spec := req.GetUserSpec()
	op, err := us.Kafka.CreateUser(ctx, req.GetClusterId(), UserSpecFromGRPC(spec))
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}

func (us *UserService) Update(ctx context.Context, req *kfv1.UpdateUserRequest) (*operation.Operation, error) {
	args, err := updateUserArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}
	op, err := us.Kafka.UpdateUser(ctx, args)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}

func (us *UserService) GrantPermission(ctx context.Context, req *kfv1.GrantUserPermissionRequest) (*operation.Operation, error) {
	op, err := us.Kafka.GrantUserPermission(ctx, req.GetClusterId(), req.GetUserName(), UserPermissionFromGRPC(req.GetPermission()))
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}

func (us *UserService) RevokePermission(ctx context.Context, req *kfv1.RevokeUserPermissionRequest) (*operation.Operation, error) {
	op, err := us.Kafka.RevokeUserPermission(ctx, req.GetClusterId(), req.GetUserName(), UserPermissionFromGRPC(req.GetPermission()))
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}

func (us *UserService) Delete(ctx context.Context, req *kfv1.DeleteUserRequest) (*operation.Operation, error) {
	op, err := us.Kafka.DeleteUser(ctx, req.GetClusterId(), req.GetUserName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}
