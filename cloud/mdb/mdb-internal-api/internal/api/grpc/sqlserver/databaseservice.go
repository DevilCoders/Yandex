package sqlserver

import (
	"context"

	ssv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	"a.yandex-team.ru/library/go/core/log"
)

// DatabaseService implements database-specific gRPC methods
type DatabaseService struct {
	ssv1.UnimplementedDatabaseServiceServer

	SQLServer sqlserver.SQLServer
	L         log.Logger
}

var _ ssv1.DatabaseServiceServer = &DatabaseService{}

func (ts *DatabaseService) Get(ctx context.Context, req *ssv1.GetDatabaseRequest) (*ssv1.Database, error) {
	db, err := ts.SQLServer.Database(ctx, req.GetClusterId(), req.GetDatabaseName())
	if err != nil {
		return nil, err
	}

	return databaseToGRPC(db), nil
}

func (ts *DatabaseService) List(ctx context.Context, req *ssv1.ListDatabasesRequest) (*ssv1.ListDatabasesResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	dbs, err := ts.SQLServer.Databases(ctx, req.GetClusterId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	return &ssv1.ListDatabasesResponse{Databases: databasesToGRPC(dbs)}, nil
}

func (ts *DatabaseService) Create(ctx context.Context, req *ssv1.CreateDatabaseRequest) (*operation.Operation, error) {
	op, err := ts.SQLServer.CreateDatabase(ctx, req.GetClusterId(), databaseSpecFromGRPC(req.GetDatabaseSpec()))
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, ts.L)
}

func (ts *DatabaseService) Delete(ctx context.Context, req *ssv1.DeleteDatabaseRequest) (*operation.Operation, error) {
	op, err := ts.SQLServer.DeleteDatabase(ctx, req.GetClusterId(), req.GetDatabaseName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, ts.L)
}

func (ts *DatabaseService) Restore(ctx context.Context, req *ssv1.RestoreDatabaseRequest) (*operation.Operation, error) {
	op, err := ts.SQLServer.RestoreDatabase(ctx, req.GetClusterId(), restoreDatabaseSpecFromGRPC(req))
	if err != nil {
		return nil, err
	}
	return grpcapi.OperationToGRPC(ctx, op, ts.L)
}

func (ts *DatabaseService) ImportBackup(ctx context.Context, req *ssv1.ImportDatabaseBackupRequest) (*operation.Operation, error) {
	op, err := ts.SQLServer.ImportDatabaseBackup(ctx, req.GetClusterId(), importDatabaseBackupSpecFromGRPC(req))
	if err != nil {
		return nil, err
	}
	return grpcapi.OperationToGRPC(ctx, op, ts.L)
}

func (ts *DatabaseService) ExportBackup(ctx context.Context, req *ssv1.ExportDatabaseBackupRequest) (*operation.Operation, error) {
	op, err := ts.SQLServer.ExportDatabaseBackup(ctx, req.GetClusterId(), exportDatabaseBackupSpecFromGRPC(req))
	if err != nil {
		return nil, err
	}
	return grpcapi.OperationToGRPC(ctx, op, ts.L)
}
