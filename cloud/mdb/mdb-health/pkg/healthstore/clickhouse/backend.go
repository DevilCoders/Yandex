package clickhouse

import (
	"context"
	"database/sql"

	_ "github.com/ClickHouse/clickhouse-go"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/chutil"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type backend struct {
	node sqlutil.Node
	l    log.Logger
}

func (b backend) Name() string {
	return "clickhouse"
}

func (b backend) Close() error {
	return nil
}

func (b *backend) IsReady(ctx context.Context) error {
	if err := b.node.DBx().PingContext(ctx); err != nil {
		return semerr.WrapWithUnavailable(err, "unavailable")
	}

	return nil
}

var _ healthstore.Backend = &backend{}

func New(logger log.Logger, config chutil.Config) (healthstore.Backend, error) {
	if err := config.Validate(); err != nil {
		return nil, xerrors.Errorf("validate config: %w", err)
	}

	if config.CAFile != "" {
		if err := config.RegisterTLSConfig(); err != nil {
			return nil, xerrors.Errorf("register tls: %w", err)
		}
	}

	db, err := sql.Open("clickhouse", config.URI())
	if err != nil {
		return nil, xerrors.Errorf("create ch connection: %w", err)
	}

	node := sqlutil.NewNode("healthdb", sqlx.NewDb(db, "clickhouse"))
	return &backend{
		node: node,
		l:    logger,
	}, nil
}
