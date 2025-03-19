package pg

import (
	"context"
	"database/sql"
	"time"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

const DBName = "katandb"

var (
	queryClustersByQuery = sqlutil.NewStmt(
		"ClustersByQuery",
		// language=PostgreSQL
		"SELECT * FROM katan.clusters WHERE tags @> :query",
		clusterRow{})
	queryAddHost = sqlutil.Stmt{
		Name: "AddHost",
		// language=PostgreSQL
		Query: "INSERT INTO katan.hosts (fqdn, tags, cluster_id) VALUES (:fqdn, :tags, :cluster_id)",
	}
	queryAddCluster = sqlutil.Stmt{
		Name: "AddCluster",
		// language=PostgreSQL
		Query: "INSERT INTO katan.clusters (cluster_id, tags) VALUES (:cluster_id, :tags)",
	}
	queryUpdateHostTags = sqlutil.Stmt{
		Name: "UpdateHostTags",
		// language=PostgreSQL
		Query: "UPDATE katan.hosts SET tags = :tags WHERE fqdn = :fqdn",
	}
	queryUpdateClusterTags = sqlutil.Stmt{
		Name: "UpdateClusterTags",
		// language=PostgreSQL
		Query: "UPDATE katan.clusters SET tags = :tags WHERE cluster_id=:cluster_id",
	}
	queryDeleteHosts = sqlutil.Stmt{
		Name: "DeleteHosts",
		// language=PostgreSQL
		Query: "DELETE FROM katan.hosts WHERE fqdn = ANY(:fqdns)",
	}
	queryDeleteCluster = sqlutil.Stmt{
		Name: "DeleteCluster",
		// language=PostgreSQL
		Query: "DELETE FROM katan.clusters WHERE cluster_id = :cluster_id",
	}
	queryAutoUpdatedClusterIDsByQuery = sqlutil.Stmt{
		Name: "AutoUpdatedClustersByQuery",
		// language=PostgreSQL
		Query: "SELECT cluster_id FROM katan.clusters WHERE tags @> :query AND auto_update = true",
	}
	queryAutoUpdatedClusterIDsBySchedule = sqlutil.Stmt{
		Name: "AutoUpdatedClusterIDsBySchedule",
		// language=PostgreSQL
		Query: `
SELECT cluster_id
  FROM katan.clusters c
 WHERE tags @> :query
   AND auto_update = true
   AND (now() - imported_at) >= :import_cooldown
   AND NOT EXISTS (
       SELECT 1
         FROM katan.cluster_rollouts cr
         JOIN katan.rollouts rr
        USING (rollout_id)
		WHERE c.cluster_id = cr.cluster_id
		  AND rr.schedule_id = :schedule_id
          AND cr.state IN ('skipped', 'failed')
       	  AND (now() - cr.updated_at) < :still_age)
   AND NOT EXISTS (
       SELECT 1
         FROM katan.cluster_rollouts cr
         JOIN katan.rollouts rr
        USING (rollout_id)
		WHERE c.cluster_id = cr.cluster_id
		  AND rr.schedule_id = :schedule_id
          AND cr.state = 'succeeded'
       	  AND (now() - cr.updated_at) < :age)
FETCH FIRST :limit ROWS ONLY`,
	}
	queryClusterHosts = sqlutil.NewStmt(
		"ClusterHosts",
		// language=PostgreSQL
		"SELECT * FROM katan.hosts WHERE cluster_id = :cluster_id",
		hostRow{},
	)
	queryAddRollout = sqlutil.Stmt{
		Name: "AddRollout",
		// language=PostgreSQL
		Query: `
WITH no_concurrent_add_rollout AS (
    SELECT pg_advisory_xact_lock(100)),
	new_roll AS (
	INSERT INTO katan.rollouts
	    (commands, created_by, parallel, schedule_id, options)
	VALUES
	    (:commands, :created_by, :parallel, :schedule_id, :options)
	RETURNING rollout_id, commands, created_at, parallel, created_by, schedule_id, finished_at),
	 new_clusters_rolls AS (
	INSERT INTO katan.cluster_rollouts
	    (rollout_id, cluster_id)
	SELECT rollout_id, cluster_id
	FROM new_roll,
	     unnest(CAST(:ids AS text[])) AS cluster_id
	RETURNING cluster_id),
     new_deps AS (
    INSERT INTO katan.rollouts_dependencies
         (rollout_id, depends_on_id)
    SELECT new_roll.rollout_id, depends_on_id
      FROM new_roll,
           (SELECT DISTINCT rollout_id AS depends_on_id
              FROM katan.rollouts
           	  JOIN katan.cluster_rollouts
             USING (rollout_id)
             WHERE rollouts.finished_at IS NULL
               AND cluster_rollouts.cluster_id IN (
                   SELECT cluster_id
                     FROM new_clusters_rolls
               )
           ) deps_rolls
     )
SELECT * FROM new_roll`,
	}
	queryRollout = sqlutil.NewStmt(
		"Rollout",
		// language=PostgreSQL
		"SELECT * FROM katan.rollouts WHERE rollout_id = :rollout_id",
		rolloutRow{},
	)
	queryStartPendingRollout = sqlutil.Stmt{
		Name: "PendingRollouts",
		// sadly, but can't use sqlutil.NewStmt here,
		// cause it wraps query in `SELECT cols FROM (..)`
		// and it is invalid syntax with `UPDATE` and `CTE`
		// Specify columns explicitly.
		// language=PostgreSQL
		Query: `
UPDATE katan.rollouts
   SET started_at = now(),
       rolled_by = :rolled_by
 WHERE rollout_id IN (
    SELECT rollout_id
	  FROM katan.rollouts pending
	 WHERE pending.started_at IS NULL
	   AND NOT EXISTS (
           SELECT 1
             FROM katan.rollouts_dependencies dependencies
             JOIN katan.rollouts running
               ON (dependencies.depends_on_id = running.rollout_id)
            WHERE dependencies.rollout_id = pending.rollout_id
              AND running.finished_at IS NULL)
     FETCH FIRST ROW ONLY)
RETURNING rollout_id,
          commands,
          created_at,
          parallel,
          finished_at,
          created_by,
          schedule_id,
          comment,
    	  options
`,
	}
	queryUnfinishedRollouts = sqlutil.NewStmt(
		"UnfinishedRollouts",
		// language=PostgreSQL
		`
SELECT *
  FROM katan.rollouts
 WHERE rolled_by = :rolled_by
   AND finished_at IS NULL
`,
		rolloutRow{},
	)
	queryLastRolloutsBySchedule = sqlutil.NewStmt(
		"LastRolloutsBySchedule",
		// language=PostgreSQL
		`
SELECT *
  FROM katan.rollouts
 WHERE schedule_id = :schedule_id
   AND created_at > :since 
 ORDER BY created_at DESC
 FETCH FIRST :limit ROWS ONLY
`,
		rolloutRow{},
	)
	queryRolloutClusters = sqlutil.NewStmt(
		"RolloutClusters",
		// language=PostgreSQL
		`
SELECT *
  FROM katan.clusters
 WHERE cluster_id IN (
     SELECT cluster_id
       FROM katan.cluster_rollouts
      WHERE rollout_id = :rollout_id
 )`,
		clusterRow{},
	)
	queryRolloutShipmentsByCluster = sqlutil.Stmt{
		Name: "RolloutShipmentsByCluster",
		// language=PostgreSQL
		Query: `
SELECT shipment_id, array_agg(fqdn)
  FROM katan.rollout_shipments rs
 WHERE rollout_id = :rollout_id
   AND fqdn IN (
       SELECT fqdn
         FROM katan.hosts
        WHERE cluster_id = :cluster_id
   )
GROUP BY shipment_id
`,
	}
	queryRolloutHosts = sqlutil.NewStmt(
		"RolloutHosts",
		// language=PostgreSQL
		`
SELECT *
  FROM katan.hosts
 WHERE cluster_id IN (
     SELECT cluster_id
       FROM katan.cluster_rollouts
      WHERE rollout_id = :rollout_id
 )`,
		hostRow{},
	)
	queryClusterRollouts = sqlutil.NewStmt(
		"ClusterRollouts",
		// language=PostgreSQL
		"SELECT * FROM katan.cluster_rollouts WHERE rollout_id  = :rollout_id ORDER BY updated_at",
		clusterRolloutRow{},
	)
	queryMarkClusterRollout = sqlutil.Stmt{
		Name: "MarkHostsRollout",
		// language=PostgreSQL
		Query: `
UPDATE katan.cluster_rollouts
   SET state = :state,
       comment = :comment,
       updated_at=now()
WHERE rollout_id = :rollout_id
  AND cluster_id = :cluster_id`,
	}
	queryTouchClusterRollout = sqlutil.Stmt{
		Name: "TouchClusterRollout",
		// language=PostgreSQL
		Query: `
UPDATE katan.cluster_rollouts
   SET updated_at=now()
WHERE rollout_id = :rollout_id
  AND cluster_id = :cluster_id`,
	}
	queryFinishRollout = sqlutil.Stmt{
		Name: "FinishRollout",
		// language=PostgreSQL
		Query: `
WITH skip_pending AS (
    UPDATE katan.cluster_rollouts
       SET state = 'skipped',
           comment = 'rollout finished before it started',
           updated_at = now()
     WHERE rollout_id = :rollout_id
       AND state = 'pending'
), cancel_running AS (
    UPDATE katan.cluster_rollouts
       SET state = 'cancelled',
           comment = 'rollout finished while it running',
           updated_at = now()
     WHERE rollout_id = :rollout_id
       AND state = 'running'
)
UPDATE katan.rollouts SET finished_at = now(), comment=:comment WHERE rollout_id = :rollout_id`,
	}
	queryAddRolloutShipment = sqlutil.Stmt{
		Name: "AddRolloutChange",
		// language=PostgreSQL
		Query: `
INSERT INTO katan.rollout_shipments (rollout_id, fqdn, shipment_id)
SELECT :rollout_id, fqdn, :shipment_id
  FROM unnest(CAST(:fqdns AS text[])) AS fqdn`,
	}

	querySchedules = sqlutil.NewStmt(
		"Schedules",
		// language=PostgreSQL
		`
SELECT s.*,
       ARRAY(SELECT depends_on_id
       		   FROM katan.schedule_dependencies d
              WHERE d.schedule_id=s.schedule_id) AS depends_on
 FROM katan.schedules s`,
		scheduleRow{},
	)

	queryScheduleFails = sqlutil.NewStmt(
		"ScheduleFails",
		// language=PostgreSQL
		`
SELECT * FROM katan.cluster_rollouts
 WHERE (rollout_id, cluster_id) IN (
     SELECT rollout_id, cluster_id
       FROM katan.schedule_fails
      WHERE schedule_id = :schedule_id
 )`,
		clusterRolloutRow{},
	)

	queryDelScheduleFails = sqlutil.Stmt{
		Name: "DelScheduleFails",
		// language=PostgreSQL
		Query: "DELETE FROM katan.schedule_fails WHERE schedule_id=:schedule_id",
	}

	queryAddScheduleFail = sqlutil.Stmt{
		Name: "AddScheduleFail",
		// language=PostgreSQL
		Query: "INSERT INTO katan.schedule_fails (schedule_id, rollout_id, cluster_id) VALUES (:schedule_id, :rollout_id, :cluster_id)",
	}
	queryMarkSchedule = sqlutil.Stmt{
		Name: "MarkSchedule",
		// language=PostgreSQL
		Query: "UPDATE katan.schedules SET state = :state, examined_rollout_id=:examined_rollout_id WHERE schedule_id = :schedule_id",
	}

	queryOldestRunningRollout = sqlutil.NewStmt(
		"OldestRunningRollout",
		// language=PostgreSQL
		`
SELECT *
  FROM katan.rollouts r,
		LATERAL (
		    SELECT updated_at AS last_updated_at,
		           cluster_id AS last_updated_at_cluster
		      FROM katan.cluster_rollouts c
		     WHERE c.rollout_id = r.rollout_id
		     ORDER BY updated_at DESC
		     FETCH FIRST ROW ONLY
		) l
 WHERE started_at IS NOT NULL
   AND finished_at IS NULL`,
		rolloutDatesRow{},
	)
)

// katanDB implements KatanDB interface for PostgreSQL
type katanDB struct {
	logger  log.Logger
	cluster *sqlutil.Cluster
}

func New(cfg pgutil.Config, logger log.Logger) (katandb.KatanDB, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(logger)))
	if err != nil {
		return nil, err
	}
	return NewWithCluster(cluster, logger), nil
}

