package pg

import (
	"context"
	"database/sql"
	"encoding/json"
	"fmt"
	"time"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	queryCreateCluster = sqlutil.Stmt{
		Name: "CreateCluster",
		Query: `
SELECT
    cid,
    name,
    env,
    type,
    created_at,
    network_id,
    status,
    description,
	user_sgroup_ids,
    rev,
    host_group_ids,
    deletion_protection
FROM code.create_cluster(
    i_cid                 => :cid,
    i_name                => :name,
    i_type                => :type,
    i_env                 => :env,
    i_public_key          => :public_key,
    i_network_id          => :network_id,
    i_folder_id           => :folder_id,
    i_description         => :description,
    i_x_request_id        => :x_request_id,
    i_host_group_ids      => :host_group_ids,
    i_deletion_protection => :deletion_protection
)`,
	}

	querySelectFolderCoordsByFolderExtID = sqlutil.Stmt{
		Name: "SelectFolderCoordsByFolderExtID",
		Query: `
SELECT
	c.cloud_id AS cloud_id,
	c.cloud_ext_id AS cloud_ext_id,
	f.folder_id AS folder_id,
	f.folder_ext_id AS folder_ext_id
FROM
	dbaas.clouds c
	JOIN dbaas.folders f USING (cloud_id)
WHERE
	f.folder_ext_id = :fextid`,
	}

	querySelectFolderCoordsByClusterID = sqlutil.Stmt{
		Name: "SelectFolderCoordsByClusterID",
		Query: `
SELECT
	c.cloud_id AS cloud_id,
	c.cloud_ext_id AS cloud_ext_id,
	f.folder_id AS folder_id,
	f.folder_ext_id AS folder_ext_id,
	cl.actual_rev AS revision,
    cl.type
FROM
	dbaas.clouds c
	JOIN dbaas.folders f USING (cloud_id)
	JOIN dbaas.clusters cl USING (folder_id)
WHERE
	cl.cid = :cid
	AND code.match_visibility(cl, :visibility)`,
	}

	querySelectFolderCoordsByOperationID = sqlutil.Stmt{
		Name: "SelectFolderCoordsByOperationID",
		Query: `
SELECT
	c.cloud_id AS cloud_id,
	c.cloud_ext_id AS cloud_ext_id,
	f.folder_id AS folder_id,
	f.folder_ext_id AS folder_ext_id
FROM
	dbaas.clouds c,
    dbaas.folders f
WHERE
	c.cloud_id = f.cloud_id
    AND f.folder_id IN (
        SELECT folder_id
        FROM code.get_folder_id_by_operation(:opid) folder_id)`,
	}

	queryCreateFolder = sqlutil.Stmt{
		Name: "CreateFolder",
		Query: `
INSERT INTO dbaas.folders (
	folder_ext_id,
	cloud_id
) VALUES (
	:folder_ext_id,
	:cloud_id
) RETURNING
	folder_id,
	folder_ext_id,
	cloud_id`,
	}

	querySelectClusterRevisionByTime = sqlutil.Stmt{
		Name: "SelectClusterRevisionByTime",
		Query: `
SELECT max(rev) rev
FROM dbaas.clusters_changes
WHERE cid = :cid
AND committed_at <= :time`,
	}

	queryLockCluster = sqlutil.Stmt{
		Name: "LockCluster",
		Query: `
SELECT
    cid,
    name,
    type,
    env,
    created_at,
    network_id,
    status,
    pillar_value AS pillar,
    description,
    CAST(labels AS jsonb) AS labels,
	user_sgroup_ids,
    rev,
    host_group_ids,
	backup_schedule,
	deletion_protection
FROM code.lock_cluster(
    i_cid          => :cid,
    i_x_request_id => :x_request_id
)`,
	}

	querySelectCluster = sqlutil.Stmt{
		Name: "SelectCluster",
		Query: `
SELECT
	c.cid,
	c.name,
	c.type,
	c.env,
	c.created_at,
	c.network_id,
	c.status,
	coalesce(p.value, CAST('{}' AS jsonb)) AS pillar,
	description,
	CAST(code.get_cluster_labels(cid) AS jsonb) AS labels,
	actual_rev AS rev,
	ARRAY(SELECT sg_ext_id FROM dbaas.sgroups WHERE cid=c.cid AND sg_type='user' ORDER BY sg_ext_id) AS user_sgroup_ids,
    c.host_group_ids,
    coalesce(br.schedule, CAST('{}' AS jsonb)) AS backup_schedule,
    c.deletion_protection
FROM
	dbaas.clusters c
	LEFT JOIN dbaas.pillar p USING (cid)
	LEFT JOIN dbaas.backup_schedule br USING (cid)
WHERE
	c.cid = :cid
	AND code.match_visibility(c, :visibility)`,
	}

	querySelectClusterAtRevision = sqlutil.Stmt{
		Name: "SelectClusterAtRevision",
		Query: `
SELECT cr.cid,
       cr.name,
       c.type,
       c.env,
       c.created_at,
       cr.network_id,
       cr.status,
       coalesce(pr.value, CAST('{}' AS jsonb)) AS pillar,
       cr.description,
       cast(code.get_cluster_labels_at_rev(cr.cid, cr.rev) AS jsonb) AS labels,
       cr.rev,
	   ARRAY(SELECT sg_ext_id FROM dbaas.sgroups_revs sg WHERE sg.cid=cr.cid AND rev = cr.rev AND sg.sg_type='user' ORDER BY sg_ext_id) AS user_sgroup_ids,
       cr.host_group_ids,
       coalesce(br.schedule, CAST('{}' AS jsonb)) AS backup_schedule
FROM dbaas.clusters_revs cr
LEFT JOIN dbaas.maintenance_window_settings_revs mwsr USING (cid, rev)
JOIN dbaas.pillar_revs pr USING (cid, rev)
LEFT JOIN dbaas.backup_schedule_revs br USING (cid, rev)
JOIN dbaas.clusters c USING (cid)
WHERE cr.cid = :cid AND cr.rev = :revision`,
	}

	queryCompleteClusterChange = sqlutil.Stmt{
		Name: "CompleteClusterChange",
		Query: `
SELECT code.complete_cluster_change(
    i_cid => :cid,
    i_rev => :rev
)`,
	}

	querySelectSubClustersByClusterID = sqlutil.Stmt{
		Name: "SelectSubClustersByClusterID",
		Query: `
SELECT
    sc.subcid,
    sc.cid,
    sc.name,
    CAST(sc.roles AS text[]) AS roles,
    coalesce(pl.value, CAST('{}' AS jsonb)) AS pillar
FROM
    dbaas.subclusters sc
    LEFT JOIN dbaas.pillar pl USING (subcid)
WHERE
    sc.cid = :cid`,
	}

	querySelectSubClustersByClusterIDAtRevision = sqlutil.Stmt{
		Name: "SelectSubClustersByClusterIDAtRevision",
		Query: `
SELECT
    sc.subcid,
    sc.cid,
    sc.name,
    CAST(sc.roles AS text[]) AS roles,
    coalesce(pl.value, CAST('{}' AS jsonb)) AS pillar
FROM
    dbaas.subclusters_revs sc
    LEFT JOIN dbaas.pillar_revs pl USING (subcid, rev)
WHERE
    sc.cid = :cid
    AND sc.rev = :rev`,
	}

	queryCreateSubCluster = sqlutil.Stmt{
		Name: "CreateSubCluster",
		Query: `
SELECT subcid,
       cid,
       name,
       roles
  FROM code.add_subcluster(
      i_cid    => :cid,
      i_subcid => :subcid,
      i_name   => :name,
      i_roles  => CAST(:roles AS dbaas.role_type[]),
      i_rev    => :rev
)`,
	}

	queryDeleteSubCluster = sqlutil.Stmt{
		Name: "DeleteSubCluster",
		Query: `
SELECT code.delete_subcluster(
      i_cid    => :cid,
      i_subcid => :subcid,
      i_rev    => :rev
)`,
	}

	queryCreateKubernetesSubcluster = sqlutil.Stmt{
		Name: "CreateKubernetesSubcluster",
		Query: `
SELECT subcid,
       cid,
       name,
       roles
  FROM code.add_kubernetes_subcluster(
      i_cid    => :cid,
      i_subcid => :subcid,
      i_name   => :name,
      i_roles  => CAST(:roles AS dbaas.role_type[]),
      i_rev    => :rev
)`,
	}

	queryGetKubernetesNodeGroup = sqlutil.Stmt{
		Name: "GetKubernetesNodeGroup",
		Query: `
SELECT subcid,
       kubernetes_cluster_id,
       node_group_id
FROM dbaas.kubernetes_node_groups
WHERE
	subcid = :subcid
`,
	}

	queryCreateShard = sqlutil.Stmt{
		Name: "CreateShard",
		Query: `
SELECT subcid, shard_id, name
  FROM code.add_shard(
    i_subcid   => :subcid,
    i_shard_id => :shard_id,
    i_name     => :name,
    i_cid      => :cid,
    i_rev      => :rev
)`,
	}

	queryDeleteShard = sqlutil.Stmt{
		Name: "DeleteShard",
		Query: `
SELECT code.delete_shard(
    i_shard_id => :shard_id,
    i_cid      => :cid,
    i_rev      => :rev
)
`,
	}

	querySelectFlavorByName = sqlutil.Stmt{
		Name: "querySelectFlavorByName",
		Query: `
SELECT
    id,
    cpu_guarantee,
    cpu_limit,
    CAST((cpu_guarantee * 100 / cpu_limit) AS int) AS cpu_fraction,
    gpu_limit,
    memory_guarantee,
    memory_limit,
    network_guarantee,
    network_limit,
    io_limit,
	io_cores_limit,
    name,
    name as description,
    vtype,
    type,
    generation,
    platform_id
FROM
    dbaas.flavors
WHERE
    name = :flavor_name
`,
	}

	queryGetClustersByFolder = sqlutil.Stmt{
		Name: "queryGetClustersByFolder",
		Query: `
SELECT
    cid,
    name,
    type,
    env,
    created_at,
    pillar_value AS pillar,
    network_id,
    status,
    description,
    CAST(labels AS jsonb) AS labels,
	user_sgroup_ids,
    rev,
	host_group_ids,
    backup_schedule,
    deletion_protection
FROM code.get_clusters(
    i_folder_id       => :folder_id,
    i_cid             => :cid,
    i_cluster_name    => :cluster_name,
    i_env             => :env,
    i_cluster_type    => :cluster_type,
    i_page_token_name => :page_token_name,
    i_limit           => :limit,
    i_visibility      => :visibility
)`,
	}

	queryListAllClusterIDs = sqlutil.Stmt{
		Name: "queryListAllClusterIDs",
		Query: `
SELECT
	cid
FROM
	dbaas.clusters c
WHERE
	(:cluster_type IS NULL OR c.type = :cluster_type)
    AND code.match_visibility(c, :visibility)
    AND (:page_token_cid IS NULL OR c.cid > :page_token_cid)
ORDER BY cid
LIMIT :limit`,
	}

	queryGetShardsByShardID = sqlutil.Stmt{
		Name: "GetShardsByShardID",
		Query: `
SELECT
    s.subcid,
    s.shard_id,
    s.name,
    s.created_at,
    coalesce(pl.value, CAST('{}' AS jsonb)) AS pillar
FROM
    dbaas.shards s
    LEFT JOIN dbaas.pillar pl USING (shard_id)
WHERE
    s.shard_id = :shard_id
ORDER BY
    s.name`,
	}

	queryGetShardOnClusterByShardName = sqlutil.Stmt{
		Name: "GetShardByShardName",
		// language=SQL
		Query: `
SELECT
	s.subcid,
	s.shard_id,
	s.name,
	s.created_at,
	coalesce(pl.value, CAST('{}' AS jsonb)) AS pillar
FROM dbaas.shards s
JOIN dbaas.subclusters sc
	ON s.subcid = sc.subcid
LEFT JOIN dbaas.pillar pl
	ON s.shard_id = pl.shard_id
WHERE
    sc.cid = :cid AND s.name = :shard_name`,
	}

	queryGetShardsByClusterID = sqlutil.Stmt{
		Name: "queryGetShardsByClusterId",
		// language=SQL
		Query: `
SELECT
    s.subcid,
    s.shard_id,
    s.name,
	s.created_at,
	coalesce(pl.value, CAST('{}' AS jsonb)) AS pillar
FROM
    dbaas.shards s
JOIN dbaas.subclusters sc
	ON s.subcid = sc.subcid
LEFT JOIN dbaas.pillar pl
	ON s.shard_id = pl.shard_id
WHERE
    sc.cid = :cid
ORDER BY
    s.name`,
	}

	queryAddBackupSchedule = sqlutil.Stmt{
		Name: "queryAddBackupSchedule",
		Query: `
SELECT code.add_backup_schedule(
	i_cid      => :cid,
	i_rev      => :rev,
	i_schedule => :schedule
)`,
	}

	queryUpdateClusterName = sqlutil.Stmt{
		Name: "queryUpdateClusterName",
		Query: `
SELECT code.update_cluster_name(
    i_cid  => :cid,
    i_name => :name,
    i_rev  => :rev
)`,
	}

	queryUpdateClusterDescription = sqlutil.Stmt{
		Name: "queryUpdateClusterDescription",
		Query: `
SELECT code.update_cluster_description(
    i_cid         => :cid,
    i_description => :description,
    i_rev         => :rev
)`,
	}

	queryUpdateClusterFolder = sqlutil.Stmt{
		Name: "queryUpdateClusterFolder",
		Query: `
SELECT code.update_cluster_folder(
    i_cid         => :cid,
    i_folder_id   => :folder_id,
    i_rev         => :rev
)`,
	}

	querySetLabelsOnCluster = sqlutil.Stmt{
		Name: "querySetLabelsOnCluster",
		Query: `
SELECT code.set_labels_on_cluster(
    i_folder_id  => 0, /* param is not used within function */
    i_cid        => :cid,
	i_rev        => :rev,
    i_labels     => (SELECT array_agg(CAST((key, value) AS code.label))
FROM (SELECT * FROM jsonb_each_text(:labels)) AS l)
)`,
	}

	queryUpdateDeletionProtection = sqlutil.Stmt{
		Name: "queryUpdateDeletionProtection",
		Query: `
SELECT code.update_cluster_deletion_protection(
    i_cid                 => :cid,
    i_deletion_protection => :deletion_protection,
    i_rev                 => :rev
)`,
	}

	queryGetClusterQuotaUsage = sqlutil.Stmt{
		Name: "queryGetClusterQuotaUsage",
		Query: `
SELECT
    cpu,
    memory,
    ssd_space,
    hdd_space,
    clusters
FROM code.get_cluster_quota_usage(
    i_cid  => :cid
)`,
	}

	queryGetVersions = sqlutil.Stmt{
		Name: "queryGetVersions",
		Query: `
SELECT
	cid,
	subcid,
	shard_id,
	component,
	major_version,
	minor_version,
	package_version,
	edition
FROM	dbaas.versions
WHERE
	cid = :cid
`,
	}

	queryGetVersionsByRev = sqlutil.Stmt{
		Name: "queryGetVersionsByRev",
		Query: `
SELECT
	cid,
	subcid,
	shard_id,
	component,
	major_version,
	minor_version,
	package_version,
	edition
FROM	dbaas.versions_revs
WHERE
	cid = :cid
AND rev = :rev
`,
	}

	queryUseBackupService = sqlutil.Stmt{
		Name: "queryUseBackupService",
		Query: `
SELECT
	COALESCE(schedule->>'use_backup_service', 'false') as use_backup_service, 1 as foo
FROM	dbaas.backup_schedule
WHERE
	cid = :cid
`,
	}

	queryGetDefaultVersions = sqlutil.Stmt{
		Name: "queryGetDefaultVersions",
		Query: `
SELECT
	major_version,
	minor_version,
	name,
	edition,
	is_default,
	is_deprecated,
	updatable_to
FROM	dbaas.default_versions
WHERE
	type = :type AND
	component = :component AND
	env = :env
`,
	}

	querySetDefaultVersionCluster = sqlutil.Stmt{
		Name: "querySetDefaultVersionCluster",
		Query: `select code.set_default_versions(
		i_cid           => :cid,
		i_subcid        => null,
		i_shard_id      => null,
		i_ctype         => :ctype,
		i_env           => :env,
		i_major_version => :major_version,
		i_edition       => :edition,
		i_rev           => :rev
	)`,
	}

	querySelectMaintenanceInfo = sqlutil.Stmt{
		Name: "SelectMaintenanceInfo",
		// language=PostgreSQL
		Query: `
SELECT cl.cid,
       mws.day,
       mws.hour,
       mt.config_id,
       mt.create_ts AS created_at,
       mt.plan_ts AS delayed_until,
       mt.info
FROM dbaas.clusters cl
LEFT JOIN dbaas.maintenance_window_settings mws USING (cid)
LEFT JOIN dbaas.maintenance_tasks mt ON (mt.cid=cl.cid AND mt.status=cast('PLANNED' AS dbaas.maintenance_task_status))
WHERE (cl.cid = :cid)`,
	}

	querySetMaintenanceSettings = sqlutil.Stmt{
		Name: "SetMaintenanceSettings",
		// language=PostgreSQL
		Query: `
SELECT code.set_maintenance_window_settings(
    i_cid         => :cid,
    i_day         => :day,
    i_hour        => :hour,
    i_rev         => :rev
)`,
	}

	queryRescheduleMaintenanceTask = sqlutil.Stmt{
		Name: "RescheduleMaintenanceTask",
		// language=PostgreSQL
		Query: `
SELECT code.reschedule_maintenance_task(
    i_cid         => :cid,
    i_config_id   => :config_id,
    i_plan_ts     => :plan_ts
)`,
	}
)

