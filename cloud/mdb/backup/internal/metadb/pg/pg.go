package pg

import (
	"context"
	"database/sql"
	"encoding/json"
	"time"

	"github.com/jackc/pgconn"
	"github.com/jackc/pgerrcode"
	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

var _ metadb.MetaDB = &metaDB{}

var (
	queryBackupBlanks = sqlutil.Stmt{
		Name: "BackupBlanks",
		Query:
		// language=PostgreSQL
		`
SELECT c.cid, c.type cluster_type, sc.subcid, sc.name subcluster_name, sh.shard_id, scheduled.ts scheduled_ts, COALESCE(CAST(bs.schedule ->> 'sleep' AS INT), :backup_delay_seconds) sleep_seconds
FROM dbaas.backup_schedule as bs
    JOIN LATERAL (
        SELECT *
          FROM
            make_interval(hours => CAST(bs.schedule -> 'start' ->> 'hours' AS INT), mins => CAST(bs.schedule -> 'start' ->> 'minutes' AS INT)) as sched_time,
            unnest(CAST(ARRAY [
						DATE(now() AT TIME ZONE 'UTC') - 1 + sched_time,
						DATE(now() AT TIME ZONE 'UTC') + sched_time
                    ] AS timestamp[])) as ts
        WHERE tsrange(now() AT TIME ZONE 'UTC' - CAST(:past_interval AS INTERVAL), now() AT TIME ZONE 'UTC' + CAST(:future_interval AS INTERVAL)) @> ts)
        scheduled ON true
    JOIN dbaas.clusters c ON (bs.cid = c.cid)
    JOIN dbaas.subclusters sc ON (c.cid = sc.cid)
    LEFT JOIN dbaas.shards sh ON (sc.subcid = sh.subcid)
 WHERE c.status = ANY(CAST(:cluster_statuses AS dbaas.cluster_status[]))
   AND c.type = ANY(CAST(:cluster_types AS dbaas.cluster_type[]))
   AND CAST(bs.schedule ->> 'use_backup_service' as BOOLEAN)
   AND NOT EXISTS (
    SELECT 1
      FROM dbaas.backups b
     WHERE c.cid = b.cid AND sc.subcid = b.subcid AND (sh.shard_id = b.shard_id OR sh.shard_id IS NULL)
       AND b.scheduled_date = DATE(scheduled.ts))
    `,
	}

	queryAddBackup = sqlutil.Stmt{
		Name: "AddBackup",
		Query:
		// language=PostgreSQL
		`
SELECT * FROM code.plan_managed_backup(
	i_backup_id      => :backup_id,
	i_cid            => :cid,
	i_subcid         => :subcid,
	i_shard_id       => :shard_id,
	i_status         => :status,
	i_method         => :method,
	i_initiator      => :initiator,
	i_delayed_until  => :delayed_until,
	i_scheduled_date => :scheduled_date,
	i_parent_ids     => :parent_ids,
	i_child_id       => :child_id
)`,
	}

	queryImportBackup = sqlutil.Stmt{
		Name: "ImportBackup",
		Query:
		// language=PostgreSQL
		`
INSERT INTO dbaas.backups
	(backup_id, cid, subcid, shard_id, status, scheduled_date,  created_at, delayed_until, started_at, finished_at, updated_at, metadata, initiator, method)
VALUES
    (:backup_id, :cid, :subcid, :shard_id, :status, :scheduled_date, :created_at, :delayed_until, :started_at, :finished_at, :updated_at, :metadata, :initiator, :method)
RETURNING *`,
	}

	queryObsoleteFailedBackups = sqlutil.Stmt{
		Name: "ObsoleteFailedBackups",
		Query:
		// language=PostgreSQL
		`
UPDATE dbaas.backups
SET status='DELETED',
    delayed_until = NOW() + TRUNC(RANDOM()  * 1440) * cast('1 MINUTE' AS INTERVAL),
    updated_at = now()
WHERE backup_id IN (
    SELECT
        backup_id
    FROM
        dbaas.backups b JOIN dbaas.clusters c USING(cid)
    WHERE
        b.status = 'CREATE-ERROR'
        AND b.finished_at < NOW() - CAST(:backup_age AS INTERVAL)
        AND c.status = ANY(CAST(:cluster_statuses AS dbaas.cluster_status[]))
)
`,
	}
	querySequentialObsoleteFailedBackups = sqlutil.Stmt{
		Name: "SequentialObsoleteFailedBackups",
		Query:
		// language=PostgreSQL
		`
UPDATE dbaas.backups
SET status='OBSOLETE',
    delayed_until = NOW() + TRUNC(RANDOM()  * 1440) * cast('1 MINUTE' AS INTERVAL),
    updated_at = now()
WHERE backup_id IN (
    SELECT backup_id FROM (
		SELECT
			backup_id,
			ROW_NUMBER() OVER(PARTITION BY b.cid ORDER BY b.finished_at) AS row_num
		FROM
			dbaas.backups b JOIN dbaas.clusters c USING(cid)
		WHERE
			b.status = 'CREATE-ERROR'
			AND b.finished_at < NOW() - CAST(:backup_age AS INTERVAL)
			AND c.status = ANY(CAST(:cluster_statuses AS dbaas.cluster_status[]))
			AND b.cid NOT IN (
			   SELECT back.cid
				FROM dbaas.backups back
			   WHERE back.status IN ('DELETING','OBSOLETE')
			)
    ) backup_by_cid WHERE row_num = 1
)
`,
	}
	queryObsoleteAutomatedBackups = sqlutil.Stmt{
		Name: "ObsoleteAutomatedBackups",
		Query:
		// language=PostgreSQL
		`
WITH RECURSIVE chains(root, backup_id) AS (
 SELECT
  backup_id AS root, backup_id
 FROM
dbaas.backups b
 JOIN
dbaas.backup_schedule sh
 USING (cid)
 JOIN
dbaas.clusters c
 USING (cid)
 WHERE
  b.status = 'DONE'
  AND b.scheduled_date IS NOT NULL
  AND b.initiator = 'SCHEDULE'
  AND b.finished_at < (NOW() - CAST((COALESCE(CAST(sh.schedule->'retain_period' AS TEXT), '7') || ' DAYS') AS INTERVAL))
  AND c.status = ANY(CAST(:cluster_statuses AS dbaas.cluster_status[]))
  AND c.type = ANY(CAST(:cluster_types AS dbaas.cluster_type[]))
UNION ALL
 SELECT root, child_id
 FROM
   dbaas.backups_dependencies bd, chains ch
 WHERE bd.parent_id = ch.backup_id
   AND EXISTS (
    SELECT 1
      FROM dbaas.backups cb
     WHERE cb.backup_id = child_id AND cb.status IN ('CREATING', 'DONE'))
)
,
obsolete_candidate AS (
 SELECT
  ch.root,
  count(b.backup_id) = (SELECT count(1) FROM chains WHERE root=ch.root) as OK
 FROM chains ch
 JOIN dbaas.backups b using(backup_id) JOIN dbaas.backup_schedule sh using(cid)
 WHERE b.status='DONE' AND b.scheduled_date IS NOT NULL AND b.initiator = 'SCHEDULE' AND b.finished_at < (NOW() - CAST((COALESCE(CAST(sh.schedule->'retain_period' AS TEXT), '7') || ' DAYS') AS INTERVAL))
 GROUP BY root
)
UPDATE dbaas.backups
SET status='OBSOLETE',
    delayed_until = NOW() + TRUNC(RANDOM()  * 1440) * cast('1 MINUTE' AS INTERVAL),
    updated_at = now()
WHERE backup_id IN (
 SELECT root FROM obsolete_candidate WHERE ok
)
`,
	}
	querySequentialObsoleteAutomatedBackups = sqlutil.Stmt{
		Name: "SequentialObsoleteAutomatedBackups",
		Query:
		// language=PostgreSQL
		`
WITH RECURSIVE chains(root, backup_id) AS (
 SELECT
  backup_id AS root, backup_id
 FROM
dbaas.backups b
 JOIN
dbaas.backup_schedule sh
 USING (cid)
 JOIN
dbaas.clusters c
 USING (cid)
 WHERE
  b.status = 'DONE'
  AND b.scheduled_date IS NOT NULL
  AND b.initiator = 'SCHEDULE'
  AND b.finished_at < (NOW() - CAST((COALESCE(CAST(sh.schedule->'retain_period' AS TEXT), '7') || ' DAYS') AS INTERVAL))
  AND c.status = ANY(CAST(:cluster_statuses AS dbaas.cluster_status[]))
  AND c.type = ANY(CAST(:cluster_types AS dbaas.cluster_type[]))
UNION ALL
 SELECT root, child_id
 FROM
   dbaas.backups_dependencies bd, chains ch
 WHERE bd.parent_id = ch.backup_id
   AND EXISTS (
    SELECT 1
      FROM dbaas.backups cb
     WHERE cb.backup_id = child_id AND cb.status IN ('CREATING', 'DONE')) -- CREATING status here because of MDB-15641
)
,
obsolete_candidate AS (
 SELECT
  ch.root,
  count(b.backup_id) = (SELECT count(1) FROM chains WHERE root=ch.root) as OK
 FROM chains ch
 JOIN dbaas.backups b using(backup_id) JOIN dbaas.backup_schedule sh using(cid)
 WHERE b.status='DONE'
  AND b.scheduled_date IS NOT NULL
  AND b.initiator = 'SCHEDULE'
  AND b.finished_at < (NOW() - CAST((COALESCE(CAST(sh.schedule->'retain_period' AS TEXT), '7') || ' DAYS') AS INTERVAL))
  AND b.cid NOT IN (
   SELECT back.cid
    FROM dbaas.backups back
   WHERE back.status IN ('DELETING','OBSOLETE')
  )
 GROUP BY root
)
UPDATE dbaas.backups
SET status='OBSOLETE',
    delayed_until = NOW() + TRUNC(RANDOM()  * 1440) * cast('1 MINUTE' AS INTERVAL),
    updated_at = now()
WHERE backup_id IN (
	SELECT backup_id FROM (
		SELECT
			b.backup_id,
			ROW_NUMBER() OVER(PARTITION BY b.cid ORDER BY b.finished_at) AS row_num
		FROM dbaas.backups b
		WHERE b.backup_id IN (
			SELECT root FROM obsolete_candidate WHERE ok
		)
	) backups_by_cid WHERE row_num = 1
)
`,
	}
	queryPurgeBackups = sqlutil.Stmt{
		Name: "PurgeBackups",
		Query:
		// language=PostgreSQL
		`
DELETE FROM dbaas.backups
WHERE finished_at < (NOW() - CAST(:backup_age AS INTERVAL)) AND status = 'DELETED' AND NOT EXISTS (SELECT 1 FROM dbaas.backups_dependencies WHERE parent_id = backup_id)
RETURNING 1
`,
	}

	querySelectHosts = sqlutil.Stmt{
		Name: "SelectHosts",
		// language=PostgreSQL
		Query: `
SELECT
	sc.cid,
    h.subcid,
    h.shard_id,
    h.fqdn
FROM
dbaas.subclusters sc
JOIN dbaas.hosts h
  ON h.subcid = sc.subcid
LEFT JOIN dbaas.shards sh
  ON h.shard_id = sh.shard_id
WHERE sc.cid = :cid
  AND (:subcid IS NULL OR sc.subcid = :subcid)
  AND (:shard_id IS NULL OR h.shard_id = :shard_id)
  ORDER BY fqdn
 `,
	}

	querySelectCluster = sqlutil.Stmt{
		Name: "SelectCluster",
		// language=PostgreSQL
		Query: `
SELECT
	c.cid,
	c.name,
	c.type,
    c.env
FROM
	dbaas.clusters c
WHERE
	c.cid = :cid
`,
	}

	querySelectOldestPendingBackupWithStatus = sqlutil.Stmt{
		Name: "SelectOldestPendingBackupWithStatus",
		// language=PostgreSQL
		Query: `
SELECT
	b.backup_id,
	b.cid,
	b.subcid,
	c.type as cluster_type,
	b.shard_id,
	b.scheduled_date,
    b.status,
	b.method,
    b.initiator,
	b.delayed_until,
	b.created_at,
	b.started_at,
	b.finished_at,
	b.updated_at,
	b.shipment_id,
	b.metadata,
	b.errors
FROM dbaas.backups b
JOIN dbaas.clusters c
  ON (b.cid = c.cid)
WHERE b.status = :backup_status
  AND b.delayed_until < now()
ORDER BY b.delayed_until
LIMIT 1
FOR UPDATE OF b
SKIP LOCKED
`,
	}

	queryCompleteBackupStartWithStatus = sqlutil.Stmt{
		Name: "CompleteBackupStartWithStatus",
		// language=PostgreSQL
		Query: `
UPDATE dbaas.backups
  SET started_at = now(),
      finished_at = NULL,
      updated_at = now(),
	  status = :backup_status,
      shipment_id = :shipment_id
WHERE backup_id = :backup_id
RETURNING 1
`,
	}

	queryCompleteBackupCreation = sqlutil.Stmt{
		Name: "CompleteBackupCreation",
		// language=PostgreSQL
		Query: `
UPDATE dbaas.backups
  SET finished_at = COALESCE(:finished_at, now()),
      updated_at = now(),
	  status = 'DONE',
      metadata = :metadata
WHERE backup_id = :backup_id
RETURNING 1
`,
	}

	queryCompleteBackupWithStatus = sqlutil.Stmt{
		Name: "CompleteBackupWithStatus",
		// language=PostgreSQL
		Query: `
UPDATE dbaas.backups
  SET finished_at = now(),
      updated_at = now(),
	  status = :backup_status
WHERE backup_id = :backup_id
RETURNING 1
`,
	}

	queryCompleteBackupWithStatusErrors = sqlutil.Stmt{
		Name: "CompleteBackupWithStatusErrors",
		// language=PostgreSQL
		Query: `
UPDATE dbaas.backups
  SET finished_at = now(),
      updated_at = now(),
	  status = :backup_status,
      errors = :errors
WHERE backup_id = :backup_id
RETURNING 1
`,
	}

	queryPostponePendingBackup = sqlutil.Stmt{
		Name: "PostponePendingBackup",
		// language=PostgreSQL
		Query: `
UPDATE dbaas.backups
  SET delayed_until = :delayed_until,
      errors = :errors,
      updated_at = now()
WHERE backup_id = :backup_id
RETURNING 1
`,
	}

	querySelectClusterPillar = sqlutil.Stmt{
		Name: "SelectClusterPillar",
		// language=PostgreSQL
		Query: `
SELECT
    value
FROM
    dbaas.pillar
WHERE
    cid = :cid`,
	}

	querySelectClusterStatus = sqlutil.Stmt{
		Name: "SelectClusterStatus",
		// language=PostgreSQL
		Query: `
SELECT
    status
FROM
    dbaas.clusters
WHERE
    cid = :cid`,
	}

	querySelectBackupServiceEnabled = sqlutil.Stmt{
		Name: "SelectBackupServiceEnabled",
		// language=PostgreSQL
		Query: `
SELECT
       COALESCE(CAST(schedule ->> 'use_backup_service' AS BOOLEAN), false)
FROM
     dbaas.backup_schedule
WHERE
      cid = :cid`,
	}

	querySetBackupServiceEnabled = sqlutil.Stmt{
		Name: "SetBackupServiceEnabled",
		// language=PostgreSQL
		Query: `
SELECT * FROM code.set_backup_service_use(
	i_cid => :cid,
	i_val => :val)`,
	}

	querySelectBackups = sqlutil.Stmt{
		Name: "SelectBackups",
		// language=PostgreSQL
		Query: `
SELECT
	b.backup_id,
	b.cid,
	b.subcid,
	c.type as cluster_type,
	b.shard_id,
	b.scheduled_date,
	b.status,
	b.method,
	b.initiator,
	b.delayed_until,
	b.created_at,
	b.started_at,
	b.finished_at,
	b.updated_at,
	b.shipment_id,
	b.metadata,
	b.errors
FROM dbaas.backups b
JOIN dbaas.clusters c
  ON (b.cid = c.cid)
WHERE b.cid= :cid
  AND (:subcid IS NULL OR b.subcid = :subcid)
  AND (:shard_id IS NULL OR b.shard_id = :shard_id)
  AND (:backup_statuses IS NULL OR b.status = ANY(CAST(:backup_statuses as dbaas.backup_status[])))
  AND (:backup_initiator IS NULL OR b.initiator = ANY(CAST(:backup_initiator as dbaas.backup_initiator[])))

`,
	}

	querySelectClusterShards = sqlutil.Stmt{
		Name: "queryGetShardsByClusterId",
		// language=PostgreSQL
		Query: `
SELECT
    sc.subcid as subcluster_id,
    sc.name as subcluster_name,
    s.shard_id as shard_id,
    s.name as shard_name
FROM
    dbaas.shards s
RIGHT JOIN dbaas.subclusters sc USING (subcid)
WHERE
    sc.cid = :cid
ORDER BY
    s.name`,
	}

	querySelectClusterSubclustes = sqlutil.Stmt{
		Name: "queryGetSubclustersByClusterId",
		// language=PostgreSQL
		Query: `
SELECT
    sc.cid as cluster_id,
    sc.subcid as subcluster_id,
    sc.name as subcluster_name
FROM
    dbaas.subclusters sc
WHERE
    sc.cid = :cid`,
	}

	querySelectVersionsByClusterIDAndTS = sqlutil.Stmt{
		Name: "SelectVersionsByClusterIDAndTS",
		Query:
		// language=PostgreSQL
		`
SELECT  v.component as component,
        v.major_version as major_version,
        v.minor_version as minor_version,
        v.edition as edition,
        v.package_version as package_version
FROM    dbaas.versions_revs v
WHERE   v.cid = :cid
  AND   v.rev = (SELECT max(rev) FROM dbaas.clusters_changes WHERE cid = :cid AND committed_at <= :timestamp)`,
	}

	queryAddBackupDependency = sqlutil.Stmt{
		Name: "AddBackupDependency",
		Query:
		// language=PostgreSQL
		`
INSERT INTO dbaas.backups_dependencies
	(parent_id, child_id)
VALUES
	(:parent_id, :child_id)`,
	}

	queryBackupSchedule = sqlutil.Stmt{
		Name: "BackupSchedule",
		Query:
		// language=PostgreSQL
		`
SELECT
        schedule
FROM
        dbaas.backup_schedule b
WHERE
        b.cid = :cid`,
	}

	queryListParentBackups = sqlutil.Stmt{
		Name: "ListParentBackups",
		Query:
		// language=PostgreSQL
		`
SELECT
	b.backup_id,
	b.cid,
	b.subcid,
	b.shard_id,
	b.scheduled_date,
	b.status,
	b.method,
	b.initiator,
	b.delayed_until,
	b.created_at,
	b.started_at,
	b.finished_at,
	b.updated_at,
	b.shipment_id,
	b.metadata,
	b.errors
FROM
        dbaas.backups b
JOIN
        dbaas.backups_dependencies bd
ON
        b.backup_id = bd.parent_id
WHERE
        bd.child_id = :id`,
	}

	queryGetHostPillar = sqlutil.Stmt{
		Name: "PillarQuery",
		Query:
		// language=PostgreSQL
		`
SELECT
	p.fqdn,
	CAST(p.value#>:path as text) as value
FROM
	dbaas.pillar p
WHERE
	p.fqdn = ANY(:fqdns)
			`,
	}

	querySetBackupSize = sqlutil.Stmt{
		Name: "SetBackupSize",
		// language=PostgreSQL
		Query: `
INSERT INTO dbaas.backups_history
	(backup_id, data_size, journal_size, committed_at)
VALUES
	(:backup_id, :data_size, :journal_size, :committed_at)`,
	}

	queryUpsertBackupImportHistory = sqlutil.Stmt{
		Name: "UpsertBackupImportHistory",
		// language=PostgreSQL
		Query: `
INSERT INTO dbaas.backups_import_history
	(cid, last_import_at, errors)
VALUES
	(:cid, :last_import_at, :errors)
ON CONFLICT
	(cid)
DO UPDATE SET errors = EXCLUDED.errors, last_import_at = EXCLUDED.last_import_at`,
	}

	querySelectClustersForImport = sqlutil.Stmt{
		Name: "SelectClustersForImport",
		// language=PostgreSQL
		Query: `
SELECT c.cid
FROM dbaas.clusters c
JOIN dbaas.backup_schedule as bs using (cid)
WHERE c.status = ANY(CAST(:cluster_statuses AS dbaas.cluster_status[]))
AND c.type = ANY(CAST(:cluster_types AS dbaas.cluster_type[]))
AND CAST(bs.schedule ->> 'use_backup_service' as BOOLEAN)
AND NOT EXISTS (
       SELECT 1
         FROM dbaas.backups_import_history as h
        WHERE h.cid = c.cid
          AND now() - h.last_import_at < CAST(:import_interval AS INTERVAL)
)
LIMIT :max_count`,
	}

	querySelectLockBackups = sqlutil.Stmt{
		Name: "SelectLockBackups",
		// language=PostgreSQL
		Query: `
SELECT
	b.backup_id,
	b.cid,
	b.subcid,
	c.type as cluster_type,
	b.shard_id,
	b.scheduled_date,
	b.status,
	b.method,
	b.initiator,
	b.delayed_until,
	b.created_at,
	b.started_at,
	b.finished_at,
	b.updated_at,
	b.shipment_id,
	b.metadata,
	b.errors
FROM dbaas.backups b
JOIN dbaas.clusters c
  ON (b.cid = c.cid)
WHERE b.cid= :cid
  AND (:subcid IS NULL OR b.subcid = :subcid)
  AND (:shard_id IS NULL OR b.shard_id = :shard_id)
  AND (:backup_statuses IS NULL OR b.status = ANY(CAST(:backup_statuses as dbaas.backup_status[])))
  AND (:backup_initiator IS NULL OR b.initiator = ANY(CAST(:backup_initiator as dbaas.backup_initiator[])))
  AND  b.backup_id != ALL(:except_ids)
FOR UPDATE OF b
NOWAIT
`,
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

func (mdb *metaDB) GetBackupBlanks(ctx context.Context, clusterTypes []metadb.ClusterType, past, future time.Duration) ([]metadb.BackupBlank, error) {
	var resList []metadb.BackupBlank

	parser := func(rows *sqlx.Rows) error {
		var row backupBlankRow
		if err := rows.StructScan(&row); err != nil {
			return err
		}
		resList = append(resList, formatBackupBlank(row))
		return nil
	}
	pastSQLInterval := &pgtype.Interval{}
	if err := pastSQLInterval.Set(past); err != nil {
		return nil, xerrors.Errorf("error while converting past planning interval: %w", err)
	}
	futureSQLInterval := &pgtype.Interval{}
	if err := futureSQLInterval.Set(future); err != nil {
		return nil, xerrors.Errorf("error while converting future planning interval: %w", err)
	}
	_, err := sqlutil.QueryTx(
		ctx,
		queryBackupBlanks,
		map[string]interface{}{
			"past_interval":        pastSQLInterval,
			"future_interval":      futureSQLInterval,
			"backup_delay_seconds": metadb.DefaultBackupDelaySeconds,
			"cluster_statuses":     clusterStatusesToDB(metadb.ApplicableClusterStatuses),
			"cluster_types":        clusterTypesToDB(clusterTypes),
		},
		parser,
		mdb.logger,
	)

	if err != nil {
		return nil, err
	}

	return resList, nil
}

func (mdb *metaDB) GetClustersForImport(ctx context.Context, maxCount int, clusterTypes []metadb.ClusterType, interval time.Duration) ([]string, error) {
	var cids []string
	parser := func(rows *sqlx.Rows) error {
		var cid string
		err := rows.Scan(&cid)
		if err != nil {
			return err
		}
		cids = append(cids, cid)
		return nil
	}

	importInterval := &pgtype.Interval{}
	if err := importInterval.Set(interval); err != nil {
		return nil, xerrors.Errorf("converting import interval: %w", err)
	}

	_, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		querySelectClustersForImport,
		map[string]interface{}{
			"cluster_statuses": clusterStatusesToDB(metadb.ApplicableClusterStatuses),
			"cluster_types":    clusterTypesToDB(clusterTypes),
			"max_count":        maxCount,
			"import_interval":  importInterval,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return nil, err
	}

	return cids, nil
}

func (mdb *metaDB) UpdateImportHistory(ctx context.Context, cid string, importErr error, t time.Time) error {
	queryArgs := map[string]interface{}{
		"cid":            cid,
		"last_import_at": t,
		"errors":         nil,
	}

	if importErr != nil {
		errs := FromErrors(importErr)

		errsBytes, err := errs.Marshal()
		if err != nil {
			return err
		}

		queryArgs["errors"] = string(errsBytes)
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryUpsertBackupImportHistory,
		queryArgs,
		sqlutil.NopParser,
		mdb.logger,
	)
	return err
}

func (mdb *metaDB) AddBackup(ctx context.Context, createBackupArgs metadb.CreateBackupArgs) (metadb.Backup, error) {
	var r backupRow
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&r)
	}

	scheduledAt := sql.NullTime{}
	if createBackupArgs.ScheduledAt.Valid {
		scheduledAt = sql.NullTime{Time: createBackupArgs.ScheduledAt.Time.UTC(), Valid: true}
	}

	args := map[string]interface{}{
		"backup_id":      createBackupArgs.BackupID,
		"cid":            createBackupArgs.ClusterID,
		"subcid":         createBackupArgs.SubClusterID,
		"shard_id":       sql.NullString(createBackupArgs.ShardID),
		"status":         metadb.BackupStatusPlanned,
		"method":         createBackupArgs.Method,
		"initiator":      createBackupArgs.Initiator,
		"delayed_until":  createBackupArgs.DelayedUntil,
		"scheduled_date": scheduledAt,
		"child_id":       createBackupArgs.BackupID,
	}

	if createBackupArgs.DependsOnBackupIDs != nil {
		args["parent_ids"] = createBackupArgs.DependsOnBackupIDs
	} else {
		args["parent_ids"] = []string{}
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryAddBackup,
		args,
		parser,
		mdb.logger,
	)
	if err != nil {
		return metadb.Backup{}, err
	}
	if count == 0 {
		return metadb.Backup{}, metadb.ErrDataNotFound
	}
	r.ClusterType = createBackupArgs.ClusterType
	return formatBackup(r)
}

func (mdb *metaDB) ImportBackup(ctx context.Context, args metadb.ImportBackupArgs) (metadb.Backup, error) {
	metabytes, err := args.Metadata.Marshal()
	if err != nil {
		return metadb.Backup{}, err
	}

	scheduledDate := sql.NullTime{}
	if args.ScheduledAt.Valid {
		scheduledDate = sql.NullTime{Time: args.ScheduledAt.Time.UTC(), Valid: true}
	}

	var r backupRow
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&r)
	}
	_, err = sqlutil.QueryTx(
		ctx,
		queryImportBackup,
		map[string]interface{}{
			"backup_id":      args.BackupID,
			"cid":            args.ClusterID,
			"subcid":         args.SubClusterID,
			"shard_id":       sql.NullString(args.ShardID),
			"status":         metadb.BackupStatusDone,
			"scheduled_date": scheduledDate,
			"created_at":     args.CreatedAt,
			"delayed_until":  args.DelayedUntil,
			"started_at":     args.StartedAt,
			"finished_at":    args.FinishedAt,
			"updated_at":     args.UpdatedAt,
			"metadata":       string(metabytes),
			"initiator":      args.Initiator,
			"method":         args.Method,
		},
		parser,
		mdb.logger)
	if err != nil {
		return metadb.Backup{}, xerrors.Errorf("Import backup query failed with %w", err)
	}

	for _, parentID := range args.DependsOnBackupIDs {
		err = mdb.addBackupDependency(ctx, parentID, args.BackupID)
		if err != nil {
			return metadb.Backup{}, xerrors.Errorf("Add backup dependency query failed with %w", err)
		}
	}

	return formatBackup(r)
}

