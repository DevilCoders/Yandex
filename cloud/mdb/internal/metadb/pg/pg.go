package pg

import (
	"context"
	"database/sql"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	pgxutil "a.yandex-team.ru/library/go/x/sql/pgx"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

var _ metadb.MetaDB = &metaDB{}

var (
	queryGetAllHostsInShard = sqlutil.Stmt{
		Name: "GetAllHostsInShard",
		// language=PostgreSQL
		Query: `SELECT h.fqdn, sc.cid, h.subcid, h.shard_id, g.name as geo, h.created_at, sc.roles, dt.disk_type_ext_id
FROM dbaas.hosts h
INNER JOIN dbaas.geo g
    ON g.geo_id = h.geo_id
INNER JOIN dbaas.subclusters sc USING (subcid)
INNER JOIN dbaas.disk_type dt USING (disk_type_id)
WHERE
      h.shard_id = :shard_id`,
	}

	queryGetAllHostsInSubcluster = sqlutil.Stmt{
		Name: "GetAllHostsInSubcluster",
		// language=PostgreSQL
		Query: `SELECT h.fqdn, sc.cid, h.subcid, h.shard_id, g.name as geo, h.created_at, sc.roles, dt.disk_type_ext_id
FROM dbaas.hosts h
INNER JOIN dbaas.geo g
    ON g.geo_id = h.geo_id
INNER JOIN dbaas.subclusters sc USING (subcid)
INNER JOIN dbaas.disk_type dt USING (disk_type_id)
WHERE
	h.subcid = :subcid`,
	}

	queryGetHostsByFQDN = sqlutil.Stmt{
		Name: "GetHostsByFQDN",
		// language=PostgreSQL
		Query: `SELECT h.fqdn, sc.cid, h.subcid, h.shard_id, g.name as geo, h.created_at, sc.roles, dt.disk_type_ext_id
FROM dbaas.hosts h
INNER JOIN dbaas.geo g
    ON g.geo_id = h.geo_id
INNER JOIN dbaas.subclusters sc USING (subcid)
INNER JOIN dbaas.disk_type dt USING (disk_type_id)
WHERE
	h.fqdn = :fqdn`,
	}

	queryGetHostsByVtypeID = sqlutil.Stmt{
		Name: "GetHostsByVtypeID",
		// language=PostgreSQL
		Query: `SELECT h.fqdn, sc.cid, h.subcid, h.shard_id, g.name as geo, h.created_at, sc.roles, dt.disk_type_ext_id
FROM dbaas.hosts h
INNER JOIN dbaas.geo g
    ON g.geo_id = h.geo_id
INNER JOIN dbaas.subclusters sc USING (subcid)
INNER JOIN dbaas.disk_type dt USING (disk_type_id)
WHERE
	h.vtype_id = :vtype_id`,
	}

	querySelectOperation = sqlutil.Stmt{
		Name:  "SelectOperation",
		Query: "SELECT * FROM dbaas.worker_queue WHERE task_id = :id",
	}

	querySelectLastOpByType = sqlutil.Stmt{
		Name: "SelectLastOpByType",
		Query: `SELECT cid, start_ts as ts
		FROM dbaas.worker_queue
		WHERE task_type = :task_type AND start_ts > :ts
	`,
	}

	querySelectFolderExtID = sqlutil.Stmt{
		Name: "SelectFolderExtID",
		// language=PostgreSQL
		Query: `SELECT folder_ext_id
FROM dbaas.clusters
JOIN dbaas.folders
    USING (folder_id)
WHERE
      type = :type
      AND cid = :cid`,
	}

	querySelectClusterInfo = sqlutil.Stmt{
		Name:  "SelectClusterInfo",
		Query: `SELECT network_id as nid, public_key as pkey, type as ctype, env, folder_id, status FROM dbaas.clusters WHERE cid = :cid`,
	}

	querySelectShardByID = sqlutil.Stmt{
		Name:  "SelectShardById",
		Query: `SELECT name as sname FROM dbaas.shards WHERE shard_id = :sid`,
	}

	querySelectClusterShards = sqlutil.Stmt{
		Name:  "SelectClusterShards",
		Query: `SELECT sh.name as sname FROM dbaas.subclusters JOIN dbaas.shards as sh using(subcid) where cid=:cid;`,
	}

	querySelectClouds = sqlutil.NewStmt(
		"SelectClouds",
		"SELECT * FROM dbaas.clouds",
		Cloud{})

	querySetCloudQuota = sqlutil.Stmt{
		Name: "SetCloudQuota",
		Query: `
	SELECT *
	  FROM code.set_cloud_quota(
		  i_cloud_ext_id   => :cloud_ext_id,
		  i_quota          => code.make_quota(
			  i_cpu		   => :cpu,
			  i_memory     => :memory,
			  i_ssd_space  => :ssd,
			  i_hdd_space  => :hdd,
			  i_clusters   => :clusters
		  ),
		  i_x_request_id => :x_request_id
	)
	`,
	}

	queryCreateCloud = sqlutil.Stmt{
		Name: "CreateCloud",
		Query: `
	SELECT *
	  FROM code.add_cloud(
		  i_cloud_ext_id   => :cloud_ext_id,
		  i_quota          => code.make_quota(
			  i_cpu		   => :cpu,
			  i_memory     => :memory,
			  i_ssd_space  => :ssd,
			  i_hdd_space  => :hdd,
			  i_clusters   => :clusters
		  ),
		  i_x_request_id => :x_request_id
	)
	`,
	}

	queryAcquireUnsentStartEvents = sqlutil.NewStmt(
		"AcquireUnsentStartEvents",
		`SELECT e.*, q.create_ts AS created_at
			 FROM dbaas.worker_queue_events e
			 JOIN dbaas.worker_queue q
            USING (task_id)
			WHERE start_sent_at IS NULL
			  FOR NO KEY UPDATE OF e SKIP LOCKED
			LIMIT :limit`,
		workerQueueEventRow{})

	queryMarkUnsetStartEvents = sqlutil.Stmt{
		Name:  "MarkUnsetStartEvents",
		Query: "UPDATE dbaas.worker_queue_events SET start_sent_at=now() WHERE event_id = ANY(:event_ids)",
	}

	queryAcquireUnsentFinishEvents = sqlutil.NewStmt(
		"AcquireUnsentFinishEvents",
		`SELECT e.*, q.end_ts AS created_at
			 FROM dbaas.worker_queue_events e
			 JOIN dbaas.worker_queue q
			USING (task_id)
			WHERE e.start_sent_at IS NOT NULL
              AND e.done_sent_at IS NULL
              AND q.end_ts IS NOT NULL
              AND q.result
			  FOR NO KEY UPDATE OF e SKIP LOCKED
			LIMIT :limit`,
		workerQueueEventRow{})

	queryMarkUnsetFinishEvents = sqlutil.Stmt{
		Name:  "MarkUnsetFinishEvents",
		Query: "UPDATE dbaas.worker_queue_events SET done_sent_at=now() WHERE event_id = ANY(:event_ids)",
	}

	queryOldestUnsentStartEvent = sqlutil.NewStmt(
		"OldestUnsentStartEvent",
		// language=PostgreSQL
		`SELECT e.*, q.create_ts AS created_at
			 FROM dbaas.worker_queue_events e
			 JOIN dbaas.worker_queue q
            USING (task_id)
			WHERE start_sent_at IS NULL
			ORDER BY q.create_ts ASC
			LIMIT 1`,
		workerQueueEventRow{},
	)

	queryClustersRevs = sqlutil.Stmt{
		Name: "ClustersRevs",
		// language=PostgreSQL
		Query: `
SELECT cid, actual_rev AS rev
  FROM dbaas.clusters
 WHERE dbaas.visible_cluster_status(status)
   AND code.managed(clusters)`,
	}

	queryClusterHostsAtRev = sqlutil.Stmt{
		Name: "ClusterHostsAtRev",
		// language=PostgreSQL
		Query: `SELECT fqdn, subcid, shard_id, geo, roles, created_at FROM code.get_hosts_by_cid_at_rev(i_cid => :cid, i_rev => :rev)`,
	}

	queryClusterHealthNonaggregatable = sqlutil.Stmt{
		Name: "ClusterHealthNonaggregatable",
		// language=PostgreSQL
		Query: `SELECT CAST(value->'data'->'mdb_health'->>'nonaggregatable' AS boolean) AS nonaggregatable FROM dbaas.pillar_revs WHERE cid = :cid AND rev = :rev`,
	}

	queryClusterAtRev = sqlutil.NewStmt(
		"ClusterAtRev",
		// language=PostgreSQL
		`
		SELECT dbaas.visible_cluster_status(cr.status) AS visible, c.env, cr.name, c.type, c.status
		  FROM dbaas.clusters c
     	  JOIN dbaas.clusters_revs cr USING (cid) WHERE cid=:cid AND rev=:rev`,
		clusterRow{},
	)

	queryMysqlClusterCustomRoles = sqlutil.Stmt{
		Name: "mysql cluster custom roles",
		Query:

		// language=PostgreSQL
		`SELECT
			fqdn,
			value->'data'->'mysql'->'replication_source' IS NOT NULL
			FROM dbaas.pillar_revs
			WHERE fqdn IN(
				SELECT fqdn from dbaas.hosts WHERE subcid = (SELECT subcid FROM dbaas.subclusters WHERE cid = :cid)
			) AND rev=:rev`,
	}

	queryPGClusterCustomRoles = sqlutil.Stmt{
		Name: "postgresql cluster custom roles",
		Query:
		// language=PostgreSQL
		`SELECT
			fqdn,
			value->'data'->'pgsync'->'replication_source' IS NOT NULL
			FROM dbaas.pillar_revs
			WHERE fqdn IN(
				SELECT fqdn from dbaas.hosts WHERE subcid = (SELECT subcid FROM dbaas.subclusters WHERE cid = :cid)
			) AND rev=:rev`,
	}

	queryTaskTypeByActions = sqlutil.Stmt{
		Name: "task type by action",
		Query: `SELECT
			task_type
		FROM code.task_type_action_map()
		WHERE action = :action;
		`,
	}
)

// LoadConfig loads PostgreSQL datastore configuration
func LoadConfig(logger log.Logger, configName string) pgutil.Config {
	cfg := pgutil.DefaultConfig()
	if err := config.Load(configName, &cfg); err != nil {
		logger.Fatalf("failed to load postgresql metadb config: %s", err)
	}

	if !cfg.Password.FromEnv("METADB_PASSWORD") {
		logger.Info("METADB_PASSWORD is empty")
	}

	return cfg
}

// metaDB implements metaDB interface for PostgreSQL
type metaDB struct {
	l log.Logger

	cluster             *sqlutil.Cluster
	customRolesHandlers map[metadb.ClusterType]func(mdb *metaDB, ctx context.Context, cid string, rev int64) (map[string]metadb.CustomRole, error)
}

// New constructs metaDB
func New(cfg pgutil.Config, l log.Logger) (metadb.MetaDB, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(l)))
	if err != nil {
		return nil, err
	}

	return NewWithCluster(cluster, l), nil
}

