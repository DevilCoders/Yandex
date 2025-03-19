package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/provider/internal/pgpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

func (pg *PostgreSQL) Database(ctx context.Context, cid, name string) (pgmodels.Database, error) {
	var db pgmodels.Database
	if err := pg.operator.ReadOnCluster(ctx, cid, clusters.TypePostgreSQL,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			var pillar pgpillars.Cluster
			if err = cl.Pillar(&pillar); err != nil {
				return err
			}

			// Construct response
			for dbname, dbpillar := range pillar.Data.DBs {
				if dbname == name {
					db = pgmodels.Database{Name: dbname, ClusterID: cid, Owner: dbpillar.User, LCCollate: dbpillar.LCCollate, LCCtype: dbpillar.LCCtype}
					return nil
				}
			}

			return semerr.NotFoundf("database %q not found", name)
		},
	); err != nil {
		return pgmodels.Database{}, err
	}

	return db, nil
}

func (pg *PostgreSQL) Databases(ctx context.Context, cid string, limit, offset int64) ([]pgmodels.Database, error) {
	var dbs []pgmodels.Database
	if err := pg.operator.ReadOnCluster(ctx, cid, clusters.TypePostgreSQL,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			var pillar pgpillars.Cluster
			if err = cl.Pillar(&pillar); err != nil {
				return err
			}

			// Construct response
			for name, db := range pillar.Data.DBs {
				dbs = append(dbs, pgmodels.Database{Name: name, ClusterID: cid, Owner: db.User, LCCollate: db.LCCollate, LCCtype: db.LCCtype})
			}
			return nil
		},
	); err != nil {
		return nil, err
	}

	return dbs, nil
}