func (mdb *metaDB) addBackupDependency(ctx context.Context, parentBackupID, childBackupID string) error {
	_, err := sqlutil.QueryTx(
		ctx,
		queryAddBackupDependency,
		map[string]interface{}{
			"parent_id": parentBackupID,
			"child_id":  childBackupID,
		},
		sqlutil.NopParser,
		mdb.logger,
	)
	return err
}
func (mdb *metaDB) ListParentBackups(ctx context.Context, backupID string) ([]metadb.Backup, error) {
	var bs []metadb.Backup

	parser := func(rows *sqlx.Rows) error {
		var r backupRow
		if err := rows.StructScan(&r); err != nil {
			return err
		}
		backup, err := formatBackup(r)
		if err != nil {
			return err
		}
		bs = append(bs, backup)
		return nil
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryListParentBackups,
		map[string]interface{}{
			"id": backupID,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return nil, err
	}
	return bs, nil
}

func (mdb *metaDB) BackupSchedule(ctx context.Context, cid string) ([]byte, error) {

	var sched []byte

	schedParser := func(rows *sqlx.Rows) error {
		if err := rows.Scan(&sched); err != nil {
			return err
		}
		return nil
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryBackupSchedule,
		map[string]interface{}{
			"cid": cid,
		},
		schedParser,
		mdb.logger,
	)
	if err != nil {
		return []byte{}, err
	}

	return sched, nil
}

func (mdb *metaDB) ObsoleteAutomatedBackups(ctx context.Context, clusterTypes []metadb.ClusterType) (int64, error) {
	return sqlutil.QueryTx(
		ctx,
		queryObsoleteAutomatedBackups,
		map[string]interface{}{
			"cluster_statuses": clusterStatusesToDB(metadb.ApplicableClusterStatuses),
			"cluster_types":    clusterTypesToDB(clusterTypes),
		},
		sqlutil.NopParser,
		mdb.logger,
	)
}

func (mdb *metaDB) ObsoleteFailedBackups(ctx context.Context, clusterTypes []metadb.ClusterType, backupAge time.Duration) (int64, error) {

	backupAgeInterval := &pgtype.Interval{Status: pgtype.Null}
	if err := backupAgeInterval.Set(backupAge); err != nil {
		return 0, xerrors.Errorf("error while converting backup age interval: %w", err)
	}

	return sqlutil.QueryTx(
		ctx,
		queryObsoleteFailedBackups,
		map[string]interface{}{
			"backup_age":       backupAgeInterval,
			"cluster_statuses": clusterStatusesToDB(metadb.ApplicableClusterStatuses),
			"cluster_types":    clusterTypesToDB(clusterTypes),
		},
		sqlutil.NopParser,
		mdb.logger,
	)
}

func (mdb *metaDB) SequentialObsoleteAutomatedBackups(ctx context.Context, clusterTypes []metadb.ClusterType) (int64, error) {
	return sqlutil.QueryTx(
		ctx,
		querySequentialObsoleteAutomatedBackups,
		map[string]interface{}{
			"cluster_statuses": clusterStatusesToDB(metadb.ApplicableClusterStatuses),
			"cluster_types":    clusterTypesToDB(clusterTypes),
		},
		sqlutil.NopParser,
		mdb.logger,
	)
}

func (mdb *metaDB) SequentialObsoleteFailedBackups(ctx context.Context, clusterTypes []metadb.ClusterType, backupAge time.Duration) (int64, error) {

	backupAgeInterval := &pgtype.Interval{Status: pgtype.Null}
	if err := backupAgeInterval.Set(backupAge); err != nil {
		return 0, xerrors.Errorf("error while converting backup age interval: %w", err)
	}

	return sqlutil.QueryTx(
		ctx,
		querySequentialObsoleteFailedBackups,
		map[string]interface{}{
			"backup_age":       backupAgeInterval,
			"cluster_statuses": clusterStatusesToDB(metadb.ApplicableClusterStatuses),
			"cluster_types":    clusterTypesToDB(clusterTypes),
		},
		sqlutil.NopParser,
		mdb.logger,
	)
}

func (mdb *metaDB) PurgeDeletedBackups(ctx context.Context, clusterTypes []metadb.ClusterType, backupAge time.Duration) (int64, error) {
	backupAgeInterval := &pgtype.Interval{Status: pgtype.Null}
	if err := backupAgeInterval.Set(backupAge); err != nil {
		return 0, xerrors.Errorf("error while converting backup age interval: %w", err)
	}

	return sqlutil.QueryTx(
		ctx,
		queryPurgeBackups,
		map[string]interface{}{
			"backup_age":    backupAgeInterval,
			"cluster_types": clusterTypesToDB(clusterTypes),
		},
		sqlutil.NopParser,
		mdb.logger,
	)
}

func (mdb *metaDB) ListHosts(ctx context.Context, clusterID string, subClusterID, shardID optional.String) ([]metadb.Host, error) {
	var dbHosts []host
	parser := func(rows *sqlx.Rows) error {
		var h host
		err := rows.StructScan(&h)
		if err != nil {
			return err
		}
		dbHosts = append(dbHosts, h)
		return nil
	}

	_, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		querySelectHosts,
		map[string]interface{}{
			"cid":      clusterID,
			"subcid":   sql.NullString(subClusterID),
			"shard_id": sql.NullString(shardID),
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return nil, err
	}

	hosts := make([]metadb.Host, len(dbHosts))
	for i := range dbHosts {
		host, err := hostFromDB(dbHosts[i])
		if err != nil {
			return nil, err
		}
		hosts[i] = host
	}

	return hosts, nil
}

func (mdb *metaDB) HostsPillarByPath(ctx context.Context, fqdns []string, path []string) (map[string]string, error) {
	hostPillars := make(map[string]string)
	parser := func(rows *sqlx.Rows) error {
		var h hostPillar
		err := rows.StructScan(&h)
		if err != nil {
			return err
		}

		if h.Value.Valid {
			hostPillars[h.FQDN] = h.Value.String
		}
		return nil
	}

	var dbPath pgtype.TextArray
	if err := dbPath.Set(path); err != nil {
		return nil, err
	}

	_, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		queryGetHostPillar,
		map[string]interface{}{
			"fqdns": fqdns,
			"path":  dbPath,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return nil, err
	}

	return hostPillars, nil
}

func (mdb *metaDB) BackupServiceEnabled(ctx context.Context, clusterID string) (bool, error) {
	var enabled bool
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&enabled)
	}

	count, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		querySelectBackupServiceEnabled,
		map[string]interface{}{
			"cid": clusterID,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return false, err
	}
	if count == 0 {
		return false, metadb.ErrDataNotFound
	}

	return enabled, nil
}