func visibilityToDB(vis models.Visibility) string {
	switch vis {
	case models.VisibilityAll:
		return "all"
	case models.VisibilityVisibleOrDeleted:
		return "visible+deleted"
	case models.VisibilityVisible:
		return "visible"
	}

	panic(fmt.Sprintf("unknown visibility: %d", vis))
}

func (b *Backend) CreateCluster(ctx context.Context, reqid string, args models.CreateClusterArgs) (metadb.Cluster, error) {
	var c cluster
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&c)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryCreateCluster,
		map[string]interface{}{
			"cid":                 args.ClusterID,
			"folder_id":           args.FolderID,
			"name":                args.Name,
			"description":         args.Description,
			"type":                args.ClusterType.Stringified(),
			"env":                 args.Environment,
			"public_key":          args.PublicKey,
			"network_id":          args.NetworkID,
			"x_request_id":        reqid,
			"host_group_ids":      args.HostGroupIDs,
			"deletion_protection": args.DeletionProtection,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return metadb.Cluster{}, err
	}
	if count == 0 {
		return metadb.Cluster{}, sqlerrors.ErrNotFound
	}

	if !args.MaintenanceWindow.Anytime() {
		if err := b.SetMaintenanceWindowSettings(ctx, c.ClusterID, c.Revision, args.MaintenanceWindow); err != nil {
			return metadb.Cluster{}, err
		}
	}

	// TODO: create pillar and labels

	return clusterFromDB(c)
}

