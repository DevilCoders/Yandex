package pg

import (
	"context"
	"encoding/json"
	"fmt"
	"strings"
	"time"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"
	"github.com/jmoiron/sqlx/types"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

const maintenanceClusterTypes string = `
     'postgresql_cluster',
     'clickhouse_cluster',
     'mongodb_cluster',
     'redis_cluster',
     'kafka_cluster',
     'mysql_cluster',
     'greenplum_cluster',
	 'elasticsearch_cluster'
`

var (
	queryGetClusterIDByVtypeID =
	// language=PostgreSQL
	`SELECT c.cid
  FROM dbaas.clusters c
  JOIN dbaas.subclusters sc USING (cid)
  JOIN dbaas.hosts h USING (subcid)
 WHERE h.vtype_id = :vtype`
	queryTemplateSelectClusters = sqlutil.NewStmt(
		"SelectClusters",
		// language=PostgreSQL
		`SELECT *, coalesce(in_progress.count_tasks > 0, false) as has_tasks
  FROM dbaas.clusters c
  LEFT JOIN dbaas.maintenance_window_settings mws USING (cid)
  JOIN dbaas.folders f USING (folder_id)
  JOIN dbaas.clouds cl USING (cloud_id)
  JOIN LATERAL (SELECT count(*) as count_tasks FROM dbaas.maintenance_tasks t
	WHERE t.status != 'COMPLETED' AND t.cid = c.cid) in_progress ON true
 WHERE c.cid IN ( %s )
   AND c.status = ANY(CAST(:statuses AS dbaas.cluster_status[]))
   AND c.type IN ( %s )`,
		clusterRow{},
	)
	queryLockFutureCluster = sqlutil.Stmt{
		Name: "LockFutureCluster",
		// language=PostgreSQL
		Query: "SELECT rev, next_rev FROM code.lock_future_cluster(:cid)",
	}

	queryLockCluster = sqlutil.Stmt{
		Name: "LockCluster",
		// language=PostgreSQL
		Query: "SELECT rev FROM code.lock_cluster(:cid)",
	}

	querySelectMaintenanceTasks = sqlutil.NewStmt(
		"SelectMaintenanceTasks",
		// language=PostgreSQL
		`
SELECT t.*
FROM dbaas.maintenance_tasks t
WHERE config_id = :config_id
  AND EXISTS (
    SELECT 1
    FROM dbaas.clusters c
    WHERE c.cid = t.cid
  	AND c.status IN ('RUNNING', 'STOPPED'))
`,
		maintenanceTaskRow{},
	)
	querySelectMaintenanceTasksByCIDs = sqlutil.NewStmt(
		"SelectMaintenanceTasksByCIDs",
		// language=PostgreSQL
		`
SELECT t.*
FROM dbaas.maintenance_tasks t
WHERE cid in ( %s )
`,
		maintenanceTaskRow{},
	)
	queryCompleteFutureClusterChange = sqlutil.Stmt{
		Name: "CompleteFutureClusterChange",
		// language=PostgreSQL
		Query: "SELECT code.complete_future_cluster_change(:cid, :rev, :next_rev)",
	}
	queryCompleteClusterChange = sqlutil.Stmt{
		Name: "CompleteClusterChange",
		// language=PostgreSQL
		Query: "SELECT code.complete_cluster_change(:cid, :rev)",
	}
	queryCompleteMaintenanceTask = sqlutil.Stmt{
		Name: "CompleteMaintenanceTask",
		// language=PostgreSQL
		Query: "UPDATE dbaas.maintenance_tasks set status = 'COMPLETED' WHERE config_id = :config_id and cid = :cid",
	}
	queryPlanMaintenanceTask = sqlutil.Stmt{
		Name: "PlanMaintenanceTask",
		// language=PostgreSQL
		Query: `SELECT code.plan_maintenance_task(
    i_task_id     	 => :task_id,
    i_cid            => :cid,
    i_config_id      => :config_id,
    i_folder_id      => :folder_id,
    i_operation_type => :operation_type,
    i_task_type      => :task_type,
    i_task_args      => CAST(:task_args AS jsonb),
    i_version        => :version,
    i_metadata       => CAST(:metadata AS jsonb),
    i_user_id        => :user_id,
    i_rev            => :rev,
    i_target_rev     => :target_rev,
    i_plan_ts        => :plan_ts,
    i_timeout        => :timeout,
    i_info           => :info,
    i_create_ts		 => :create_ts,
	i_max_delay 	 => :max_delay
)`,
	}

	queryGetZkHosts = sqlutil.Stmt{
		Name: "GetZkHosts",
		// language=PostgreSQL
		Query: `
 SELECT CASE
           WHEN type = 'postgresql_cluster' THEN value -> 'data' -> 'pgsync' -> 'zk_hosts'
           WHEN type = 'mysql_cluster' THEN value -> 'data' -> 'mysql' -> 'zk_hosts'
           ELSE CAST('null' AS jsonb) END
 FROM dbaas.clusters
         JOIN dbaas.pillar USING (cid)
 WHERE cid = :cid`,
	}

	onlyRunningClusterStatus         = []string{"RUNNING"}
	runningAndStoppedClusterStatuses = []string{"RUNNING", "STOPPED"}
)

// PGMetaDB implements MetaDB interface for PostgreSQL
type PGMetaDB struct {
	logger  log.Logger
	cluster *sqlutil.Cluster
}

func (mdb *PGMetaDB) Exec(ctx context.Context, query string, parser func(rows *sqlx.Rows) error) (int64, error) {
	if parser == nil {
		parser = func(rows *sqlx.Rows) error { return nil }
	}
	return sqlutil.QueryContext(
		ctx,
		mdb.cluster.PrimaryChooser(),
		sqlutil.Stmt{
			Name:  "custom test query",
			Query: query,
		},
		map[string]interface{}{},
		parser,
		mdb.logger)
}

func (mdb *PGMetaDB) GetClusterByInstanceID(ctx context.Context, instanceID string, clusterType string) (models.Cluster, error) {
	var result models.Cluster
	query := queryTemplateSelectClusters.Format(queryGetClusterIDByVtypeID, fmt.Sprintf("'%s'", clusterType))
	ctxlog.Debug(ctx, mdb.logger, "select clusters by instance id", log.String("query", query.Query), log.String("instanceID", instanceID))
	parser := func(rows *sqlx.Rows) error {
		var r clusterRow
		if err := rows.StructScan(&r); err != nil {
			return err
		}
		cluster, err := r.render()
		if err != nil {
			return err
		}
		result = cluster
		return nil
	}
	mdb.logger.Debug("Getting cluster by instance id", log.String("instanceID", instanceID))
	count, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.PrimaryChooser(),
		query,
		map[string]interface{}{"vtype": instanceID, "statuses": onlyRunningClusterStatus},
		parser,
		mdb.logger)
	if err != nil {
		return models.Cluster{}, err
	}
	if count == 0 {
		return models.Cluster{}, sqlerrors.ErrNotFound.Wrap(xerrors.New(fmt.Sprintf("instance %q", instanceID)))
	}
	mdb.logger.Debug("Got cluster by instance id", log.String("instanceID", instanceID), log.String("cluster_name", result.ClusterName))
	return result, nil
}