func (mdb *metaDB) SetBackupServiceEnabled(ctx context.Context, clusterID string, enabled bool) error {
	count, err := sqlutil.QueryTx(
		ctx,
		querySetBackupServiceEnabled,
		map[string]interface{}{
			"cid": clusterID,
			"val": enabled,
		},
		sqlutil.NopParser,
		mdb.logger,
	)
	if err != nil {
		return err
	}

	if count == 0 {
		return metadb.ErrDataNotFound
	}

	return nil
}

func (mdb *metaDB) ListBackups(ctx context.Context, clusterID string, subClusterID optional.String, shardID optional.String, statuses []metadb.BackupStatus, initiators []metadb.BackupInitiator) ([]metadb.Backup, error) {
	var resList []metadb.Backup

	parser := func(rows *sqlx.Rows) error {
		var row backupRow
		if err := rows.StructScan(&row); err != nil {
			return err
		}
		backup, err := formatBackup(row)
		if err != nil {
			return err
		}
		resList = append(resList, backup)
		return nil
	}

	_, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		querySelectBackups,
		map[string]interface{}{
			"cid":              clusterID,
			"subcid":           sql.NullString(subClusterID),
			"shard_id":         sql.NullString(shardID),
			"backup_statuses":  sqlStatusesFromModel(statuses),
			"backup_initiator": sqlInitiatorsFromModel(initiators),
		},
		parser,
		mdb.logger,
	)

	if err != nil {
		return nil, err
	}

	return resList, nil
}