// NewWithCluster constructs metaDB with custom cluster (useful for mocking)
func NewWithCluster(cluster *sqlutil.Cluster, l log.Logger) metadb.MetaDB {
	return &metaDB{
		l:       l,
		cluster: cluster,
		customRolesHandlers: map[metadb.ClusterType]func(mdb *metaDB, ctx context.Context, cid string, rev int64) (map[string]metadb.CustomRole, error){
			metadb.PostgresqlCluster: pgHandler,
			metadb.MysqlCluster:      myHandler,
		},
	}
}

func myHandler(mdb *metaDB, ctx context.Context, cid string, rev int64) (map[string]metadb.CustomRole, error) {
	res := map[string]metadb.CustomRole{}
	parser := func(rows *sqlx.Rows) error {
		var fqdn string
		var isCascadeReplica bool
		if err := rows.Scan(&fqdn, &isCascadeReplica); err != nil {
			return err
		}
		if isCascadeReplica {
			res[fqdn] = metadb.MysqlCascadeReplicaRole
		}
		return nil
	}
	_, err := sqlutil.QueryContext(
		ctx, mdb.cluster.AliveChooser(), queryMysqlClusterCustomRoles, map[string]interface{}{
			"cid": cid,
			"rev": rev,
		}, parser, mdb.l,
	)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func pgHandler(mdb *metaDB, ctx context.Context, cid string, rev int64) (map[string]metadb.CustomRole, error) {
	res := map[string]metadb.CustomRole{}
	parser := func(rows *sqlx.Rows) error {
		var fqdn string
		var isCascadeReplica bool
		if err := rows.Scan(&fqdn, &isCascadeReplica); err != nil {
			return err
		}
		if isCascadeReplica {
			res[fqdn] = metadb.PostgresqlCascadeReplicaRole
		}
		return nil
	}

	_, err := sqlutil.QueryContext(
		ctx, mdb.cluster.AliveChooser(), queryPGClusterCustomRoles, map[string]interface{}{
			"cid": cid,
			"rev": rev,
		}, parser, mdb.l,
	)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func (mdb *metaDB) Close() error {
	return mdb.cluster.Close()
}

func (mdb *metaDB) IsReady(ctx context.Context) error {
	node := mdb.cluster.Alive()
	if node == nil {
		return semerr.Unavailable("unavailable")
	}

	return nil
}

func (mdb *metaDB) callOnMaster(ctx context.Context, stmt sqlutil.Stmt, params map[string]interface{}) error {
	_, err := sqlutil.QueryContext(
		ctx, mdb.cluster.PrimaryChooser(), stmt, params, sqlutil.NopParser, mdb.l,
	)
	return err
}

func (mdb *metaDB) Operation(ctx context.Context, id string) (*metadb.Operation, error) {
	var opRow Operation
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&opRow)
	}

	args := map[string]interface{}{"id": id}
	count, err := sqlutil.QueryContext(
		ctx, mdb.cluster.AliveChooser(), querySelectOperation, args, parser, mdb.l,
	)
	if err != nil {
		return nil, err
	}
	if count == 0 {
		return nil, metadb.ErrDataNotFound
	}

	formattedOp, err := FormatOperation(opRow)
	if err != nil {
		ctxlog.Error(ctx, mdb.l,
			"Error while formatting operation result from statemen",
			log.Reflect("stmt", querySelectOperation),
			log.Error(err),
		)
		return nil, metadb.ErrInvalidDataType.Wrap(err)
	}
	return formattedOp, nil
}

