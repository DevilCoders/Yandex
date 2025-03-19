package pg

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-reaper/internal/metadb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

type cloudWithClustersRow struct {
	CloudID  string           `db:"cloud_id"`
	Clusters pgtype.TextArray `db:"cluster_ids"`
}

type clusterRow struct {
	CID          string    `db:"cid"`
	Name         string    `db:"name"`
	Env          string    `db:"env"`
	CType        string    `db:"type"`
	LastActionAt time.Time `db:"last_action_at"`
}

var (
	queryCloudsWithRunningClusters = sqlutil.Stmt{
		Name: "CloudsWithRunningClusters",
		// language=PostgreSQL
		Query: `
SELECT cl.cloud_ext_id AS cloud_id, array_agg(c.cid) AS cluster_ids
FROM dbaas.clusters c
JOIN dbaas.folders f USING (folder_id)
JOIN dbaas.clouds cl USING (cloud_id)
WHERE c.status = 'RUNNING' AND NOT (:exclude_tagged AND c.name LIKE '%mdb-auto-purge-off%')
GROUP BY cl.cloud_ext_id
HAVING COUNT(*) > 0`}
	queryClusters = sqlutil.Stmt{
		Name: "Clusters",
		// language=PostgreSQL
		Query: `
SELECT cid, name, type, env, (SELECT MAX(create_ts) FROM dbaas.worker_queue wq WHERE wq.cid = c.cid) AS last_action_at
FROM dbaas.clusters c
WHERE cid IN (%s)`}
)

// MetaDB implements interface for PostgreSQL
type metaDB struct {
	logger  log.Logger
	cluster *sqlutil.Cluster
}

func New(cfg pgutil.Config, logger log.Logger) (*metaDB, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(logger)))
	if err != nil {
		return nil, err
	}
	return NewWithCluster(cluster, logger), nil
}

func NewWithCluster(cluster *sqlutil.Cluster, logger log.Logger) *metaDB {
	return &metaDB{
		logger:  logger,
		cluster: cluster,
	}
}

var _ metadb.MetaDB = &metaDB{}

func (mdb *metaDB) IsReady(_ context.Context) error {
	node := mdb.cluster.Alive()
	if node == nil {
		return semerr.Unavailable("unavailable")
	}

	return nil
}

func (mdb *metaDB) CloudsWithRunningClusters(ctx context.Context, excludeTagged bool) (metadb.ClusterIDsByCloudID, error) {
	ret := make(metadb.ClusterIDsByCloudID)
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.AliveChooser(),
		queryCloudsWithRunningClusters,
		map[string]interface{}{"exclude_tagged": excludeTagged},
		func(rows *sqlx.Rows) error {
			var r cloudWithClustersRow
			if err := rows.StructScan(&r); err != nil {
				return err
			}
			var clusters metadb.ClusterIDs
			for _, cl := range r.Clusters.Elements {
				clusters = append(clusters, cl.String)
			}
			ret[r.CloudID] = clusters
			return nil
		},
		mdb.logger); err != nil {
		return nil, xerrors.Errorf("cloud with running clusters select: %w", err)
	}
	return ret, nil
}

func (mdb *metaDB) Clusters(ctx context.Context, clusterIDs metadb.ClusterIDs) ([]metadb.Cluster, error) {
	var quotedCIDs []string
	for _, CID := range clusterIDs {
		quotedCIDs = append(quotedCIDs, fmt.Sprintf("'%s'", CID))
	}

	query := queryClusters.Format(strings.Join(quotedCIDs, ", "))

	var ret []metadb.Cluster
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.AliveChooser(),
		query,
		nil,
		func(rows *sqlx.Rows) error {
			var r clusterRow
			if err := rows.StructScan(&r); err != nil {
				return err
			}
			ret = append(ret, metadb.Cluster{
				ID:           r.CID,
				Name:         r.Name,
				Type:         metadb.ClusterType(r.CType),
				Environment:  r.Env,
				LastActionAt: r.LastActionAt,
			})
			return nil
		},
		mdb.logger); err != nil {
		return nil, xerrors.Errorf("cloud with running clusters select: %w", err)
	}
	return ret, nil
}

func (mdb *metaDB) Close() error {
	return mdb.cluster.Close()
}