func (mdb *metaDB) ListShards(ctx context.Context, cid string) ([]metadb.Shard, error) {
	var shards []metadb.Shard
	parser := func(rows *sqlx.Rows) error {
		var s shard
		if err := rows.StructScan(&s); err != nil {
			return err
		}
		shards = append(shards, shardFromDB(s))
		return nil
	}

	count, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		querySelectClusterShards,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return []metadb.Shard{}, err
	}
	if count == 0 {
		return nil, metadb.ErrDataNotFound
	}

	return shards, nil
}

func (mdb *metaDB) ListSubClusters(ctx context.Context, cid string) ([]metadb.SubCluster, error) {
	var subclusters []metadb.SubCluster
	parser := func(rows *sqlx.Rows) error {
		var s subcluster
		if err := rows.StructScan(&s); err != nil {
			return err
		}
		subclusters = append(subclusters, subclusterFromDB(s))
		return nil
	}

	count, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		querySelectClusterSubclustes,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return []metadb.SubCluster{}, err
	}
	if count == 0 {
		return nil, metadb.ErrDataNotFound
	}

	return subclusters, nil
}

func (mdb *metaDB) Cluster(ctx context.Context, cid string) (metadb.Cluster, error) {
	var c cluster
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&c)
	}

	count, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		querySelectCluster,
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

	return clusterFromDB(c), nil
}