func (mdb *metaDB) LastOperationsByType(
	ctx context.Context,
	taskType string,
	fromts time.Time,
) ([]metadb.CidAndTimestamp, error) {
	params := map[string]interface{}{"task_type": taskType, "ts": fromts}

	var resList []metadb.CidAndTimestamp
	rowScanner := func(rows *sqlx.Rows) error {
		var row CidAndTimestamp
		if err := rows.StructScan(&row); err != nil {
			return err
		}
		rowSt := FormatCidAndTS(row)
		resList = append(resList, rowSt)
		return nil
	}
	if _, err := sqlutil.QueryContext(ctx, mdb.cluster.AliveChooser(), querySelectLastOpByType, params, rowScanner, mdb.l); err != nil {
		return nil, err
	}

	return resList, nil
}

func (mdb *metaDB) FolderExtIDByClusterID(ctx context.Context, cid string, clusterType metadb.ClusterType) (string, error) {
	params := map[string]interface{}{
		"cid":  cid,
		"type": clusterType,
	}
	var folderID string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&folderID)
	}
	count, err := sqlutil.QueryContext(ctx, mdb.cluster.AliveChooser(), querySelectFolderExtID, params, parser, mdb.l)
	if err != nil {
		return "", err
	}
	if count == 0 {
		return "", metadb.ErrDataNotFound
	}

	return folderID, nil
}