func (b *Backend) CreateSubCluster(ctx context.Context, args models.CreateSubClusterArgs) (metadb.SubCluster, error) {
	var s subCluster
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&s)
	}

	var roles []string
	for _, r := range args.Roles {
		roles = append(roles, r.Stringified())
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryCreateSubCluster,
		map[string]interface{}{
			"cid":    args.ClusterID,
			"subcid": args.SubClusterID,
			"name":   args.Name,
			"roles":  roles,
			"rev":    args.Revision,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return metadb.SubCluster{}, err
	}
	if count == 0 {
		return metadb.SubCluster{}, sqlerrors.ErrNotFound
	}

	return subClusterFromDB(s)
}

func (b *Backend) DeleteSubCluster(ctx context.Context, args models.DeleteSubClusterArgs) error {
	parser := func(rows *sqlx.Rows) error {
		return nil
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryDeleteSubCluster,
		map[string]interface{}{
			"cid":    args.ClusterID,
			"subcid": args.SubClusterID,
			"rev":    args.Revision,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return err
	}
	return nil
}

func (b *Backend) CreateKubernetesSubCluster(ctx context.Context, args models.CreateSubClusterArgs) (metadb.SubCluster, error) {
	var s subCluster
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&s)
	}

	var roles []string
	for _, r := range args.Roles {
		roles = append(roles, r.Stringified())
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryCreateKubernetesSubcluster,
		map[string]interface{}{
			"cid":    args.ClusterID,
			"subcid": args.SubClusterID,
			"name":   args.Name,
			"roles":  roles,
			"rev":    args.Revision,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return metadb.SubCluster{}, err
	}
	if count == 0 {
		return metadb.SubCluster{}, sqlerrors.ErrNotFound
	}

	return subClusterFromDB(s)
}