func (mdb *metaDB) oldestPendingBackupWithStatus(ctx context.Context, status metadb.BackupStatus) (metadb.Backup, error) {
	var row backupRow
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&row)
	}
	count, err := sqlutil.QueryTx(
		ctx,
		querySelectOldestPendingBackupWithStatus,
		map[string]interface{}{
			"backup_status": status,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return metadb.Backup{}, err
	}
	if count == 0 {
		return metadb.Backup{}, metadb.ErrDataNotFound
	}
	return formatBackup(row)
}

func (mdb *metaDB) completeBackupStartWithStatus(ctx context.Context, backupID, shipmentID string, status metadb.BackupStatus) error {
	count, err := sqlutil.QueryTx(
		ctx,
		queryCompleteBackupStartWithStatus,
		map[string]interface{}{
			"backup_id":     backupID,
			"shipment_id":   shipmentID,
			"backup_status": status,
		},
		sqlutil.NopParser,
		mdb.logger,
	)
	if err != nil {
		return err
	}
	if count == 0 {
		return metadb.ErrDataNotFound
	}
	return nil
}

func (mdb *metaDB) updateBackupStatus(ctx context.Context, backupID string, status metadb.BackupStatus) error {
	count, err := sqlutil.QueryTx(
		ctx,
		queryCompleteBackupWithStatus,
		map[string]interface{}{
			"backup_id":     backupID,
			"backup_status": status,
		},
		sqlutil.NopParser,
		mdb.logger,
	)
	if err != nil {
		return err
	}
	if count == 0 {
		return metadb.ErrDataNotFound
	}
	return nil
}