func NewWithCluster(cluster *sqlutil.Cluster, logger log.Logger) katandb.KatanDB {
	return &katanDB{
		logger:  logger,
		cluster: cluster,
	}
}

func (kdb *katanDB) Close() error {
	return kdb.cluster.Close()
}

func (kdb *katanDB) IsReady(context.Context) error {
	if kdb.cluster.Primary() == nil {
		return semerr.Unavailable("unavailable")
	}
	return nil
}

func (kdb *katanDB) query(ctx context.Context, chooser sqlutil.NodeChooser, query sqlutil.Stmt, params map[string]interface{}, parser sqlutil.RowParser) error {
	_, err := sqlutil.QueryContext(
		ctx, chooser, query, params, parser, kdb.logger,
	)

	if err != nil {
		return xerrors.Errorf("%s query failed with: %w", query.Name, err)
	}
	return nil
}

func (kdb *katanDB) queryMaster(ctx context.Context, query sqlutil.Stmt, params map[string]interface{}, parser sqlutil.RowParser) error {
	return kdb.query(ctx, kdb.cluster.PrimaryChooser(), query, params, parser)
}

func (kdb *katanDB) queryAlive(ctx context.Context, query sqlutil.Stmt, params map[string]interface{}, parser sqlutil.RowParser) error {
	return kdb.query(ctx, kdb.cluster.AliveChooser(), query, params, parser)
}