func (mdb *metaDB) ClusterInfo(ctx context.Context, cid string) (metadb.ClusterInfo, error) {
	params := map[string]interface{}{"cid": cid}
	var row ClusterInfo
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&row)
	}
	count, err := sqlutil.QueryContext(ctx, mdb.cluster.AliveChooser(), querySelectClusterInfo, params, parser, mdb.l)
	if err != nil {
		return metadb.ClusterInfo{}, err
	}
	if count == 0 {
		return metadb.ClusterInfo{}, metadb.ErrDataNotFound
	}

	return FormatClusterInfo(row), nil
}

func (mdb *metaDB) GetHostsByShardID(ctx context.Context, shardID string) ([]metadb.Host, error) {
	params := map[string]interface{}{"shard_id": shardID}
	var results []metadb.Host
	parser := func(rows *sqlx.Rows) error {
		var dbRow hostRow
		if err := rows.StructScan(&dbRow); err != nil {
			return err
		}
		results = append(results, formatHost(dbRow))
		return nil
	}
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.AliveChooser(),
		queryGetAllHostsInShard,
		params,
		parser,
		mdb.l,
	); err != nil {
		return nil, err
	}
	return results, nil
}

func (mdb *metaDB) GetHostsBySubcid(ctx context.Context, subCid string) ([]metadb.Host, error) {
	params := map[string]interface{}{"subcid": subCid}
	var results []metadb.Host
	parser := func(rows *sqlx.Rows) error {
		var dbRow hostRow
		if err := rows.StructScan(&dbRow); err != nil {
			return err
		}
		results = append(results, formatHost(dbRow))
		return nil
	}
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.AliveChooser(),
		queryGetAllHostsInSubcluster,
		params,
		parser,
		mdb.l,
	); err != nil {
		return nil, err
	}
	return results, nil
}

