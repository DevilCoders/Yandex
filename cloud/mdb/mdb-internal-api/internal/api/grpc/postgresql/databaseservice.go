package postgresql

import (
	"context"

	pgv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/postgresql/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql"
	"a.yandex-team.ru/library/go/core/log"
)

// DatabaseService implements DB-specific gRPC methods
type DatabaseService struct {
	pgv1.UnimplementedDatabaseServiceServer

	l  log.Logger
	pg postgresql.PostgreSQL
}

func NewDatabaseService(pg postgresql.PostgreSQL, l log.Logger) *DatabaseService {
	return &DatabaseService{pg: pg, l: l}
}

var _ pgv1.DatabaseServiceServer = &DatabaseService{}

func (ds *DatabaseService) Get(ctx context.Context, req *pgv1.GetDatabaseRequest) (*pgv1.Database, error) {
	db, err := ds.pg.Database(ctx, req.GetClusterId(), req.GetDatabaseName())
	if err != nil {
		return nil, err
	}

	return &pgv1.Database{Name: db.Name, ClusterId: db.ClusterID, Owner: db.Owner, LcCollate: db.LCCollate, LcCtype: db.LCCtype}, nil
}

func (ds *DatabaseService) List(ctx context.Context, req *pgv1.ListDatabasesRequest) (*pgv1.ListDatabasesResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	dbs, err := ds.pg.Databases(ctx, req.GetClusterId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	var respDBs []*pgv1.Database
	for _, db := range dbs {
		respDBs = append(respDBs, &pgv1.Database{Name: db.Name, ClusterId: db.ClusterID, Owner: db.Owner, LcCollate: db.LCCollate, LcCtype: db.LCCtype})
	}

	return &pgv1.ListDatabasesResponse{Databases: respDBs}, nil
}