func (kdb *katanDB) ClustersByTagsQuery(ctx context.Context, query string) ([]katandb.Cluster, error) {
	var hp clustersParser

	if err := kdb.queryMaster(ctx, queryClustersByQuery, map[string]interface{}{"query": query}, hp.parse); err != nil {
		return nil, err
	}

	return hp.ret, nil
}

func (kdb *katanDB) AutoUpdatedClustersIDsByQuery(ctx context.Context, query string) ([]string, error) {
	var parser listOfStringParser

	if err := kdb.queryMaster(
		ctx,
		queryAutoUpdatedClusterIDsByQuery,
		map[string]interface{}{"query": query},
		parser.parse,
	); err != nil {
		return nil, err
	}

	return parser.ret, nil
}

func (kdb *katanDB) AutoUpdatedClustersBySchedule(
	ctx context.Context, query string, scheduleID int64, age, stillAge, importCooldown time.Duration, limit int64) ([]string, error) {

	var pgAge pgtype.Interval
	if err := pgAge.Set(age); err != nil {
		return nil, xerrors.Errorf("fail to build age var: %w", err)
	}
	var pgStillAge pgtype.Interval
	if err := pgStillAge.Set(stillAge); err != nil {
		return nil, xerrors.Errorf("fail to build still_age var: %w", err)
	}
	var pgImportCooldown pgtype.Interval
	if err := pgImportCooldown.Set(importCooldown); err != nil {
		return nil, xerrors.Errorf("fail to build import_cooldown_age var: %w", err)
	}
	var parser listOfStringParser

	if err := kdb.queryMaster(
		ctx,
		queryAutoUpdatedClusterIDsBySchedule,
		map[string]interface{}{
			"query":           query,
			"schedule_id":     scheduleID,
			"age":             pgAge,
			"still_age":       pgStillAge,
			"import_cooldown": pgImportCooldown,
			"limit":           limit,
		},
		parser.parse,
	); err != nil {
		return nil, err
	}

	return parser.ret, nil
}