func (b *Backend) CreateShard(ctx context.Context, args models.CreateShardArgs) (metadb.Shard, error) {
	var s shard
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&s)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryCreateShard,
		map[string]interface{}{
			"cid":      args.ClusterID,
			"subcid":   args.SubClusterID,
			"name":     args.Name,
			"shard_id": args.ShardID,
			"rev":      args.Revision,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return metadb.Shard{}, err
	}
	if count == 0 {
		return metadb.Shard{}, sqlerrors.ErrNotFound
	}

	return shardFromDB(s)
}

func (b *Backend) DeleteShard(ctx context.Context, cid, shardid string, revision int64) error {
	parser := func(rows *sqlx.Rows) error {
		return nil
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryDeleteShard,
		map[string]interface{}{
			"cid":      cid,
			"shard_id": shardid,
			"rev":      revision,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return err
	}
	if count == 0 {
		return sqlerrors.ErrNotFound
	}

	return nil
}

func (b *Backend) FolderCoordsByFolderExtID(ctx context.Context, fExtID string) (metadb.FolderCoords, error) {
	return b.folderIDs(ctx, querySelectFolderCoordsByFolderExtID, map[string]interface{}{"fextid": fExtID})
}

func (b *Backend) FolderCoordsByClusterID(ctx context.Context, cid string, vis models.Visibility) (metadb.FolderCoords, int64, clusters.Type, error) {
	return b.folderIDsWithRevision(
		ctx,
		querySelectFolderCoordsByClusterID,
		map[string]interface{}{
			"cid":        cid,
			"visibility": visibilityToDB(vis),
		})
}

func (b *Backend) FolderCoordsByOperationID(ctx context.Context, opid string) (metadb.FolderCoords, error) {
	return b.folderIDs(ctx, querySelectFolderCoordsByOperationID, map[string]interface{}{"opid": opid})
}

func (b *Backend) CreateFolder(ctx context.Context, folderExtID string, cloudID int64) (metadb.FolderCoords, error) {
	_, err := sqlutil.QueryTx(
		ctx,
		queryCreateFolder,
		map[string]interface{}{
			"folder_ext_id": folderExtID,
			"cloud_id":      cloudID,
		},
		sqlutil.NopParser,
		b.logger,
	)
	if err != nil {
		return metadb.FolderCoords{}, err
	}

	return b.FolderCoordsByFolderExtID(ctx, folderExtID)
}

type folderIDs struct {
	CloudID     int64  `db:"cloud_id"`
	CloudExtID  string `db:"cloud_ext_id"`
	FolderID    int64  `db:"folder_id"`
	FolderExtID string `db:"folder_ext_id"`
}

func (b *Backend) folderIDs(ctx context.Context, query sqlutil.Stmt, args map[string]interface{}) (metadb.FolderCoords, error) {
	var res folderIDs
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&res)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		query,
		args,
		parser,
		b.logger,
	)
	if err != nil {
		return metadb.FolderCoords{}, err
	}
	if count == 0 {
		return metadb.FolderCoords{}, sqlerrors.ErrNotFound
	}

	return metadb.FolderCoords(res), nil
}