func New(cfg pgutil.Config, logger log.Logger) (metadb.MetaDB, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(logger)))
	if err != nil {
		return nil, err
	}
	return NewWithCluster(cluster, logger), nil
}

func NewWithCluster(cluster *sqlutil.Cluster, logger log.Logger) metadb.MetaDB {
	return &PGMetaDB{
		logger:  logger,
		cluster: cluster,
	}
}

func (mdb *PGMetaDB) SelectClusters(ctx context.Context, cfg models.MaintenanceTaskConfig) ([]models.Cluster, error) {
	var ret []models.Cluster
	query := queryTemplateSelectClusters.Format(cfg.ClustersSelection.DB, maintenanceClusterTypes)
	ctxlog.Debug(ctx, mdb.logger, "select clusters query", log.String("query", query.Query))
	statuses := onlyRunningClusterStatus
	if cfg.SupportsOffline {
		statuses = runningAndStoppedClusterStatuses
	}
	_, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.PrimaryChooser(),
		query,
		map[string]interface{}{"config_id": cfg.ID, "statuses": statuses},
		func(rows *sqlx.Rows) error {
			var r clusterRow
			if err := rows.StructScan(&r); err != nil {
				return err
			}
			cluster, err := r.render()
			if err != nil {
				return err
			}
			ret = append(ret, cluster)
			return nil
		},
		mdb.logger)
	if err != nil {
		return nil, err
	}
	return ret, nil
}

func (mdb *PGMetaDB) ChangePillar(ctx context.Context, cid string, cfg models.MaintenanceTaskConfig) error {
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.PrimaryChooser(),
		sqlutil.Stmt{
			Name:  "ChangePillar",
			Query: cfg.PillarChange,
		},
		map[string]interface{}{"cid": cid},
		sqlutil.NopParser,
		mdb.logger); err != nil {
		return xerrors.Errorf("ChangePillar failed with %w", err)
	}
	return nil
}

