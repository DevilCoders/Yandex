package sqlserver

import (
	"context"

	ssv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/ssmodels"
	"a.yandex-team.ru/library/go/core/log"
)

// UserService implements gRPC methods for user management.
type UserService struct {
	ssv1.UnimplementedUserServiceServer

	SQLServer sqlserver.SQLServer
	L         log.Logger
}

func (us *UserService) GrantPermission(ctx context.Context, request *ssv1.GrantUserPermissionRequest) (*operation.Operation, error) {
	op, err := us.SQLServer.GrantPermission(ctx, request.ClusterId, request.UserName, ssmodels.Permission{
		DatabaseName: request.Permission.GetDatabaseName(),
		Roles:        databaseRolesFromGRPC(request.Permission.GetRoles()),
	})
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}

func (us *UserService) RevokePermission(ctx context.Context, request *ssv1.RevokeUserPermissionRequest) (*operation.Operation, error) {
	op, err := us.SQLServer.RevokePermission(ctx, request.ClusterId, request.UserName, ssmodels.Permission{
		DatabaseName: request.Permission.GetDatabaseName(),
		Roles:        databaseRolesFromGRPC(request.Permission.GetRoles()),
	})
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}

var _ ssv1.UserServiceServer = &UserService{}

// Returns the specified SQLServer user.
func (us *UserService) Get(ctx context.Context, req *ssv1.GetUserRequest) (*ssv1.User, error) {
	user, err := us.SQLServer.User(ctx, req.GetClusterId(), req.GetUserName())
	if err != nil {
		return nil, err
	}

	return userToGRPC(user), nil
}

func (us *UserService) List(ctx context.Context, req *ssv1.ListUsersRequest) (*ssv1.ListUsersResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	users, err := us.SQLServer.Users(ctx, req.GetClusterId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}
	return &ssv1.ListUsersResponse{Users: usersToGRPC(users)}, nil
}

func (us *UserService) Update(ctx context.Context, req *ssv1.UpdateUserRequest) (*operation.Operation, error) {
	usrArgs, err := userUpdateFromGRPC(req)
	if err != nil {
		return nil, err
	}
	op, err := us.SQLServer.UpdateUser(ctx, req.GetClusterId(), usrArgs)
	if err != nil {
		return nil, err
	}
	return grpcapi.OperationToGRPC(ctx, op, us.L)
}

func (us *UserService) Create(ctx context.Context, req *ssv1.CreateUserRequest) (*operation.Operation, error) {
	userSpec := userSpecFromGRPC(req.GetUserSpec())
	op, err := us.SQLServer.CreateUser(ctx, req.GetClusterId(), userSpec)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}

func (us *UserService) Delete(ctx context.Context, req *ssv1.DeleteUserRequest) (*operation.Operation, error) {
	op, err := us.SQLServer.DeleteUser(ctx, req.GetClusterId(), req.GetUserName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, us.L)
}