func (kdb *katanDB) AddHost(ctx context.Context, host katandb.Host) error {
	params := map[string]interface{}{
		"fqdn":       host.FQDN,
		"tags":       host.Tags,
		"cluster_id": host.ClusterID,
	}
	return kdb.queryMaster(ctx, queryAddHost, params, sqlutil.NopParser)
}

func (kdb *katanDB) AddCluster(ctx context.Context, cluster katandb.Cluster) error {
	params := map[string]interface{}{
		"cluster_id": cluster.ID,
		"tags":       cluster.Tags,
	}
	return kdb.queryMaster(ctx, queryAddCluster, params, sqlutil.NopParser)
}

func (kdb *katanDB) DeleteHosts(ctx context.Context, fqdns []string) error {
	var fqdnsArr pgtype.TextArray
	if err := fqdnsArr.Set(fqdns); err != nil {
		return xerrors.Errorf("fail to initialize fqdns var: %w", err)
	}
	return kdb.queryMaster(ctx, queryDeleteHosts, map[string]interface{}{"fqdns": fqdnsArr}, sqlutil.NopParser)
}

func (kdb *katanDB) DeleteCluster(ctx context.Context, id string) error {
	return kdb.queryMaster(ctx, queryDeleteCluster, map[string]interface{}{"cluster_id": id}, sqlutil.NopParser)
}