func (mdb *metaDB) updateBackupStatusWithErrors(ctx context.Context, backupID string, status metadb.BackupStatus, errs metadb.Errors) error {
	if errs == nil {
		return xerrors.Errorf("errs should not be nil ")
	}

	errsbytes, err := errs.Marshal()
	if err != nil {
		return err
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryCompleteBackupWithStatusErrors,
		map[string]interface{}{
			"backup_id":     backupID,
			"backup_status": status,
			"errors":        string(errsbytes),
		},
		sqlutil.NopParser,
		mdb.logger,
	)
	if err != nil {
		return err
	}
	if count == 0 {
		return metadb.ErrDataNotFound
	}
	return nil
}

func (mdb *metaDB) PlannedBackup(ctx context.Context) (metadb.Backup, error) {
	return mdb.oldestPendingBackupWithStatus(ctx, metadb.BackupStatusPlanned)
}

func (mdb *metaDB) CompleteBackupCreationStart(ctx context.Context, backupID, shipmentID string) error {
	return mdb.completeBackupStartWithStatus(ctx, backupID, shipmentID, metadb.BackupStatusCreating)
}

func (mdb *metaDB) DelayPendingBackupUntil(ctx context.Context, backupID string, t time.Time, errs metadb.Errors) error {
	if errs == nil {
		return xerrors.Errorf("errs should not be nil ")
	}

	errsbytes, err := errs.Marshal()
	if err != nil {
		return err
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryPostponePendingBackup,
		map[string]interface{}{
			"backup_id":     backupID,
			"delayed_until": t,
			"errors":        string(errsbytes),
		},
		sqlutil.NopParser,
		mdb.logger,
	)
	if err != nil {
		return err
	}
	if count == 0 {
		return metadb.ErrDataNotFound
	}

	return nil
}

