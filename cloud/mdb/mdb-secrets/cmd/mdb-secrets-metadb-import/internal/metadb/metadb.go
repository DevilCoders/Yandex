package metadb

import (
	"context"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

var (
	queryAllHostsCerts = sqlutil.Stmt{
		Name: "allHostsCerts",
		// Use `WITH` cause use materialization.
		// Postgres want to use Bitmap Index Scan on i_pillar_value,
		// its works badly, cause there are a lot of deleted clusters.
		// https://paste.yandex-team.ru/3976480
		// language=PostgreSQL
		Query: `
WITH all_pillars AS (
    SELECT clusters.cid, fqdn, value
      FROM dbaas.pillar
      JOIN dbaas.hosts
     USING (fqdn)
      JOIN dbaas.subclusters ON (hosts.subcid=subclusters.subcid)
      JOIN dbaas.clusters ON (clusters.cid=subclusters.cid)
     WHERE code.visible(clusters) AND fqdn IS NOT NULL) 
SELECT cid AS cluster_id,
       fqdn,
       value->'cert.crt'->>'data' AS crt,
       value->'cert.key'->>'data' AS key
  FROM all_pillars WHERE value ?& '{cert.crt, cert.key}'`,
	}

	queryAllClusterCerts = sqlutil.Stmt{
		Name: "allClusterCerts",
		// language=PostgreSQL
		Query: `
WITH all_pillars AS (
    SELECT clusters.cid, fqdn, value
      FROM dbaas.pillar
      JOIN dbaas.hosts
     USING (fqdn)
      JOIN dbaas.subclusters ON (hosts.subcid=subclusters.subcid)
      JOIN dbaas.clusters ON (clusters.cid=subclusters.cid)
     WHERE code.visible(clusters) AND fqdn IS NOT NULL AND clusters.cid = :cid) 
SELECT cid AS cluster_id,
       fqdn,
       value->'cert.crt'->>'data' AS crt,
       value->'cert.key'->>'data' AS key
  FROM all_pillars WHERE value ?& '{cert.crt, cert.key}'`,
	}
)

type HostCerts struct {
	ClusterID    string `db:"cluster_id"`
	FQDN         string `db:"fqdn"`
	EncryptedKey string `db:"key"`
	EncryptedCrt string `db:"crt"`
}

type MetaDB struct {
	l       log.Logger
	cluster *sqlutil.Cluster
}

// New constructs metaDB
func New(cfg pgutil.Config, l log.Logger) (*MetaDB, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(l)))
	if err != nil {
		return nil, err
	}

	return &MetaDB{
		l:       l,
		cluster: cluster,
	}, nil
}

func DefaultConfig() pgutil.Config {
	return pgutil.DefaultConfig()
}

func (mdb *MetaDB) IsReady(context.Context) error {
	if mdb.cluster.Primary() == nil {
		return xerrors.New("meta not ready, cause primary unavailable")
	}
	return nil
}

func (mdb *MetaDB) GetHostsCertificates(ctx context.Context) ([]HostCerts, error) {
	var ret []HostCerts
	_, err := sqlutil.QueryContext(
		ctx, mdb.cluster.PrimaryChooser(), queryAllHostsCerts, map[string]interface{}{}, func(rows *sqlx.Rows) error {
			var row HostCerts
			if err := rows.StructScan(&row); err != nil {
				return err
			}
			ret = append(ret, row)
			return nil
		}, mdb.l,
	)

	if err != nil {
		return nil, xerrors.Errorf("allHostsCerts query failed with: %w", err)
	}
	return ret, nil
}

func (mdb *MetaDB) GetClusterCertificates(ctx context.Context, cid string) ([]HostCerts, error) {
	var ret []HostCerts
	_, err := sqlutil.QueryContext(
		ctx, mdb.cluster.PrimaryChooser(), queryAllClusterCerts, map[string]interface{}{
			"cid": cid,
		}, func(rows *sqlx.Rows) error {
			var row HostCerts
			if err := rows.StructScan(&row); err != nil {
				return err
			}
			ret = append(ret, row)
			return nil
		}, mdb.l,
	)

	if err != nil {
		return nil, xerrors.Errorf("%q query failed with: %w", queryAllClusterCerts.Name, err)
	}
	return ret, nil
}

func (mdb *MetaDB) RemoveCertificates(ctx context.Context, clusterID, fqdn string) error {
	master := mdb.cluster.Primary()
	if master == nil {
		return xerrors.New("primary unavailable")
	}
	result, err := master.DBx().ExecContext(
		ctx,
		// language=PostgreSQL
		`DO $$
DECLARE
    i_cid  text := $1;
	i_fqdn text := $2;
    v_rev  bigint;
BEGIN
	v_rev := (code.lock_cluster(i_cid, 'import certs to mdb-secretes')).rev;
	UPDATE dbaas.pillar
       SET value = (value - 'cert.key') - 'cert.crt'
     WHERE fqdn = i_fqdn;
	PERFORM code.complete_cluster_change(i_cid, v_rev);
END;
$$`,
		clusterID,
		fqdn,
	)
	if err != nil {
		return xerrors.Errorf("remove certificates: %w", err)
	}
	if _, err := result.RowsAffected(); err != nil {
		return xerrors.Errorf("remove certificates: %w", err)
	}
	return nil
}
