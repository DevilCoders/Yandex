package pg

import (
	"context"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/secretsstore"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

var (
	querySelectClusterSecret = sqlutil.Stmt{
		Name:  "SelectClusterSecret",
		Query: "SELECT public_key FROM dbaas.clusters WHERE cid = :cid",
	}
)

type backend struct {
	logger log.Logger

	cluster *sqlutil.Cluster
}

// New constructs PostgreSQL backend
func New(cfg pgutil.Config, l log.Logger) (secretsstore.Backend, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(l)))
	if err != nil {
		return nil, err
	}

	return NewWithCluster(cluster, l), nil
}

// NewWithCluster constructs PostgreSQL backend with custom cluster (useful for mocking)
func NewWithCluster(cluster *sqlutil.Cluster, l log.Logger) secretsstore.Backend {
	return &backend{
		logger:  l,
		cluster: cluster,
	}
}

func (b *backend) Close() error {
	return b.cluster.Close()
}

func (b *backend) IsReady(ctx context.Context) error {
	node := b.cluster.Alive()
	if node == nil {
		return semerr.Unavailable("unavailable")
	}

	return nil
}

func (b *backend) LoadClusterSecret(ctx context.Context, cid string) ([]byte, error) {
	var secret []byte
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&secret)
	}

	count, err := sqlutil.QueryNode(
		ctx,
		b.cluster.Alive(),
		querySelectClusterSecret,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}
	if count == 0 {
		return nil, secretsstore.ErrSecretNotFound.WithFrame()
	}

	return secret, nil
}