func (kdb *katanDB) UpdateHostTags(ctx context.Context, fqdn, tags string) error {
	params := map[string]interface{}{
		"fqdn": fqdn,
		"tags": tags,
	}
	return kdb.queryMaster(ctx, queryUpdateHostTags, params, sqlutil.NopParser)
}

func (kdb *katanDB) UpdateClusterTags(ctx context.Context, id, tags string) error {
	params := map[string]interface{}{
		"cluster_id": id,
		"tags":       tags,
	}
	return kdb.queryMaster(ctx, queryUpdateClusterTags, params, sqlutil.NopParser)
}

func toSQLNullableInt(val optional.Int64) sql.NullInt64 {
	var sqlVal sql.NullInt64
	if val.Valid {
		sqlVal.Valid = true
		sqlVal.Int64 = val.Int64
	}
	return sqlVal
}

func (kdb *katanDB) AddRollout(ctx context.Context, commands, options, createdBy string, parallel int32, scheduleID optional.Int64, ids []string) (katandb.Rollout, error) {
	var row rolloutRow
	var idsArr pgtype.TextArray
	if err := idsArr.Set(ids); err != nil {
		return katandb.Rollout{}, xerrors.Errorf("unable to initialize ids var: %w", err)
	}
	params := map[string]interface{}{
		"commands":    commands,
		"options":     options,
		"created_by":  createdBy,
		"schedule_id": toSQLNullableInt(scheduleID),
		"parallel":    parallel,
		"ids":         idsArr,
	}

	if err := kdb.queryMaster(ctx, queryAddRollout, params, func(rows *sqlx.Rows) error {
		return rows.StructScan(&row)
	}); err != nil {
		return katandb.Rollout{}, err
	}

	return row.render(), nil
}

func (kdb *katanDB) queryRollout(ctx context.Context, query sqlutil.Stmt, params map[string]interface{}) (katandb.Rollout, error) {
	var row rolloutRow

	count, err := sqlutil.QueryContext(
		ctx, kdb.cluster.PrimaryChooser(), query, params,
		func(rows *sqlx.Rows) error {
			return rows.StructScan(&row)
		},
		kdb.logger,
	)
	if err != nil {
		return katandb.Rollout{}, xerrors.Errorf("fail to %s: %w", query.Name, err)
	}
	if count == 0 {
		return katandb.Rollout{}, katandb.ErrNoDataFound
	}

	return row.render(), nil
}

