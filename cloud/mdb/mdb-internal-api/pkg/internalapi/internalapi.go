package internalapi

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
)

//go:generate ../../../scripts/mockgen.sh Client,PostgreSQLClient,ClickHouseClient

// Client is an interface to mdb-internal-api service
type Client interface {
	ready.Checker

	PostgreSQL() PostgreSQLClient
	ClickHouse() ClickHouseClient
}

type LogMessage = logs.Message

type PostgreSQLDatabase = pgmodels.Database

// PostgreSQLClient provides API for manipulating Cluster clusters
type PostgreSQLClient interface {
	ready.Checker

	Logs(ctx context.Context, clusterID string, st PostgresqlLogsServiceType, opts LogsOptions) ([]LogMessage, string, error)

	Databases(ctx context.Context, clusterID string) ([]PostgreSQLDatabase, error)
}

type PostgresqlLogsServiceType int

const (
	PostgresqlLogsServiceTypePostgreSQL = PostgresqlLogsServiceType(logs.ServiceTypePostgreSQL)
	PostgresqlLogsServiceTypePooler     = PostgresqlLogsServiceType(logs.ServiceTypePooler)
)

type ClickHouseDatabase = chmodels.Database

// ClickHouseClient provides API for manipulating ClickHouse clusters
type ClickHouseClient interface {
	ready.Checker

	Logs(ctx context.Context, clusterID string, opts LogsOptions) ([]LogMessage, string, error)

	Database(ctx context.Context, clusterID, name string) (ClickHouseDatabase, error)
	Databases(ctx context.Context, clusterID string) ([]ClickHouseDatabase, error)
}

// LogsOptions holds optional arguments for Logs func
type LogsOptions struct {
	ColumnFilter        optional.Strings
	FromTS              optional.Time
	ToTS                optional.Time
	PageSize            optional.Int64
	PageToken           optional.String
	AlwaysNextPageToken optional.Bool
}