func (b *Backend) folderIDsWithRevision(ctx context.Context, query sqlutil.Stmt, args map[string]interface{}) (metadb.FolderCoords, int64, clusters.Type, error) {
	var res struct {
		folderIDs
		Revision int64  `db:"revision"`
		Type     string `db:"type"`
	}
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&res)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		query,
		args,
		parser,
		b.logger,
	)
	if err != nil {
		return metadb.FolderCoords{}, 0, clusters.TypeUnknown, err
	}
	if count == 0 {
		return metadb.FolderCoords{}, 0, clusters.TypeUnknown, sqlerrors.ErrNotFound
	}

	clusterType, err := clusters.ParseTypeStringified(res.Type)
	if err != nil {
		return metadb.FolderCoords{}, 0, clusters.TypeUnknown, sqlerrors.ErrNotFound
	}

	return metadb.FolderCoords(res.folderIDs), res.Revision, clusterType, nil
}

func (b *Backend) ClusterRevisionByTime(ctx context.Context, cid string, time time.Time) (int64, error) {
	var rev sql.NullInt64
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&rev)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectClusterRevisionByTime,
		map[string]interface{}{
			"cid":  cid,
			"time": time,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return 0, err
	}
	if count == 0 || !rev.Valid {
		return 0, sqlerrors.ErrNotFound
	}

	return rev.Int64, nil
}

