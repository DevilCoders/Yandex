package grpc

import (
	"context"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	grpcsrv "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	grpcch "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/pkg/internalapi"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type chClient struct {
	c *Client

	cluster  chv1.ClusterServiceClient
	database chv1.DatabaseServiceClient
}

var _ internalapi.ClickHouseClient = &chClient{}

func (chc *chClient) IsReady(ctx context.Context) error {
	return chc.c.IsReady(ctx)
}

func (chc *chClient) Logs(
	ctx context.Context,
	clusterID string,
	opts internalapi.LogsOptions,
) ([]internalapi.LogMessage, string, error) {
	req := &chv1.ListClusterLogsRequest{
		ClusterId:   clusterID,
		ServiceType: grpcch.ListLogsServiceTypeToGRPC(logs.ServiceTypeClickHouse),
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

	resp, err := chc.cluster.ListLogs(ctx, req)
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

func (chc *chClient) Database(ctx context.Context, clusterID, name string) (internalapi.ClickHouseDatabase, error) {
	resp, err := chc.database.Get(ctx, &chv1.GetDatabaseRequest{ClusterId: clusterID, DatabaseName: name})
	if err != nil {
		return internalapi.ClickHouseDatabase{}, xerrors.Errorf("error while getting database: %w", err)
	}

	return internalapi.ClickHouseDatabase{ClusterID: resp.ClusterId, Name: resp.Name}, nil

}

func (chc *chClient) Databases(ctx context.Context, clusterID string) ([]internalapi.ClickHouseDatabase, error) {
	resp, err := chc.database.List(ctx, &chv1.ListDatabasesRequest{ClusterId: clusterID})
	if err != nil {
		return nil, xerrors.Errorf("error while listing databases: %w", err)
	}

	var res []internalapi.ClickHouseDatabase
	for _, db := range resp.Databases {
		res = append(res, internalapi.ClickHouseDatabase{ClusterID: db.ClusterId, Name: db.Name})
	}

	return res, nil
}
