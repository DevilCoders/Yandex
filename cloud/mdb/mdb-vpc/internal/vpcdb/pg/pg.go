package pg

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

type DB struct {
	logger log.Logger

	cluster *sqlutil.Cluster
}

func (d *DB) Close() error {
	return d.cluster.Close()
}

func (d *DB) IsReady(_ context.Context) error {
	node := d.cluster.Alive()
	if node == nil {
		return semerr.Unavailable("postgresql not available")
	}

	return nil
}

func New(cfg pgutil.Config, logger log.Logger) (vpcdb.VPCDB, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(logger)))
	if err != nil {
		return nil, err
	}

	return &DB{
		logger:  logger,
		cluster: cluster,
	}, nil
}

func (d *DB) GetDB() *sqlutil.Cluster {
	return d.cluster
}
