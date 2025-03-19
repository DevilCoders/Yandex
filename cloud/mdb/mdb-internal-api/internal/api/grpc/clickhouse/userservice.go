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

// UserService implements gRPC methods for user management.
type UserService struct {
	chv1.UnimplementedUserServiceServer

	l  log.Logger
	ch clickhouse.ClickHouse
}

var _ chv1.UserServiceServer = &UserService{}

func NewUserService(ch clickhouse.ClickHouse, l log.Logger) *UserService {
	return &UserService{ch: ch, l: l}
}

// Returns the specified ClickHouse user.
func (us *UserService) Get(ctx context.Context, req *chv1.GetUserRequest) (*chv1.User, error) {
	user, err := us.ch.User(ctx, req.GetClusterId(), req.GetUserName())
	if err != nil {
		return nil, err
	}

	return UserToGRPC(user), nil
}

func (us *UserService) List(ctx context.Context, req *chv1.ListUsersRequest) (*chv1.ListUsersResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	users, userPageToken, err := us.ch.Users(ctx, req.GetClusterId(), req.GetPageSize(), pageToken.Offset)
	if err != nil {
		return nil, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(userPageToken, false)
	if err != nil {
		return nil, err
	}

	return &chv1.ListUsersResponse{
		Users:         UsersToGRPC(users),
		NextPageToken: nextPageToken,
	}, nil
}

func (us *UserService) Create(ctx context.Context, req *chv1.CreateUserRequest) (*operation.Operation, error) {
	spec, err := UserSpecFromGRPC(req.GetUserSpec())
	if err != nil {
		return nil, err
	}
	op, err := us.ch.CreateUser(ctx, req.GetClusterId(), spec)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.l)
}

func (us *UserService) Update(ctx context.Context, req *chv1.UpdateUserRequest) (*operation.Operation, error) {
	spec, err := UserUpdateFromGRPC(req)
	if err != nil {
		return nil, err
	}

	op, err := us.ch.UpdateUser(ctx, req.ClusterId, req.UserName, spec)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.l)
}

func (us *UserService) Delete(ctx context.Context, req *chv1.DeleteUserRequest) (*operation.Operation, error) {
	op, err := us.ch.DeleteUser(ctx, req.GetClusterId(), req.GetUserName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.l)
}

func (us *UserService) GrantPermission(ctx context.Context, req *chv1.GrantUserPermissionRequest) (*operation.Operation, error) {
	perm, err := UserPermissionFromGRPC(req.Permission)
	if err != nil {
		return nil, err
	}
	op, err := us.ch.GrantPermission(ctx, req.ClusterId, req.UserName, perm)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.l)
}

func (us *UserService) RevokePermission(ctx context.Context, req *chv1.RevokeUserPermissionRequest) (*operation.Operation, error) {
	op, err := us.ch.RevokePermission(ctx, req.ClusterId, req.UserName, req.DatabaseName)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.l)
}