func (kdb *katanDB) Rollout(ctx context.Context, id int64) (katandb.Rollout, error) {
	return kdb.queryRollout(ctx, queryRollout,
		map[string]interface{}{"rollout_id": id})
}

func (kdb *katanDB) OldestRunningRollout(ctx context.Context) (katandb.RolloutDates, error) {
	var row rolloutDatesRow
	count, err := sqlutil.QueryContext(
		ctx,
		kdb.cluster.AliveChooser(),
		queryOldestRunningRollout,
		nil,
		func(rows *sqlx.Rows) error {
			return rows.StructScan(&row)
		}, kdb.logger,
	)
	if err != nil {
		return katandb.RolloutDates{}, xerrors.Errorf("OldestRollout query failed with: %w", err)
	}
	if count == 0 {
		return katandb.RolloutDates{}, katandb.ErrNoDataFound
	}
	return row.render(), nil
}

func (kdb *katanDB) StartPendingRollout(ctx context.Context, rolledBy string) (katandb.Rollout, error) {
	return kdb.queryRollout(ctx, queryStartPendingRollout,
		map[string]interface{}{"rolled_by": rolledBy})
}

func (kdb *katanDB) UnfinishedRollouts(ctx context.Context, rolledBy string) ([]katandb.Rollout, error) {
	var parser rolloutsParser
	if err := kdb.queryAlive(
		ctx,
		queryUnfinishedRollouts,
		map[string]interface{}{
			"rolled_by": rolledBy,
		},
		parser.parse,
	); err != nil {
		return nil, err
	}
	return parser.ret, nil
}

func (kdb *katanDB) LastRolloutsBySchedule(ctx context.Context, scheduleID int64, since time.Time, limit int) ([]katandb.Rollout, error) {
	var parser rolloutsParser
	if err := kdb.queryAlive(
		ctx,
		queryLastRolloutsBySchedule,
		map[string]interface{}{
			"schedule_id": scheduleID,
			"limit":       limit,
			"since":       since,
		},
		parser.parse,
	); err != nil {
		return nil, err
	}
	return parser.ret, nil
}

func (kdb *katanDB) RolloutClusters(ctx context.Context, rolloutID int64) ([]katandb.Cluster, error) {
	var parser clustersParser

	if err := kdb.queryMaster(ctx, queryRolloutClusters, map[string]interface{}{"rollout_id": rolloutID}, parser.parse); err != nil {
		return nil, err
	}

	return parser.ret, nil
}

func (kdb *katanDB) RolloutShipmentsByCluster(ctx context.Context, rolloutID int64, clusterID string) ([]katandb.RolloutShipment, error) {
	var ret []katandb.RolloutShipment

	if err := kdb.queryAlive(
		ctx,
		queryRolloutShipmentsByCluster,
		map[string]interface{}{
			"rollout_id": rolloutID,
			"cluster_id": clusterID,
		},
		func(rows *sqlx.Rows) error {
			var fqdns pgtype.TextArray
			var shipmentID int64

			if err := rows.Scan(&shipmentID, &fqdns); err != nil {
				return err
			}
			rolloutShipment := katandb.RolloutShipment{ShipmentID: shipmentID}
			if err := fqdns.AssignTo(&rolloutShipment.FQDNs); err != nil {
				return xerrors.Errorf("failed to assign rollout shipment FQDNs: %w", err)
			}
			ret = append(ret, rolloutShipment)
			return nil
		},
	); err != nil {
		return nil, err
	}

	return ret, nil
}

func (kdb *katanDB) RolloutClustersHosts(ctx context.Context, rolloutID int64) ([]katandb.Host, error) {
	var parser hostsParser

	if err := kdb.queryMaster(ctx, queryRolloutHosts, map[string]interface{}{"rollout_id": rolloutID}, parser.parse); err != nil {
		return nil, err
	}

	return parser.ret, nil
}

func (kdb *katanDB) ClusterHosts(ctx context.Context, clusterID string) ([]katandb.Host, error) {
	var parser hostsParser

	if err := kdb.queryAlive(ctx, queryClusterHosts, map[string]interface{}{"cluster_id": clusterID}, parser.parse); err != nil {
		return nil, err
	}

	return parser.ret, nil
}