func (b *Backend) LockCluster(ctx context.Context, cid, reqid string) (metadb.Cluster, error) {
	var c cluster
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&c)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryLockCluster,
		map[string]interface{}{
			"cid":          cid,
			"x_request_id": reqid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return metadb.Cluster{}, err
	}
	if count == 0 {
		return metadb.Cluster{}, sqlerrors.ErrNotFound
	}

	return clusterFromDB(c)
}

func (b *Backend) ClusterByClusterID(ctx context.Context, cid string, vis models.Visibility) (metadb.Cluster, error) {
	var c cluster
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&c)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectCluster,
		map[string]interface{}{
			"cid":        cid,
			"visibility": visibilityToDB(vis),
		},
		parser,
		b.logger,
	)
	if err != nil {
		return metadb.Cluster{}, err
	}
	if count == 0 {
		return metadb.Cluster{}, sqlerrors.ErrNotFound
	}

	return clusterFromDB(c)
}

func (b *Backend) ClusterByClusterIDAtRevision(ctx context.Context, cid string, rev int64) (metadb.Cluster, error) {
	var c cluster
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&c)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectClusterAtRevision,
		map[string]interface{}{
			"cid":      cid,
			"revision": rev,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return metadb.Cluster{}, err
	}
	if count == 0 {
		return metadb.Cluster{}, sqlerrors.ErrNotFound
	}

	return clusterFromDB(c)
}

func (b *Backend) MaintenanceInfoByClusterID(ctx context.Context, cid string) (clusters.MaintenanceInfo, error) {
	var mi maintenanceInfo
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&mi)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectMaintenanceInfo,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return clusters.MaintenanceInfo{}, err
	}

	return maintenanceInfoFromDB(mi, count > 0), nil
}

func (b *Backend) SetMaintenanceWindowSettings(ctx context.Context, cid string, rev int64, mw clusters.MaintenanceWindow) error {
	if !mw.Anytime() && mw.WeeklyMaintenanceWindow == nil {
		return nil
	}

	args := map[string]interface{}{
		"cid":  cid,
		"rev":  rev,
		"day":  sql.NullString{},
		"hour": sql.NullInt64{},
	}

	if mw.WeeklyMaintenanceWindow != nil {
		args["day"] = mw.Day
		args["hour"] = mw.Hour
	}

	_, err := sqlutil.QueryTx(
		ctx,
		querySetMaintenanceSettings,
		args,
		sqlutil.NopParser,
		b.logger,
	)

	return err
}

func (b *Backend) RescheduleMaintenanceTask(ctx context.Context, cid, configID string, maintenanceTime time.Time) error {
	args := map[string]interface{}{
		"cid":       cid,
		"config_id": configID,
		"plan_ts":   maintenanceTime,
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryRescheduleMaintenanceTask,
		args,
		sqlutil.NopParser,
		b.logger,
	)

	return err
}

func (b *Backend) ResourcePresetByExtID(ctx context.Context, extID string) (resources.Preset, error) {
	var rs resourcePreset
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&rs)
	}
	count, err := sqlutil.QueryTx(
		ctx,
		querySelectFlavorByName,
		map[string]interface{}{
			"flavor_name": extID,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return resources.Preset{}, err
	}
	if count == 0 {
		return resources.Preset{}, sqlerrors.ErrNotFound
	}

	return resourcePresetFromDB(rs)
}

func (b *Backend) SubClustersByClusterID(ctx context.Context, cid string) ([]metadb.SubCluster, error) {
	var scs []metadb.SubCluster
	parser := func(rows *sqlx.Rows) error {
		var p subCluster
		if err := rows.StructScan(&p); err != nil {
			return err
		}
		sc, err := subClusterFromDB(p)
		if err != nil {
			return err
		}
		scs = append(scs, sc)
		return nil
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectSubClustersByClusterID,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}
	if count == 0 {
		return nil, sqlerrors.ErrNotFound
	}

	return scs, nil
}