func (mdb *metaDB) FailBackupCreation(ctx context.Context, backupID string, errs metadb.Errors) error {
	return mdb.updateBackupStatusWithErrors(ctx, backupID, metadb.BackupStatusCreateError, errs)
}

func (mdb *metaDB) CreatingBackup(ctx context.Context) (metadb.Backup, error) {
	return mdb.oldestPendingBackupWithStatus(ctx, metadb.BackupStatusCreating)
}

func (mdb *metaDB) CompleteBackupCreation(ctx context.Context, backupID string, finishTime optional.Time, metadata metadb.BackupMetadata) error {
	metabytes, err := metadata.Marshal()
	if err != nil {
		return err
	}

	finishedAt := sql.NullTime{}
	if finishTime.Valid {
		finishedAt = sql.NullTime{Time: finishTime.Time.UTC(), Valid: true}
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryCompleteBackupCreation,
		map[string]interface{}{
			"backup_id":   backupID,
			"finished_at": finishedAt,
			"metadata":    string(metabytes),
		},
		sqlutil.NopParser,
		mdb.logger,
	)
	if err != nil {
		return err
	}
	if count == 0 {
		return metadb.ErrDataNotFound
	}

	return nil
}

func (mdb *metaDB) ObsoleteBackup(ctx context.Context) (metadb.Backup, error) {
	return mdb.oldestPendingBackupWithStatus(ctx, metadb.BackupStatusObsolete)
}