func (mdb *PGMetaDB) SelectTaskArgs(ctx context.Context, cid string, cfg models.MaintenanceTaskConfig) (map[string]interface{}, error) {
	var taskArgs types.JSONText
	count, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.PrimaryChooser(),
		sqlutil.Stmt{
			Name:  "SelectTaskArgs",
			Query: cfg.Worker.TaskArgsQuery,
		},
		map[string]interface{}{"cid": cid},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&taskArgs)
		},
		mdb.logger)
	if err != nil {
		return nil, err
	}
	if count != 1 {
		return nil, xerrors.Errorf("TaskArgsQuery must return exactly 1 row, got: (%v)", count)
	}

	var ret map[string]interface{}
	if err = json.Unmarshal(taskArgs, &ret); err != nil {
		return nil, err
	}

	return ret, nil
}

func (mdb *PGMetaDB) LockFutureCluster(ctx context.Context, cid string) (models.ClusterRevs, error) {
	var ret models.ClusterRevs

	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.PrimaryChooser(),
		queryLockFutureCluster,
		map[string]interface{}{"cid": cid},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&ret.Rev, &ret.NextRev)
		},
		mdb.logger); err != nil {
		return models.ClusterRevs{}, xerrors.Errorf("LockFutureCluster failed with %w", err)
	}
	return ret, nil
}

func (mdb *PGMetaDB) CompleteFutureClusterChange(ctx context.Context, cid string, rev, nextRev int64) error {
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.PrimaryChooser(),
		queryCompleteFutureClusterChange,
		map[string]interface{}{"cid": cid, "rev": rev, "next_rev": nextRev},
		sqlutil.NopParser,
		mdb.logger); err != nil {
		return xerrors.Errorf("CompleteFutureClusterChange failed with %w", err)
	}
	return nil
}

func (mdb *PGMetaDB) GetZkHosts(ctx context.Context, cid string) (types.JSONText, error) {
	var zkHosts types.JSONText

	query := queryGetZkHosts
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.PrimaryChooser(),
		query,
		map[string]interface{}{"cid": cid},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&zkHosts)
		},
		mdb.logger); err != nil {
		return nil, xerrors.Errorf("%s failed with %w", query.Name, err)
	}
	return zkHosts, nil
}

func (mdb *PGMetaDB) AddZkHostsToTaskArgs(ctx context.Context, req *models.PlanMaintenanceTaskRequest) error {
	zkHosts, err := mdb.GetZkHosts(ctx, req.ClusterID)
	if err != nil {
		return err
	}
	if len(zkHosts) == 0 {
		return nil
	}
	var dat map[string]interface{}
	if req.TaskArgs == "null" {
		req.TaskArgs = "{}"
	}
	err = json.Unmarshal([]byte(req.TaskArgs), &dat)
	if err != nil {
		return err
	}
	dat["zk_hosts"] = zkHosts
	newTaskArgs, _ := json.Marshal(dat)
	req.TaskArgs = string(newTaskArgs)
	return nil
}

func (mdb *PGMetaDB) PlanMaintenanceTask(ctx context.Context, req models.PlanMaintenanceTaskRequest) error {
	var pgTimeout pgtype.Interval
	if err := pgTimeout.Set(req.Timeout); err != nil {
		return xerrors.Errorf("set timeout var: %w", err)
	}
	if err := mdb.AddZkHostsToTaskArgs(ctx, &req); err != nil {
		return xerrors.Errorf("Add zk_hosts error: %w", err)
	}
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.PrimaryChooser(),
		queryPlanMaintenanceTask,
		map[string]interface{}{
			"max_delay":      req.MaxDelay,
			"task_id":        req.ID,
			"cid":            req.ClusterID,
			"config_id":      req.ConfigID,
			"folder_id":      req.FolderID,
			"operation_type": req.OperationType,
			"task_type":      req.TaskType,
			"task_args":      req.TaskArgs,
			"version":        req.Version,
			"metadata":       req.Metadata,
			"user_id":        req.UserID,
			"rev":            req.Rev,
			"target_rev":     req.TargetRev,
			"plan_ts":        req.PlanTS,
			"timeout":        pgTimeout,
			"info":           req.Info,
			"create_ts":      req.CreateTS,
		},
		sqlutil.NopParser,
		mdb.logger); err != nil {
		return xerrors.Errorf("PlanMaintenanceTask failed with %w", err)
	}
	return nil
}