func (kdb *katanDB) ClusterRollouts(ctx context.Context, rolloutID int64) ([]katandb.ClusterRollout, error) {
	var parser clusterRolloutsParser

	if err := kdb.queryAlive(ctx, queryClusterRollouts, map[string]interface{}{"rollout_id": rolloutID}, parser.parse); err != nil {
		return nil, err
	}

	return parser.ret, nil
}

func (kdb *katanDB) MarkClusterRollout(ctx context.Context, rolloutID int64, clusterID string, state katandb.ClusterRolloutState, comment string) error {
	return kdb.queryMaster(ctx, queryMarkClusterRollout, map[string]interface{}{
		"rollout_id": rolloutID,
		"cluster_id": clusterID,
		"state":      state.String(),
		"comment":    comment,
	}, sqlutil.NopParser)
}

func (kdb *katanDB) TouchClusterRollout(ctx context.Context, rolloutID int64, clusterID string) error {
	return kdb.queryMaster(ctx, queryTouchClusterRollout, map[string]interface{}{
		"rollout_id": rolloutID,
		"cluster_id": clusterID,
	}, sqlutil.NopParser)
}

func (kdb *katanDB) AddRolloutShipment(ctx context.Context, rolloutID int64, fqdns []string, shipmentID int64) error {
	var fqdnsArr pgtype.TextArray
	if err := fqdnsArr.Set(fqdns); err != nil {
		return xerrors.Errorf("fail to set fqdns var: %w", err)
	}
	return kdb.queryMaster(ctx, queryAddRolloutShipment, map[string]interface{}{
		"rollout_id":  rolloutID,
		"fqdns":       fqdnsArr,
		"shipment_id": shipmentID,
	}, sqlutil.NopParser)
}

func (kdb *katanDB) FinishRollout(ctx context.Context, id int64, comment optional.String) error {
	var pgComment sql.NullString
	if comment.Valid {
		pgComment.Valid = true
		pgComment.String = comment.String
	}
	return kdb.queryMaster(ctx, queryFinishRollout, map[string]interface{}{"rollout_id": id, "comment": pgComment}, sqlutil.NopParser)
}

func (kdb *katanDB) Schedules(ctx context.Context) ([]katandb.Schedule, error) {
	var parser schedulesParser

	if err := kdb.queryAlive(ctx, querySchedules, map[string]interface{}{}, parser.parse); err != nil {
		return nil, err
	}

	return parser.ret, nil
}

func (kdb *katanDB) ClusterRolloutsFailedInSchedule(ctx context.Context, scheduleID int64) ([]katandb.ClusterRollout, error) {
	var parser clusterRolloutsParser

	if err := kdb.queryAlive(
		ctx,
		queryScheduleFails,
		map[string]interface{}{
			"schedule_id": scheduleID,
		},
		parser.parse); err != nil {
		return nil, err
	}

	return parser.ret, nil
}

func (kdb *katanDB) MarkSchedule(ctx context.Context, scheduleID int64, state katandb.ScheduleState, examinedRolloutID int64, fails []katandb.ScheduleFail) error {
	if err := kdb.queryMaster(
		ctx,
		queryDelScheduleFails,
		map[string]interface{}{
			"schedule_id": scheduleID,
		},
		sqlutil.NopParser,
	); err != nil {
		return err
	}
	for _, f := range fails {
		if err := kdb.queryMaster(
			ctx,
			queryAddScheduleFail,
			map[string]interface{}{
				"schedule_id": scheduleID,
				"rollout_id":  f.RolloutID,
				"cluster_id":  f.ClusterID,
			},
			sqlutil.NopParser,
		); err != nil {
			return err
		}
	}
	return kdb.queryMaster(ctx, queryMarkSchedule,
		map[string]interface{}{
			"schedule_id":         scheduleID,
			"state":               state.String(),
			"examined_rollout_id": examinedRolloutID,
		},
		sqlutil.NopParser)
}
