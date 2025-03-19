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
type DatabaseService struct {
	mongov1.UnimplementedDatabaseServiceServer

	l     log.Logger
	mongo mongodb.MongoDB
}

var _ mongov1.DatabaseServiceServer = &DatabaseService{}

func NewDatabaseService(mongo mongodb.MongoDB, l log.Logger) *DatabaseService {
	return &DatabaseService{mongo: mongo, l: l}
}

func (ds *DatabaseService) Get(ctx context.Context, req *mongov1.GetDatabaseRequest) (*mongov1.Database, error) {
	db, err := ds.mongo.Database(ctx, req.GetClusterId(), req.GetDatabaseName())
	if err != nil {
		return nil, err
	}

	return DatabaseToGRPC(db), nil
}

func (ds *DatabaseService) List(ctx context.Context, req *mongov1.ListDatabasesRequest) (*mongov1.ListDatabasesResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	dbs, err := ds.mongo.Databases(ctx, req.GetClusterId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	return &mongov1.ListDatabasesResponse{Databases: DatabasesToGRPC(dbs)}, nil
}

func (ds *DatabaseService) Create(ctx context.Context, req *mongov1.CreateDatabaseRequest) (*operation.Operation, error) {
	op, err := ds.mongo.CreateDatabase(ctx, req.GetClusterId(), DatabaseSpecFromGRPC(req.GetDatabaseSpec()))
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, ds.l)
}