func (b *Backend) SubClustersByClusterIDAtRevision(ctx context.Context, cid string, rev int64) ([]metadb.SubCluster, error) {
	var scs []metadb.SubCluster
	parser := func(rows *sqlx.Rows) error {
		var p subCluster
		if err := rows.StructScan(&p); err != nil {
			return err
		}
		sc, err := subClusterFromDB(p)
		if err != nil {
			return err
		}
		scs = append(scs, sc)
		return nil
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectSubClustersByClusterIDAtRevision,
		map[string]interface{}{
			"cid": cid,
			"rev": rev,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}
	if count == 0 {
		return nil, sqlerrors.ErrNotFound
	}

	return scs, nil
}

func (b *Backend) KubernetesNodeGroup(ctx context.Context, subcid string) (metadb.KubernetesNodeGroup, error) {
	var nodeGroups []metadb.KubernetesNodeGroup

	parser := func(rows *sqlx.Rows) error {
		var p kubernetesNodeGroup
		if err := rows.StructScan(&p); err != nil {
			return err
		}
		nodeGroup, err := nodeGroupFromDB(p)
		if err != nil {
			return err
		}

		nodeGroups = append(nodeGroups, nodeGroup)
		return nil
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryGetKubernetesNodeGroup,
		map[string]interface{}{
			"subcid": subcid,
		},
		parser,
		b.logger,
	)
	if count == 0 {
		return metadb.KubernetesNodeGroup{}, sqlerrors.ErrNotFound
	}
	if err != nil {
		return metadb.KubernetesNodeGroup{}, err
	}
	return nodeGroups[0], nil
}

func (b *Backend) CompleteClusterChange(ctx context.Context, cid string, revision int64) error {
	_, err := sqlutil.QueryTx(
		ctx,
		queryCompleteClusterChange,
		map[string]interface{}{
			"cid": cid,
			"rev": revision,
		},
		sqlutil.NopParser,
		b.logger,
	)

	return err
}

func (b *Backend) Clusters(ctx context.Context, args models.ListClusterArgs) ([]metadb.Cluster, error) {
	var cs []metadb.Cluster
	parser := func(rows *sqlx.Rows) error {
		var p cluster
		if err := rows.StructScan(&p); err != nil {
			return err
		}
		c, err := clusterFromDB(p)
		if err != nil {
			return err
		}
		cs = append(cs, c)
		return nil
	}

	clusterType := pgtype.Text{Status: pgtype.Null}
	if args.ClusterType != clusters.TypeUnknown {
		clusterType = pgtype.Text{
			String: args.ClusterType.Stringified(),
			Status: pgtype.Present,
		}
	}
	_, err := sqlutil.QueryTx(
		ctx,
		queryGetClustersByFolder,
		map[string]interface{}{
			"folder_id":       args.FolderID,
			"cid":             sql.NullString(args.ClusterID),
			"cluster_name":    sql.NullString(args.Name),
			"cluster_type":    clusterType,
			"env":             nil, // TODO: add env type
			"limit":           sql.NullInt64{Int64: pagination.SanePageSize(args.Limit.Int64), Valid: args.Limit.Valid},
			"page_token_name": sql.NullString(args.PageTokenName),
			"visibility":      visibilityToDB(args.Visibility),
		},
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}

	return cs, nil
}

func (b *Backend) ListClusterIDs(ctx context.Context, args models.ListClusterIDsArgs) ([]string, error) {
	type clusterID struct {
		ClusterID string `db:"cid"`
	}
	var clusterIDs []string

	parser := func(rows *sqlx.Rows) error {
		var p clusterID
		if err := rows.StructScan(&p); err != nil {
			return err
		}
		clusterIDs = append(clusterIDs, p.ClusterID)
		return nil
	}

	queryArgs := map[string]interface{}{
		"cluster_type":   args.ClusterType.Stringified(),
		"visibility":     visibilityToDB(args.Visibility),
		"limit":          args.Limit,
		"page_token_cid": nil,
	}
	if args.AfterClusterID.Valid {
		queryArgs["page_token_cid"] = args.AfterClusterID.String
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryListAllClusterIDs,
		queryArgs,
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}

	return clusterIDs, nil
}

func (b *Backend) ShardByShardID(ctx context.Context, shardid string) (metadb.Shard, error) {
	var s shard
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&s)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryGetShardsByShardID,
		map[string]interface{}{
			"shard_id": shardid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return metadb.Shard{}, err
	}
	if count == 0 {
		return metadb.Shard{}, sqlerrors.ErrNotFound
	}

	return shardFromDB(s)
}

func (b *Backend) ShardByShardName(ctx context.Context, shardName, clusterID string) (metadb.Shard, error) {
	var s shard
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&s)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryGetShardOnClusterByShardName,
		map[string]interface{}{
			"shard_name": shardName,
			"cid":        clusterID,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return metadb.Shard{}, err
	}
	if count == 0 {
		return metadb.Shard{}, sqlerrors.ErrNotFound
	}

	return shardFromDB(s)
}

func (b *Backend) ShardsByClusterID(ctx context.Context, cid string) ([]metadb.Shard, error) {
	var shards []metadb.Shard
	parser := func(rows *sqlx.Rows) error {
		var s shard
		if err := rows.StructScan(&s); err != nil {
			return err
		}
		shard, err := shardFromDB(s)
		if err != nil {
			return err
		}
		shards = append(shards, shard)
		return nil
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryGetShardsByClusterID,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return []metadb.Shard{}, err
	}
	if count == 0 {
		return nil, sqlerrors.ErrNotFound
	}

	return shards, nil
}

func (b *Backend) AddBackupSchedule(ctx context.Context, cid string, schedule bmodels.BackupSchedule, revision int64) error {
	marshaledSchedule, err := json.Marshal(schedule)
	if err != nil {
		return xerrors.Errorf("failed to marshal cluster backup schedule to raw form: %w", err)
	}

	_, err = sqlutil.QueryTx(
		ctx,
		queryAddBackupSchedule,
		map[string]interface{}{
			"cid":      cid,
			"schedule": string(marshaledSchedule),
			"rev":      revision,
		},
		sqlutil.NopParser,
		b.logger,
	)

	return err
}

func (b *Backend) UpdateClusterName(ctx context.Context, cid string, name string, revision int64) error {
	_, err := sqlutil.QueryTx(
		ctx,
		queryUpdateClusterName,
		map[string]interface{}{
			"cid":  cid,
			"name": name,
			"rev":  revision,
		},
		sqlutil.NopParser,
		b.logger,
	)

	return err
}