func (mdb *PGMetaDB) MaintenanceTasks(ctx context.Context, configID string) ([]models.MaintenanceTask, error) {
	var ret []models.MaintenanceTask
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.AliveChooser(),
		querySelectMaintenanceTasks,
		map[string]interface{}{
			"config_id": configID,
		},
		func(rows *sqlx.Rows) error {
			var r maintenanceTaskRow
			if err := rows.StructScan(&r); err != nil {
				return err
			}
			m, err := r.render()
			if err != nil {
				return err
			}
			ret = append(ret, m)
			return nil
		},
		mdb.logger); err != nil {
		return nil, xerrors.Errorf("SelectMaintenanceTasks failed with %w", err)
	}
	return ret, nil
}

func (mdb *PGMetaDB) MaintenanceTasksByCIDs(ctx context.Context, CIDs []string) ([]models.MaintenanceTask, error) {
	if len(CIDs) == 0 {
		return []models.MaintenanceTask{}, nil
	}
	var ret []models.MaintenanceTask
	var quotedCIDs []string
	for _, CID := range CIDs {
		quotedCIDs = append(quotedCIDs, fmt.Sprintf("'%s'", CID))
	}
	query := querySelectMaintenanceTasksByCIDs.Format(strings.Join(quotedCIDs, ", "))

	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.AliveChooser(),
		query,
		map[string]interface{}{},
		func(rows *sqlx.Rows) error {
			var r maintenanceTaskRow
			if err := rows.StructScan(&r); err != nil {
				return err
			}
			m, err := r.render()
			if err != nil {
				return err
			}
			ret = append(ret, m)
			return nil
		},
		mdb.logger); err != nil {
		return nil, xerrors.Errorf("MaintenanceTasksByCIDs failed with %w", err)
	}
	return ret, nil
}

func (mdb *PGMetaDB) Close() error {
	return mdb.cluster.Close()
}

func (mdb *PGMetaDB) IsReady(_ context.Context) error {
	node := mdb.cluster.Alive()
	if node == nil {
		return semerr.Unavailable("unavailable")
	}

	return nil
}

func (mdb *PGMetaDB) Begin(ctx context.Context) (context.Context, error) {
	binding, err := sqlutil.Begin(ctx, mdb.cluster, sqlutil.Primary, nil)
	if err != nil {
		return ctx, err
	}

	return sqlutil.WithTxBinding(ctx, binding), nil
}

func (mdb *PGMetaDB) Commit(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Commit(ctx)
}

func (mdb *PGMetaDB) Rollback(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Rollback(ctx)
}

func (mdb *PGMetaDB) LockCluster(ctx context.Context, cid string) (int64, error) {
	var rev int64

	query := queryLockCluster
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.PrimaryChooser(),
		query,
		map[string]interface{}{"cid": cid},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&rev)
		},
		mdb.logger); err != nil {
		return 0, xerrors.Errorf("%s failed with %w", query.Name, err)
	}
	return rev, nil
}

func (mdb *PGMetaDB) CompleteClusterChange(ctx context.Context, cid string, rev int64) error {
	query := queryCompleteClusterChange
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.PrimaryChooser(),
		query,
		map[string]interface{}{"cid": cid, "rev": rev},
		sqlutil.NopParser,
		mdb.logger); err != nil {
		return xerrors.Errorf("%s failed with %w", query.Name, err)
	}
	return nil
}

func (mdb *PGMetaDB) CompleteMaintenanceTask(ctx context.Context, cid string, configID string) error {
	query := queryCompleteMaintenanceTask
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.PrimaryChooser(),
		query,
		map[string]interface{}{"cid": cid, "config_id": configID},
		sqlutil.NopParser, mdb.logger); err != nil {
		return xerrors.Errorf("%s failed with %w", query.Name, err)
	}
	return nil
}

func (mdb *PGMetaDB) SelectTimeout(ctx context.Context, cid string, cfg models.MaintenanceTaskConfig) (time.Duration, error) {
	var timeout string
	var ret time.Duration
	count, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.PrimaryChooser(),
		sqlutil.Stmt{
			Name:  "SelectTimeout",
			Query: cfg.Worker.TimeoutQuery,
		},
		map[string]interface{}{"cid": cid},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&timeout)
		},
		mdb.logger)
	if err != nil {
		return ret, err
	}
	if count != 1 {
		return ret, xerrors.Errorf("TimeoutQuery must return exactly 1 row, got: (%v)", count)
	}

	if ret, err = time.ParseDuration(timeout); err != nil {
		return ret, err
	}

	return ret, nil
}
