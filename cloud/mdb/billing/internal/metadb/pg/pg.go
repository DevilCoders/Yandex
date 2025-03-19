package pg

import (
	"context"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/billing/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

var _ metadb.MetaDB = &metaDB{}

var (
	querySelectBackups = sqlutil.Stmt{
		Name: "SelectBackups",
		// language=PostgreSQL
		Query: `
SELECT
	b.backup_id,
    CASE
        WHEN c.type in ('postgresql_cluster', 'mysql_cluster')
        THEN jsonb_extract_path(b.metadata, 'compressed_size')
        WHEN c.type = 'mongodb_cluster'
        THEN jsonb_extract_path(b.metadata, 'data_size')        
        ELSE NULL
    END AS data_size
FROM dbaas.backups b
JOIN dbaas.clusters c
  ON (b.cid = c.cid)
WHERE b.cid= :cid
  AND b.status IN ('DONE')
`,
	}

	querySelectClusterSpace = sqlutil.Stmt{
		Name: "SelectClusterSpace",
		// language=PostgreSQL
		Query: `
SELECT
    SUM(h.space_limit)
FROM
    dbaas.hosts h
JOIN dbaas.subclusters s ON (h.subcid = s.subcid)
WHERE cid = :cid
`,
	}

	querySelectClusterMeta = sqlutil.Stmt{
		Name: "SelectClusterMeta",
		// language=PostgreSQL
		Query: `
SELECT
    c.cid as cluster_id,
    c.type cluster_type,
    cl.cloud_ext_id cloud_id,
    f.folder_ext_id folder_id
FROM
    dbaas.clusters c
JOIN
    dbaas.folders f USING(folder_id)
JOIN
    dbaas.clouds cl USING(cloud_id)
WHERE
    c.cid = :cid;`,
	}

	querySelectClickHouseClusterDetails = sqlutil.Stmt{
		Name: "SelectClusterCloudStorageBucket",
		// language=PostgreSQL
		Query: `
SELECT DISTINCT
    cl.cloud_ext_id AS cloud_id,
    f.folder_ext_id AS folder_id,
    r.name AS cloud_region,
    r.cloud_provider as cloud_provider,
    CAST(p.value#>>'{data,cloud_storage,s3,bucket}' AS text) AS bucket,
    fl.type as resource_preset_type
FROM
    dbaas.clusters c JOIN
    dbaas.subclusters sc USING (cid) JOIN
    dbaas.pillar p ON (sc.subcid = p.subcid) JOIN
    dbaas.folders f USING (folder_id) JOIN 
    dbaas.clouds cl USING (cloud_id) JOIN
    dbaas.hosts h ON (sc.subcid = h.subcid) JOIN
    dbaas.geo g USING (geo_id) JOIN 
    dbaas.regions r USING(region_id) JOIN
    dbaas.flavors fl ON(fl.id = h.flavor)
WHERE c.cid = :cid AND CAST((p.value#>'{data,cloud_storage,enabled}') AS boolean)`,
	}
)

// metaDB implements MetaDB interface for PostgreSQL
type metaDB struct {
	logger  log.Logger
	cluster *sqlutil.Cluster
}

// New constructs metaDB
func New(cfg pgutil.Config, l log.Logger) (metadb.MetaDB, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(l)))
	if err != nil {
		return nil, err
	}

	return &metaDB{logger: l, cluster: cluster}, nil
}

func (mdb *metaDB) Begin(ctx context.Context, ns sqlutil.NodeStateCriteria) (context.Context, error) {
	binding, err := sqlutil.Begin(ctx, mdb.cluster, ns, nil)
	if err != nil {
		return ctx, err
	}

	return sqlutil.WithTxBinding(ctx, binding), nil
}

func (mdb *metaDB) Commit(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Commit(ctx)
}

func (mdb *metaDB) Rollback(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Rollback(ctx)
}

func (mdb *metaDB) ListBackups(ctx context.Context, clusterID string, fromTS, untilTS time.Time) ([]metadb.Backup, error) {
	var resList []metadb.Backup

	parser := func(rows *sqlx.Rows) error {
		var row backupRow
		if err := rows.StructScan(&row); err != nil {
			return err
		}
		resList = append(resList, formatBackup(row))
		return nil
	}

	_, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		querySelectBackups,
		map[string]interface{}{
			"cid":      clusterID,
			"from_ts":  fromTS,
			"until_ts": untilTS,
		},
		parser,
		mdb.logger,
	)

	if err != nil {
		return nil, err
	}

	return resList, nil
}

func (mdb *metaDB) ClusterSpace(ctx context.Context, cid string) (int64, error) {
	var size int64
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&size)
	}

	count, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		querySelectClusterSpace,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return 0, err
	}
	if count == 0 {
		return 0, metadb.ErrDataNotFound
	}

	return size, nil
}

func (mdb *metaDB) ClusterMeta(ctx context.Context, cid string) (metadb.Cluster, error) {
	cluster := clusterRow{}
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&cluster)
	}

	count, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		querySelectClusterMeta,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return metadb.Cluster{}, err
	}
	if count == 0 {
		return metadb.Cluster{}, metadb.ErrDataNotFound
	}

	return formatCluster(cluster), nil
}

func (mdb *metaDB) ClickHouseCloudStorageDetails(ctx context.Context, cid string) (metadb.ClickHouseCloudStorageDetails, error) {
	var result metadb.ClickHouseCloudStorageDetails
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&result)
	}
	count, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		querySelectClickHouseClusterDetails,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return result, err
	}
	if count == 0 {
		return result, metadb.ErrDataNotFound
	}
	if count > 1 {
		return metadb.ClickHouseCloudStorageDetails{}, xerrors.Errorf("unexpected cluster details count %d", count)
	}

	return result, nil
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