func (mdb *metaDB) CompleteBackupDeletionStart(ctx context.Context, backupID, shipmentID string) error {
	return mdb.completeBackupStartWithStatus(ctx, backupID, shipmentID, metadb.BackupStatusDeleting)
}

func (mdb *metaDB) DeletingBackup(ctx context.Context) (metadb.Backup, error) {
	return mdb.oldestPendingBackupWithStatus(ctx, metadb.BackupStatusDeleting)
}

func (mdb *metaDB) CompleteBackupDeletion(ctx context.Context, backupID string) error {
	return mdb.updateBackupStatus(ctx, backupID, metadb.BackupStatusDeleted)
}

func (mdb *metaDB) FailBackupDeletion(ctx context.Context, backupID string, errs metadb.Errors) error {
	return mdb.updateBackupStatusWithErrors(ctx, backupID, metadb.BackupStatusDeleteError, errs)
}

func (mdb *metaDB) ClusterBucket(ctx context.Context, clusterID string) (string, error) {
	var pillar json.RawMessage
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&pillar)
	}

	_, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		querySelectClusterPillar,
		map[string]interface{}{
			"cid": clusterID,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return "", err
	}

	return UnmarshalS3Bucket(pillar)
}

func (mdb *metaDB) ClusterStatusIsIn(ctx context.Context, clusterID string, statuses []metadb.ClusterStatus) (bool, error) {
	var status metadb.ClusterStatus
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&status)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectClusterStatus,
		map[string]interface{}{
			"cid": clusterID,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return false, err
	}

	if count == 0 {
		return false, metadb.ErrDataNotFound
	}

	for i := range statuses {
		if status == statuses[i] {
			return true, nil
		}
	}
	return false, nil
}

func (mdb *metaDB) ClusterVersions(ctx context.Context, clusterID string) (map[string]metadb.ComponentVersion, error) {
	return mdb.ClusterVersionsAtTS(ctx, clusterID, time.Now())
}

func (mdb *metaDB) ClusterVersionsAtTS(ctx context.Context, clusterID string, ts time.Time) (map[string]metadb.ComponentVersion, error) {
	versionByComponent := make(map[string]metadb.ComponentVersion)
	parser := func(rows *sqlx.Rows) error {
		var v versionRow
		if err := rows.StructScan(&v); err != nil {
			return err
		}
		versionByComponent[v.Component] = componentVersionFromDB(v)
		return nil
	}

	count, err := sqlutil.QueryNode(
		ctx,
		mdb.cluster.Primary(),
		querySelectVersionsByClusterIDAndTS,
		map[string]interface{}{
			"cid":       clusterID,
			"timestamp": ts,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return nil, err
	}
	if count == 0 {
		return nil, metadb.ErrDataNotFound
	}

	return versionByComponent, nil
}

func (mdb *metaDB) SetBackupSize(ctx context.Context, backupID string, data, journal int64, t time.Time) error {
	_, err := sqlutil.QueryTx(
		ctx,
		querySetBackupSize,
		map[string]interface{}{
			"backup_id":    backupID,
			"data_size":    data,
			"journal_size": journal,
			"committed_at": t,
		},
		sqlutil.NopParser,
		mdb.logger,
	)
	return err
}

func (mdb *metaDB) LockBackups(
	ctx context.Context,
	clusterID string,
	subClusterID optional.String,
	shardID optional.String,
	statuses []metadb.BackupStatus,
	initiators []metadb.BackupInitiator,
	exceptIDS []string,
) ([]metadb.Backup, error) {
	var resList []metadb.Backup

	parser := func(rows *sqlx.Rows) error {
		var row backupRow
		if err := rows.StructScan(&row); err != nil {
			return err
		}
		backup, err := formatBackup(row)
		if err != nil {
			return err
		}
		resList = append(resList, backup)
		return nil
	}

	_, err := sqlutil.QueryTx(
		ctx,
		querySelectLockBackups,
		map[string]interface{}{
			"cid":              clusterID,
			"subcid":           sql.NullString(subClusterID),
			"shard_id":         sql.NullString(shardID),
			"backup_statuses":  sqlStatusesFromModel(statuses),
			"backup_initiator": sqlInitiatorsFromModel(initiators),
			"except_ids":       exceptIDS,
		},
		parser,
		mdb.logger,
	)

	if err != nil {
		var pgErr *pgconn.PgError
		if xerrors.As(err, &pgErr) {
			if pgErr.Code == pgerrcode.LockNotAvailable {
				return nil, metadb.CanNotLockBackups
			}
		}
		return nil, err
	}

	return resList, nil
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