func (mdb *metaDB) GetHostByFQDN(ctx context.Context, fqdn string) (metadb.Host, error) {
	params := map[string]interface{}{"fqdn": fqdn}
	var results []metadb.Host
	parser := func(rows *sqlx.Rows) error {
		var dbRow hostRow
		if err := rows.StructScan(&dbRow); err != nil {
			return err
		}
		results = append(results, formatHost(dbRow))
		return nil
	}
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.AliveChooser(),
		queryGetHostsByFQDN,
		params,
		parser,
		mdb.l,
	); err != nil {
		return metadb.Host{}, err
	}

	if len(results) == 0 {
		return metadb.Host{}, metadb.ErrDataNotFound
	}

	if len(results) > 1 {
		return metadb.Host{}, xerrors.Errorf("Something wrong in metadb: there are %d hosts associated with fqdn %s",
			len(results), fqdn)
	}

	return results[0], nil
}

func (mdb *metaDB) GetHostByVtypeID(ctx context.Context, vtypeID string) (metadb.Host, error) {
	params := map[string]interface{}{"vtype_id": vtypeID}
	var results []metadb.Host
	parser := func(rows *sqlx.Rows) error {
		var dbRow hostRow
		if err := rows.StructScan(&dbRow); err != nil {
			return err
		}
		results = append(results, formatHost(dbRow))
		return nil
	}
	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.AliveChooser(),
		queryGetHostsByVtypeID,
		params,
		parser,
		mdb.l,
	); err != nil {
		return metadb.Host{}, err
	}

	if len(results) == 0 {
		return metadb.Host{}, metadb.ErrDataNotFound
	}

	if len(results) > 1 {
		return metadb.Host{}, xerrors.Errorf("Something wrong in metadb: there are %d hosts associated with vtype %q",
			len(results), vtypeID)
	}

	return results[0], nil
}

func (mdb *metaDB) ShardByID(ctx context.Context, sid string) (metadb.ShardInfo, error) {
	params := map[string]interface{}{"sid": sid}
	var row ShardInfo
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&row)
	}
	count, err := sqlutil.QueryContext(ctx, mdb.cluster.AliveChooser(), querySelectShardByID, params, parser, mdb.l)
	if err != nil {
		return metadb.ShardInfo{}, err
	}
	if count == 0 {
		return metadb.ShardInfo{}, metadb.ErrDataNotFound
	}

	return FormatShardInfo(row), nil
}

func (mdb *metaDB) ClusterShards(ctx context.Context, cid string) ([]metadb.ShardInfo, error) {
	params := map[string]interface{}{"cid": cid}
	var res []metadb.ShardInfo
	rowScanner := func(rows *sqlx.Rows) error {
		var row ShardInfo
		if err := rows.StructScan(&row); err != nil {
			return err
		}
		rowSt := FormatShardInfo(row)
		res = append(res, rowSt)
		return nil
	}
	_, err := sqlutil.QueryContext(ctx, mdb.cluster.AliveChooser(), querySelectClusterShards, params, rowScanner, mdb.l)
	return res, err
}

