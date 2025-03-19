package clickhouse

import (
	"context"
	"database/sql"
	"time"

	// ClickHouse driver
	_ "github.com/ClickHouse/clickhouse-go"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/chutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func DefaultConfig() Config {
	return Config{
		DataOpts: DataOptions{
			TimeColumn: "log_time",
		},
		DB: chutil.Config{
			DB:       "mdb",
			User:     "dbaas_reader",
			Secure:   true,
			Compress: true,
			CAFile:   "/opt/yandex/allCAs.pem",
		},
	}
}

type DataOptions struct {
	TimeColumn string `json:"time_column" yaml:"time_column"`
}

type Config struct {
	DataOpts DataOptions   `json:"data_opts" yaml:"data_opts"`
	DB       chutil.Config `json:"db" yaml:"db"`
}

type Backend struct {
	lg   log.Logger
	node sqlutil.Node
	cfg  Config
}

var _ logsdb.Backend = &Backend{}

func New(cfg Config, lg log.Logger) (*Backend, error) {
	db, err := sql.Open("clickhouse", cfg.DB.URI())
	if err != nil {
		return nil, err
	}

	return NewWithDB(db, cfg, lg), nil
}

func NewWithDB(db *sql.DB, cfg Config, lg log.Logger) *Backend {
	node := sqlutil.NewNode("logsdb", sqlx.NewDb(db, "clickhouse"))
	return &Backend{
		lg:   lg,
		node: node,
		cfg:  cfg,
	}
}

func (b *Backend) IsReady(ctx context.Context) error {
	if err := b.node.DBx().PingContext(ctx); err != nil {
		return semerr.WrapWithUnavailable(err, "unavailable")
	}

	return nil
}

func (b *Backend) Logs(
	ctx context.Context,
	cid string,
	st logsdb.LogType,
	columnFilter []string,
	fromTS, toTS time.Time,
	limit, offset int64,
	conditions []sqlfilter.Term,
) (res []logs.Message, more bool, err error) {
	query, whereParams, err := BuildQuery(st, columnFilter, conditions, b.cfg.DataOpts.TimeColumn)
	if err != nil {
		return nil, false, err
	}

	queryParams := map[string]interface{}{
		"cid":       cid,
		"from_time": fromTS.Unix(),
		"to_time":   toTS.Unix(),
		"from_ms":   fromTS.Nanosecond() / 1e6,
		"to_ms":     toTS.Nanosecond() / 1e6,
		"offset":    offset,
		"limit":     limit + 1, // Add 1 to limit to check if there is more data
	}
	for k, v := range whereParams {
		queryParams[k] = v
	}

	nextMessageToken := offset
	parser := func(rows *sqlx.Rows) error {
		m, err := stringMapScan(rows)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", query.Name, err)
		}
		// Extract log timestamp (this deletes value from map)
		sec, err := extractInt64FromStringMap(ParamSeconds, m)
		if err != nil {
			return xerrors.Errorf("failed to parse query %s result: %w", query.Name, err)
		}
		ms, err := extractInt64FromStringMap(ParamMilliseconds, m)
		if err != nil {
			return xerrors.Errorf("failed to parse query %s result: %w", query.Name, err)
		}
		nextMessageToken++
		res = append(res, logs.Message{Timestamp: time.Unix(sec, ms*1000000), Message: m, NextMessageToken: nextMessageToken})
		return nil
	}

	// TODO: remove it after this fix will be shipped to our vendor https://github.com/ClickHouse/clickhouse-go/pull/350
	conn, err := b.node.DBx().Connx(ctx)
	if err != nil {
		return nil, false, semerr.WrapWithUnavailable(err, "unavailable")
	}
	defer conn.Close()

	if err = conn.PingContext(ctx); err != nil {
		return nil, false, semerr.WrapWithUnavailable(err, "unavailable")
	}
	_, err = sqlutil.QueryConn(
		ctx,
		conn,
		b.node.DBx().DriverName(),
		"",
		query,
		queryParams,
		parser,
		b.lg,
	)
	if err != nil {
		return nil, false, chutil.HandleErrBadConn(ctx, err)
	}
	if int64(len(res)) > limit {
		// Remove additional data if there is any
		res = res[:len(res)-1]
		more = true
	}

	return res, more, nil
}
