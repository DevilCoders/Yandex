package pg

import (
	"context"
	"time"

	"github.com/jackc/pgconn"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

type Backend struct {
	log log.Logger

	cluster                  *sqlutil.Cluster
	defMaxAllowedTimeAtWalle time.Duration
}

var _ cmsdb.Client = &Backend{}

// New constructs PostgreSQL backend
func New(cfg pgutil.Config, l log.Logger) (cmsdb.Client, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(l)))
	if err != nil {
		return &Backend{}, err
	}

	return &Backend{
		log:     l,
		cluster: cluster,
	}, nil
}

func (b *Backend) Close() error {
	return b.cluster.Close()
}

func (b *Backend) IsReady(_ context.Context) error {
	node := b.cluster.Alive()
	if node == nil {
		return semerr.Unavailable("postgresql not available")
	}

	return nil
}

func (b *Backend) GetDB() *sqlutil.Cluster {
	return b.cluster
}

func wrapError(err error) error {
	if err != nil {
		var pgError *pgconn.PgError
		if xerrors.As(err, &pgError) {
			switch pgError.Code {
			case "23505":
				return semerr.FailedPrecondition(pgError.Message)
			}
		}
	}
	return err
}