// Clouds return all clouds from metadb
func (mdb *metaDB) Clouds(ctx context.Context) ([]metadb.Cloud, error) {
	var resList []metadb.Cloud

	parser := func(rows *sqlx.Rows) error {
		var row Cloud
		if err := rows.StructScan(&row); err != nil {
			return err
		}
		resList = append(resList, FormatCloud(row))
		return nil
	}

	if _, err := sqlutil.QueryContext(
		ctx, mdb.cluster.AliveChooser(), querySelectClouds, map[string]interface{}{}, parser, mdb.l,
	); err != nil {
		return nil, err
	}
	return resList, nil
}

// Set new quota for the cloud
func (mdb *metaDB) SetCloudQuota(ctx context.Context, cloudID string, newResources metadb.ResourcesChange, xRequestID string) error {
	params := map[string]interface{}{
		"cloud_ext_id": cloudID,
		"x_request_id": xRequestID,
	}

	setParam := func(p optional.Int64, name string) {
		if p.Valid {
			params[name] = p.Must()
		} else {
			params[name] = nil
		}
	}
	params["cpu"] = nil
	if newResources.CPUCores.Valid {
		params["cpu"] = newResources.CPUCores.Must()
	}
	setParam(newResources.MemoryBytes, "memory")
	setParam(newResources.SSDBytes, "ssd")
	setParam(newResources.HDDBytes, "hdd")
	setParam(newResources.ClustersCount, "clusters")

	return mdb.callOnMaster(ctx, querySetCloudQuota, params)
}

func (mdb *metaDB) CreateCloud(ctx context.Context, cloudID string, quota metadb.Resources, xRequestID string) error {
	params := map[string]interface{}{
		"cloud_ext_id": cloudID,
		"x_request_id": xRequestID,
		"cpu":          quota.CPUCores,
		"memory":       quota.MemoryBytes,
		"ssd":          quota.SSDBytes,
		"hdd":          quota.HDDBytes,
		"clusters":     quota.ClustersCount,
	}

	return mdb.callOnMaster(ctx, queryCreateCloud, params)
}

func (mdb *metaDB) onWorkerQueueEvents(
	ctx context.Context,
	acquireQuery sqlutil.Stmt,
	acquireParams map[string]interface{},
	markQuery sqlutil.Stmt,
	handler metadb.OnWorkerQueueEventsHandler,
) (int, error) {
	acquiredEvents := make([]metadb.WorkerQueueEvent, 0)
	acquireParser := func(rows *sqlx.Rows) error {
		var r workerQueueEventRow
		if err := rows.StructScan(&r); err != nil {
			return err
		}
		acquiredEvents = append(acquiredEvents, formatWorkerQueueEvent(r))
		return nil
	}
	var sentEventsCount int

	err := sqlutil.InTxDoQueries(
		ctx,
		mdb.cluster.Primary(),
		mdb.l,
		func(query sqlutil.QueryCallback) error {
			if err := query(
				acquireQuery, acquireParams, acquireParser,
			); err != nil {
				return err
			}

			// call handler only if acquire something
			if len(acquiredEvents) == 0 {
				return nil
			}

			sentEventsIDs, err := handler(ctx, acquiredEvents)
			if err != nil {
				return err
			}
			sentEventsCount = len(sentEventsIDs)
			// update events status only if handler sent something
			if sentEventsCount == 0 {
				return nil
			}

			markParams := map[string]interface{}{
				"event_ids": pgxutil.Array(sentEventsIDs),
			}
			return query(markQuery, markParams, sqlutil.NopParser)
		})
	return sentEventsCount, err
}

func (mdb *metaDB) OnUnsentWorkerQueueStartEvents(
	ctx context.Context,
	limit int64,
	handler metadb.OnWorkerQueueEventsHandler,
) (int, error) {
	return mdb.onWorkerQueueEvents(
		ctx,
		queryAcquireUnsentStartEvents,
		map[string]interface{}{"limit": limit},
		queryMarkUnsetStartEvents,
		handler,
	)
}

func (mdb *metaDB) OnUnsentWorkerQueueDoneEvents(
	ctx context.Context,
	limit int64,
	handler metadb.OnWorkerQueueEventsHandler,
) (int, error) {
	return mdb.onWorkerQueueEvents(
		ctx,
		queryAcquireUnsentFinishEvents,
		map[string]interface{}{"limit": limit},
		queryMarkUnsetFinishEvents,
		handler,
	)
}