func (b *Backend) UpdateClusterDescription(ctx context.Context, cid string, description string, revision int64) error {
	_, err := sqlutil.QueryTx(
		ctx,
		queryUpdateClusterDescription,
		map[string]interface{}{
			"cid":         cid,
			"description": description,
			"rev":         revision,
		},
		sqlutil.NopParser,
		b.logger,
	)

	return err
}

func (b *Backend) UpdateClusterFolder(ctx context.Context, cid string, folderID, revision int64) error {
	_, err := sqlutil.QueryTx(
		ctx,
		queryUpdateClusterFolder,
		map[string]interface{}{
			"cid":       cid,
			"folder_id": folderID,
			"rev":       revision,
		},
		sqlutil.NopParser,
		b.logger,
	)

	return err
}

func (b *Backend) SetLabelsOnCluster(ctx context.Context, cid string, labels map[string]string, revision int64) error {
	if labels == nil {
		labels = map[string]string{}
	}
	jsonBytes, err := json.Marshal(labels)
	if err != nil {
		return xerrors.Errorf("failed to marshal labels to json form: %w", err)
	}
	_, err = sqlutil.QueryTx(
		ctx,
		querySetLabelsOnCluster,
		map[string]interface{}{
			"cid":    cid,
			"rev":    revision,
			"labels": string(jsonBytes),
		},
		sqlutil.NopParser,
		b.logger,
	)

	return err
}

func (b *Backend) UpdateDeletionProtection(ctx context.Context, cid string, deletionProtection bool, revision int64) error {
	_, err := sqlutil.QueryTx(
		ctx,
		queryUpdateDeletionProtection,
		map[string]interface{}{
			"cid":                 cid,
			"deletion_protection": deletionProtection,
			"rev":                 revision,
		},
		sqlutil.NopParser,
		b.logger,
	)

	return err
}

func (b *Backend) GetClusterQuotaUsage(ctx context.Context, cid string) (metadb.Resources, error) {
	var res metadb.Resources

	parser := func(rows *sqlx.Rows) error {
		var q quotaUsage
		err := rows.StructScan(&q)
		if err != nil {
			return err
		}

		res, err = quotaUsageFromDB(q)
		if err != nil {
			return err
		}

		return nil
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryGetClusterQuotaUsage,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return metadb.Resources{}, err
	}

	return res, err
}

func (b *Backend) GetDefaultVersions(ctx context.Context, clusterType clusters.Type, env environment.SaltEnv, component string) ([]console.DefaultVersion, error) {
	versions := []console.DefaultVersion{}

	parser := func(rows *sqlx.Rows) error {
		var v defaultVersion
		err := rows.StructScan(&v)
		if err != nil {
			return err
		}

		version, err := defaultVersionFromDB(v)
		if err != nil {
			return err
		}

		versions = append(versions, version)
		return nil
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryGetDefaultVersions,
		map[string]interface{}{
			"type":      clusterType.Stringified(),
			"component": component,
			"env":       env,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}

	return versions, nil
}

func (b *Backend) SetDefaultVersionCluster(ctx context.Context, cid string,
	ctype clusters.Type, env environment.SaltEnv, majorVersion string, edition string, revision int64) error {
	_, err := sqlutil.QueryTx(
		ctx,
		querySetDefaultVersionCluster,
		map[string]interface{}{
			"cid":           cid,
			"ctype":         ctype.Stringified(),
			"env":           env,
			"major_version": majorVersion,
			"edition":       edition,
			"rev":           revision,
		},
		sqlutil.NopParser,
		b.logger,
	)

	return err
}

func (b *Backend) ClusterUsesBackupService(ctx context.Context, cid string) (bool, error) {
	type scheduleMeta struct {
		UseBackupService bool `db:"use_backup_service"`
		Foo              int  `db:"foo"`
	}

	var res scheduleMeta

	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&res)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryUseBackupService,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return false, err
	}
	if count == 0 {
		return false, sqlerrors.ErrNotFound
	}

	return res.UseBackupService, nil

}

func (b *Backend) GetClusterVersions(ctx context.Context, cid string) ([]console.Version, error) {
	return b.GetClusterVersionsCommon(ctx, cid, 0, false)
}

func (b *Backend) GetClusterVersionsAtRevision(ctx context.Context, cid string, rev int64) ([]console.Version, error) {
	return b.GetClusterVersionsCommon(ctx, cid, rev, true)
}

func (b *Backend) GetClusterVersionsCommon(ctx context.Context, cid string, rev int64, useRevision bool) ([]console.Version, error) {
	var versions []console.Version

	parser := func(rows *sqlx.Rows) error {
		var v version
		err := rows.StructScan(&v)
		if err != nil {
			return err
		}

		version, err := versionFromDB(v)
		if err != nil {
			return err
		}

		versions = append(versions, version)
		return nil
	}

	args := make(map[string]interface{})
	args["cid"] = cid
	query := queryGetVersions
	if useRevision {
		args["rev"] = rev
		query = queryGetVersionsByRev
	}
	_, err := sqlutil.QueryTx(
		ctx,
		query,
		args,
		parser,
		b.logger,
	)
	if err != nil {
		return []console.Version{}, err
	}

	return versions, nil
}
