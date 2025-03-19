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

// DatabaseService implements DB-specific gRPC methods
type DatabaseService struct {
	chv1.UnimplementedDatabaseServiceServer

	l  log.Logger
	ch clickhouse.ClickHouse
}

var _ chv1.DatabaseServiceServer = &DatabaseService{}

func NewDatabaseService(ch clickhouse.ClickHouse, l log.Logger) *DatabaseService {
	return &DatabaseService{ch: ch, l: l}
}

func (ds *DatabaseService) Get(ctx context.Context, req *chv1.GetDatabaseRequest) (*chv1.Database, error) {
	db, err := ds.ch.Database(ctx, req.GetClusterId(), req.GetDatabaseName())
	if err != nil {
		return nil, err
	}

	return DatabaseToGRPC(db), nil
}

func (ds *DatabaseService) List(ctx context.Context, req *chv1.ListDatabasesRequest) (*chv1.ListDatabasesResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return &chv1.ListDatabasesResponse{}, err
	}

	dbs, databasePageToken, err := ds.ch.Databases(ctx, req.GetClusterId(), req.GetPageSize(), pageToken.Offset)
	if err != nil {
		return nil, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(databasePageToken, false)
	if err != nil {
		return &chv1.ListDatabasesResponse{}, err
	}

	return &chv1.ListDatabasesResponse{
		Databases:     DatabasesToGRPC(dbs),
		NextPageToken: nextPageToken,
	}, nil
}

func (ds *DatabaseService) Create(ctx context.Context, req *chv1.CreateDatabaseRequest) (*operation.Operation, error) {
	op, err := ds.ch.CreateDatabase(ctx, req.GetClusterId(), DatabaseSpecFromGRPC(req.GetDatabaseSpec()))
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, ds.l)
}

func (ds *DatabaseService) Delete(ctx context.Context, req *chv1.DeleteDatabaseRequest) (*operation.Operation, error) {
	op, err := ds.ch.DeleteDatabase(ctx, req.GetClusterId(), req.GetDatabaseName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, ds.l)
}