func (mdb *metaDB) OldestUnsetStartEvent(ctx context.Context) (metadb.WorkerQueueEvent, error) {
	var row workerQueueEventRow

	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&row)
	}
	_, err := sqlutil.QueryContext(
		ctx, mdb.cluster.AliveChooser(), queryOldestUnsentStartEvent, map[string]interface{}{}, parser, mdb.l,
	)
	if err != nil {
		return metadb.WorkerQueueEvent{}, err
	}
	return formatWorkerQueueEvent(row), nil
}

func (mdb *metaDB) ClustersRevs(ctx context.Context) ([]metadb.ClusterRev, error) {
	var resList []metadb.ClusterRev

	parser := func(rows *sqlx.Rows) error {
		var cid string
		var rev int64
		if err := rows.Scan(&cid, &rev); err != nil {
			return err
		}
		resList = append(resList, metadb.ClusterRev{
			ClusterID: cid,
			Rev:       rev,
		})
		return nil
	}

	if _, err := sqlutil.QueryContext(
		ctx, mdb.cluster.AliveChooser(), queryClustersRevs, map[string]interface{}{}, parser, mdb.l,
	); err != nil {
		return nil, err
	}
	return resList, nil
}

func (mdb *metaDB) ClusterHostsAtRev(ctx context.Context, cid string, rev int64) ([]metadb.Host, error) {
	var resList []metadb.Host

	parser := func(rows *sqlx.Rows) error {
		var row hostRow
		if err := rows.StructScan(&row); err != nil {
			return err
		}
		resList = append(resList, formatHost(row))
		return nil
	}

	if _, err := sqlutil.QueryContext(
		ctx, mdb.cluster.AliveChooser(), queryClusterHostsAtRev, map[string]interface{}{"cid": cid, "rev": rev}, parser, mdb.l,
	); err != nil {
		return nil, err
	}
	return resList, nil
}

func (mdb *metaDB) ClusterHealthNonaggregatable(ctx context.Context, cid string, rev int64) (bool, error) {
	var isNonaggregatable bool
	parser := func(rows *sqlx.Rows) error {
		var tribool sql.NullBool
		if err := rows.Scan(&tribool); err != nil {
			return err
		}
		if tribool.Valid {
			isNonaggregatable = tribool.Bool
		}
		return nil
	}

	if _, err := sqlutil.QueryContext(
		ctx, mdb.cluster.AliveChooser(), queryClusterHealthNonaggregatable, map[string]interface{}{"cid": cid, "rev": rev}, parser, mdb.l,
	); err != nil {
		return false, err
	}
	return isNonaggregatable, nil
}

func (mdb *metaDB) ClusterAtRev(ctx context.Context, cid string, rev int64) (metadb.Cluster, error) {
	var row clusterRow

	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&row)
	}

	count, err := sqlutil.QueryContext(
		ctx, mdb.cluster.AliveChooser(), queryClusterAtRev, map[string]interface{}{"cid": cid, "rev": rev}, parser, mdb.l,
	)

	if err != nil {
		return metadb.Cluster{}, err
	}

	if count == 0 {
		return metadb.Cluster{}, metadb.ErrDataNotFound
	}

	return formatCluster(row), nil
}

func (mdb *metaDB) GetClusterCustomRolesAtRev(ctx context.Context, cType metadb.ClusterType, cid string, rev int64) (map[string]metadb.CustomRole, error) {
	handler, ok := mdb.customRolesHandlers[cType]
	if !ok {
		// get custom role handler not implemented for this cluster type
		// but thats ok - maybe some cluster type do not need them
		return nil, nil
	}
	return handler(mdb, ctx, cid, rev)
}

func (mdb *metaDB) GetTaskTypeByAction(ctx context.Context, action string) ([]string, error) {
	var res []string
	parser := func(rows *sqlx.Rows) error {
		var taskType string
		if err := rows.Scan(&taskType); err != nil {
			return err
		}
		res = append(res, taskType)
		return nil
	}
	_, err := sqlutil.QueryContext(
		ctx, mdb.cluster.AliveChooser(), queryTaskTypeByActions, map[string]interface{}{
			"action": action,
		}, parser, mdb.l,
	)
	if err != nil {
		return nil, err
	}
	return res, nil
}
