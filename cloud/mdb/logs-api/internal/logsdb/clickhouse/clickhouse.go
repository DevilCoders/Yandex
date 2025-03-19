package clickhouse

import (
	"context"
	"database/sql"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/chutil"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logsdb"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/models"
	"a.yandex-team.ru/library/go/core/log"
)

type Backend struct {
	l    log.Logger
	node sqlutil.Node
	cfg  Config
}

var _ logsdb.Backend = &Backend{}

func New(cfg Config, l log.Logger) (*Backend, error) {
	if err := cfg.DB.RegisterTLSConfig(); err != nil {
		return nil, err
	}

	db, err := sql.Open("clickhouse", cfg.DB.URI())
	if err != nil {
		return nil, err
	}

	return NewWithDB(db, cfg, l), nil
}

func NewWithDB(db *sql.DB, cfg Config, l log.Logger) *Backend {
	node := sqlutil.NewNode("logsdb", sqlx.NewDb(db, "clickhouse"))
	return &Backend{
		l:    l,
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

type logDB struct {
	SourceType int32          `db:"source_type"`
	SourceID   string         `db:"resource_id"`
	Instance   sql.NullString `db:"instance"`
	Seconds    int64          `db:"seconds"`
	MS         int64          `db:"ms"`
	Severity   string         `db:"severity"`
	Message    string         `db:"message"`
}

func (b *Backend) Logs(ctx context.Context, criteria logsdb.Criteria) (res []models.Log, more bool, err error) {
	query, queryParams, err := BuildQuery(b.cfg.DataOpts, criteria)
	if err != nil {
		return nil, false, err
	}

	b.l.Infof("my_q: %s", query.Query)
	nextMessageToken := criteria.Offset
	parser := func(rows *sqlx.Rows) error {
		var l logDB
		if err := rows.StructScan(&l); err != nil {
			return err
		}

		nextMessageToken++
		res = append(res, models.Log{
			Source: models.LogSource{
				Type: models.LogSourceType(l.SourceType),
				ID:   l.SourceID,
			},
			Instance: optional.String{
				Valid:  l.Instance.Valid,
				String: l.Instance.String,
			},
			Timestamp: time.Unix(l.Seconds, l.MS*1e6),
			Severity:  models.LogLevel(l.Severity),
			Message:   l.Message,
			Offset:    nextMessageToken,
		})

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
		b.node.Addr(),
		query,
		queryParams,
		parser,
		b.l,
	)
	if err != nil {
		return nil, false, chutil.HandleErrBadConn(ctx, err)
	}
	if int64(len(res)) > criteria.Limit {
		// Remove additional data if there is any
		res = res[:len(res)-1]
		more = true
	}

	return res, more, nil
}
