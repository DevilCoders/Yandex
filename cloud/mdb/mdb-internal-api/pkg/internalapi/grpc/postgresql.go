package grpc

import (
	"context"

	pgv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/postgresql/v1"
	grpcsrv "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/pkg/internalapi"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type pgClient struct {
	c *Client

	cluster  pgv1.ClusterServiceClient
	database pgv1.DatabaseServiceClient
}

var _ internalapi.PostgreSQLClient = &pgClient{}

func (pgc *pgClient) IsReady(ctx context.Context) error {
	return pgc.c.IsReady(ctx)
}

func postgresqlLogsServiceTypeToGRPC(st internalapi.PostgresqlLogsServiceType) pgv1.ListClusterLogsRequest_ServiceType {
	switch st {
	case internalapi.PostgresqlLogsServiceTypePostgreSQL:
		return pgv1.ListClusterLogsRequest_POSTGRESQL
	case internalapi.PostgresqlLogsServiceTypePooler:
		return pgv1.ListClusterLogsRequest_POOLER
	default:
		return pgv1.ListClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
	}
}

func (pgc *pgClient) Logs(
	ctx context.Context,
	clusterID string,
	st internalapi.PostgresqlLogsServiceType,
	opts internalapi.LogsOptions,
) ([]internalapi.LogMessage, string, error) {
	req := &pgv1.ListClusterLogsRequest{
		ClusterId:   clusterID,
		ServiceType: postgresqlLogsServiceTypeToGRPC(st),
	}

	if opts.ColumnFilter.Valid {
		req.ColumnFilter = opts.ColumnFilter.Strings
	}
	if opts.FromTS.Valid {
		req.FromTime = grpcsrv.TimeToGRPC(opts.FromTS.Time)
	}
	if opts.ToTS.Valid {
		req.ToTime = grpcsrv.TimeToGRPC(opts.ToTS.Time)
	}
	if opts.PageSize.Valid {
		req.PageSize = opts.PageSize.Int64
	}
	if opts.PageToken.Valid {
		req.PageToken = opts.PageToken.String
	}
	if opts.AlwaysNextPageToken.Valid {
		req.AlwaysNextPageToken = opts.AlwaysNextPageToken.Bool
	}

	resp, err := pgc.cluster.ListLogs(ctx, req)
	if err != nil {
		return nil, "", xerrors.Errorf("error while listing logs: %w", err)
	}

	var res []internalapi.LogMessage
	for _, l := range resp.Logs {
		res = append(
			res,
			internalapi.LogMessage{
				Timestamp: grpcsrv.TimeFromGRPC(l.Timestamp),
				Message:   l.Message,
			},
		)
	}

	return res, resp.NextPageToken, nil
}

func (pgc *pgClient) Databases(ctx context.Context, clusterID string) ([]internalapi.PostgreSQLDatabase, error) {
	resp, err := pgc.database.List(ctx, &pgv1.ListDatabasesRequest{ClusterId: clusterID})
	if err != nil {
		return nil, xerrors.Errorf("error while listing databases: %w", err)
	}

	var res []internalapi.PostgreSQLDatabase
	for _, db := range resp.Databases {
		res = append(res, internalapi.PostgreSQLDatabase{Name: db.Name, ClusterID: db.ClusterId, Owner: db.Owner, LCCtype: db.LcCtype, LCCollate: db.LcCollate})
	}

	return res, nil
}
