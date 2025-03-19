package pg

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/metadb"
	"a.yandex-team.ru/library/go/core/log"
)

// metaDB implements MetaDB interface for PostgreSQL
type metaDB struct {
	logger  log.Logger
	cluster *sqlutil.Cluster
}

var _ metadb.MetaDB = &metaDB{}

func New(cluster *sqlutil.Cluster, logger log.Logger) *metaDB {
	return &metaDB{
		logger:  logger,
		cluster: cluster,
	}
}

func (mdb *metaDB) Close() error {
	return mdb.cluster.Close()
}

func (mdb *metaDB) IsReady(context.Context) error {
	if mdb.cluster.Primary() == nil {
		return semerr.Unavailable("unavailable")
	}

	return nil
}
