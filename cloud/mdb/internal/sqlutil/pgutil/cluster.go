package pgutil

import (
	"github.com/jmoiron/sqlx"
	"golang.yandex/hasql/checkers"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	// ErrSQLDriver failed to open SQL driver
	ErrSQLDriver = xerrors.NewSentinel("failed to open SQL driver")
)

// NewCluster constructs PostgreSQL cluster object
func NewCluster(pgcfg Config, opts ...sqlutil.ClusterOption) (*sqlutil.Cluster, error) {
	nodes := make([]sqlutil.Node, 0, len(pgcfg.Addrs))
	for _, addr := range pgcfg.Addrs {
		connString := ConnString(addr, pgcfg.DB, pgcfg.User, pgcfg.Password.Unmask(), pgcfg.SSLMode.String(), pgcfg.SSLRootCert)
		configKey, err := RegisterConfigForConnString(connString, pgcfg.TCP)
		if err != nil {
			return nil, ErrSQLDriver.Wrap(err)
		}

		db, err := sqlx.Open("pgx", configKey)
		if err != nil {
			return nil, ErrSQLDriver.Wrap(err)
		}

		db.SetMaxOpenConns(pgcfg.MaxOpenConn)
		db.SetMaxIdleConns(pgcfg.MaxIdleConn)
		db.SetConnMaxIdleTime(pgcfg.MaxIdleTime)
		db.SetConnMaxLifetime(pgcfg.MaxLifetime)

		nodes = append(nodes, sqlutil.NewNode(addr, db))
	}

	// We put picker first so that callers can override with another picker
	opts = append([]sqlutil.ClusterOption{sqlutil.WithNodePicker(sqlutil.PickNodeClosest())}, opts...)
	return sqlutil.NewCluster(nodes, checkers.PostgreSQL, opts...)
}
