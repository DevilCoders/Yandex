package pg

import (
	"context"

	"github.com/jackc/pgconn"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

func DefaultConfig() pgutil.Config {
	cfg := pgutil.DefaultConfig()
	cfg.DB = "dbaas_metadb"
	cfg.User = "dbaas_api"
	cfg.SSLMode = pgutil.VerifyFullSSLMode
	cfg.SSLRootCert = "/opt/yandex/allCAs.pem"
	return cfg
}

type Backend struct {
	logger log.Logger

	cluster *sqlutil.Cluster
}

var _ metadb.Backend = &Backend{}

// New constructs PostgreSQL backend
func New(cfg pgutil.Config, l log.Logger) (*Backend, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(l)))
	if err != nil {
		return nil, err
	}

	return NewWithCluster(cluster, l), nil
}

// NewWithCluster constructs PostgreSQL backend with custom cluster (useful for mocking)
func NewWithCluster(cluster *sqlutil.Cluster, l log.Logger) *Backend {
	b := &Backend{
		logger:  l,
		cluster: cluster,
	}

	return b
}

func (b *Backend) Close() error {
	return b.cluster.Close()
}

func (b *Backend) IsReady(ctx context.Context) error {
	node := b.cluster.Alive()
	if node == nil {
		return semerr.Unavailable("unavailable")
	}

	return nil
}

func (b *Backend) Begin(ctx context.Context, ns sqlutil.NodeStateCriteria) (context.Context, error) {
	binding, err := sqlutil.Begin(ctx, b.cluster, ns, nil)
	if err != nil {
		return ctx, err
	}

	return sqlutil.WithTxBinding(ctx, binding), nil
}

func (b *Backend) Commit(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Commit(ctx)
}

func (b *Backend) Rollback(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Rollback(ctx)
}

func wrapError(err error) error {
	if err != nil {
		var pgError *pgconn.PgError
		if xerrors.As(err, &pgError) {
			switch pgError.Code {
			case "MDB02":
				return semerr.NotFound(pgError.Message)
			case "23514":
				return semerr.FailedPrecondition(pgError.Message)
			}
		}
	}
	return err
}
