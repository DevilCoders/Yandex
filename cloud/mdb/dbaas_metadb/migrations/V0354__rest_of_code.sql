

--head/code/05_finished_operation_task_type.sql
CREATE OR REPLACE FUNCTION code.finished_operation_task_type() RETURNS text AS $$
SELECT 'finished_before_starting'::text;
$$ LANGUAGE SQL IMMUTABLE;

--head/code/10_check_pillar_key.sql
CREATE OR REPLACE FUNCTION code.check_pillar_key(
	i_key code.pillar_key
) RETURNS void AS $$
BEGIN
-- Since PostgreSQL 11. We can CREATE DOMAIN on composite type, instead that function.
-- But we don't upgrade MetaDB yet.
--
-- CREATE DOMAIN code.pillar_key AS code.pillar_key_record
--    CONSTRAINT all_keys_is_not_null NOT NULL
--    CONSTRAINT exists_only_one_not_null_key CHECK (
--         (
--             ((VALUE).cid IS NOT NULL)::int +
--             ((VALUE).subcid IS NOT NULL)::int +
--             ((VALUE).shard_id IS NOT NULL)::int +
--             ((VALUE).fqdn IS NOT NULL)::int
--         ) = 1
--    );
    IF $1 IS NULL THEN
        RAISE EXCEPTION 'All pillar-components are null: %', to_json($1);
    END IF;

	IF ((($1).cid      IS NOT NULL)::int +
        (($1).subcid   IS NOT NULL)::int +
        (($1).shard_id IS NOT NULL)::int +
        (($1).fqdn     IS NOT NULL)::int) != 1
	THEN
        RAISE EXCEPTION 'Only one pillar-key component should be defined: %', to_json($1);
    END IF;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

--head/code/10_make_pillar_key.sql
CREATE OR REPLACE FUNCTION code.make_pillar_key(
    i_cid      text DEFAULT NULL,
    i_subcid   text DEFAULT NULL,
    i_shard_id text DEFAULT NULL,
    i_fqdn     text DEFAULT NULL
) RETURNS code.pillar_key
AS $$
SELECT (i_cid, i_subcid, i_shard_id, i_fqdn)::code.pillar_key;
$$ LANGUAGE SQL IMMUTABLE;

--head/code/10_round_cpu_quota.sql
CREATE OR REPLACE FUNCTION code.round_cpu_quota(real)
RETURNS real AS $$
SELECT cast(round(cast($1 AS numeric), 2) AS real);
$$ LANGUAGE SQL IMMUTABLE;

--head/code/10_task_type_actions.sql
CREATE OR REPLACE FUNCTION code.task_type_action_map()
RETURNS TABLE (
    task_type text,
    action dbaas.action
) AS $$
SELECT task_type::text,
       action::dbaas.action
  FROM (
    VALUES
        ('postgresql_cluster_stop', 'cluster-stop'),
        ('clickhouse_cluster_stop', 'cluster-stop'),
        ('mongodb_cluster_stop', 'cluster-stop'),
        ('mysql_cluster_stop', 'cluster-stop'),
        ('sqlserver_cluster_stop', 'cluster-stop'),
        ('greenplum_cluster_stop', 'cluster-stop'),
        ('redis_cluster_stop', 'cluster-stop'),
        ('hadoop_cluster_stop', 'cluster-stop'),
        ('kafka_cluster_stop', 'cluster-stop'),
        ('elasticsearch_cluster_stop', 'cluster-stop'),
        ('postgresql_cluster_start', 'cluster-start'),
        ('clickhouse_cluster_start', 'cluster-start'),
        ('mongodb_cluster_start', 'cluster-start'),
        ('mysql_cluster_start', 'cluster-start'),
        ('sqlserver_cluster_start', 'cluster-start'),
        ('greenplum_cluster_start', 'cluster-start'),
        ('redis_cluster_start', 'cluster-start'),
        ('hadoop_cluster_start', 'cluster-start'),
        ('kafka_cluster_start', 'cluster-start'),
        ('elasticsearch_cluster_start', 'cluster-start'),
        ('postgresql_cluster_create', 'cluster-create'),
        ('clickhouse_cluster_create', 'cluster-create'),
        ('mongodb_cluster_create', 'cluster-create'),
        ('mysql_cluster_create', 'cluster-create'),
        ('sqlserver_cluster_create', 'cluster-create'),
        ('greenplum_cluster_create', 'cluster-create'),
        ('redis_cluster_create', 'cluster-create'),
        ('hadoop_cluster_create', 'cluster-create'),
        ('kafka_cluster_create', 'cluster-create'),
        ('elasticsearch_cluster_create', 'cluster-create'),
        ('postgresql_cluster_restore', 'cluster-create'),
        ('clickhouse_cluster_restore', 'cluster-create'),
        ('mongodb_cluster_restore', 'cluster-create'),
        ('mysql_cluster_restore', 'cluster-create'),
        ('sqlserver_cluster_restore', 'cluster-create'),
        ('greenplum_cluster_restore', 'cluster-create'),
        ('redis_cluster_restore', 'cluster-create'),
        ('elasticsearch_cluster_restore', 'cluster-create'),
        ('postgresql_cluster_delete', 'cluster-delete'),
        ('clickhouse_cluster_delete', 'cluster-delete'),
        ('mongodb_cluster_delete', 'cluster-delete'),
        ('mysql_cluster_delete', 'cluster-delete'),
        ('sqlserver_cluster_delete', 'cluster-delete'),
        ('greenplum_cluster_delete', 'cluster-delete'),
        ('redis_cluster_delete', 'cluster-delete'),
        ('hadoop_cluster_delete', 'cluster-delete'),
        ('kafka_cluster_delete', 'cluster-delete'),
        ('elasticsearch_cluster_delete', 'cluster-delete'),
        ('postgresql_cluster_delete_metadata', 'cluster-delete-metadata'),
        ('clickhouse_cluster_delete_metadata', 'cluster-delete-metadata'),
        ('mongodb_cluster_delete_metadata', 'cluster-delete-metadata'),
        ('mysql_cluster_delete_metadata', 'cluster-delete-metadata'),
        ('sqlserver_cluster_delete_metadata', 'cluster-delete-metadata'),
        ('greenplum_cluster_delete_metadata', 'cluster-delete-metadata'),
        ('redis_cluster_delete_metadata', 'cluster-delete-metadata'),
        ('kafka_cluster_delete_metadata', 'cluster-delete-metadata'),
        ('elasticsearch_cluster_delete_metadata', 'cluster-delete-metadata'),
        ('postgresql_cluster_purge', 'cluster-purge'),
        ('clickhouse_cluster_purge', 'cluster-purge'),
        ('mongodb_cluster_purge', 'cluster-purge'),
        ('mysql_cluster_purge', 'cluster-purge'),
        ('sqlserver_cluster_purge', 'cluster-purge'),
        ('greenplum_cluster_purge', 'cluster-purge'),
        ('redis_cluster_purge', 'cluster-purge'),
        ('kafka_cluster_purge', 'cluster-purge'),
        ('elasticsearch_cluster_purge', 'cluster-purge'),
        ('clickhouse_cluster_maintenance', 'cluster-maintenance'),
        ('kafka_cluster_maintenance', 'cluster-maintenance'),
        ('mongodb_cluster_maintenance', 'cluster-maintenance'),
        ('mysql_cluster_maintenance', 'cluster-maintenance'),
        ('postgresql_cluster_maintenance', 'cluster-maintenance'),
        ('redis_cluster_maintenance', 'cluster-maintenance'),
        ('greenplum_cluster_maintenance', 'cluster-maintenance'),
        ('elasticsearch_cluster_maintenance', 'cluster-maintenance'),
        ('postgresql_cluster_online_resetup', 'cluster-resetup'),
        ('postgresql_cluster_offline_resetup', 'cluster-offline-resetup'),
        ('clickhouse_cluster_online_resetup', 'cluster-resetup'),
        ('clickhouse_cluster_offline_resetup', 'cluster-offline-resetup'),
        ('mongodb_cluster_online_resetup', 'cluster-resetup'),
        ('mongodb_cluster_offline_resetup', 'cluster-offline-resetup'),
        ('redis_cluster_online_resetup', 'cluster-resetup'),
        ('redis_cluster_offline_resetup', 'cluster-offline-resetup'),
        ('mysql_cluster_online_resetup', 'cluster-resetup'),
        ('mysql_cluster_offline_resetup', 'cluster-offline-resetup'),
        ('sqlserver_cluster_online_resetup', 'cluster-resetup'),
        ('sqlserver_cluster_offline_resetup', 'cluster-offline-resetup'),
        ('hadoop_cluster_online_resetup', 'cluster-resetup'),
        ('hadoop_cluster_offline_resetup', 'cluster-offline-resetup'),
        ('kafka_cluster_online_resetup', 'cluster-resetup'),
        ('kafka_cluster_offline_resetup', 'cluster-offline-resetup'),
        ('elasticsearch_cluster_online_resetup', 'cluster-resetup'),
        ('elasticsearch_cluster_offline_resetup', 'cluster-offline-resetup'),
        ('noop', 'noop')
) AS t(task_type, action);
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.task_type_action(
    i_task_type text
) RETURNS dbaas.action AS $$
DECLARE
    v_action dbaas.action;
BEGIN
    SELECT action
      INTO v_action
      FROM code.task_type_action_map()
     WHERE task_type = i_task_type;

    IF found THEN
        RETURN v_action;
    END IF;
    RETURN 'cluster-modify'::dbaas.action;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

--head/code/15_cluster_status_transitions.sql
CREATE OR REPLACE FUNCTION code.cluster_status_acquire_transitions()
RETURNS TABLE (
    from_status dbaas.cluster_status,
    to_status dbaas.cluster_status,
    action dbaas.action
) AS $$
SELECT from_status::dbaas.cluster_status,
       to_status::dbaas.cluster_status,
       action::dbaas.action
  FROM (
    VALUES
        ('STOPPED', 'RESTORING-OFFLINE', 'cluster-offline-resetup'),
        ('RUNNING', 'RESTORING-ONLINE', 'cluster-resetup'),
        ('RESTORE-OFFLINE-ERROR', 'RESTORING-OFFLINE', 'cluster-offline-resetup'),
        ('RESTORE-ONLINE-ERROR', 'RESTORING-ONLINE', 'cluster-resetup'),
        ('STOP-ERROR', 'STOPPING', 'cluster-stop'),
        ('START-ERROR', 'STARTING', 'cluster-start'),
        ('CREATE-ERROR', 'CREATING', 'cluster-create'),
        ('RUNNING', 'MODIFYING', 'cluster-modify'),
        ('MODIFY-ERROR', 'MODIFYING', 'cluster-modify'),
        ('DELETE-ERROR', 'DELETING', 'cluster-delete'),
        ('PURGE-ERROR', 'PURGING', 'cluster-purge'),
        ('DELETED', 'METADATA-DELETING', 'cluster-delete-metadata'),
        ('METADATA-DELETE-ERROR', 'METADATA-DELETING', 'cluster-delete-metadata'),
        -- cluster-purge tasks are delayed,
        -- so status is switched on task acquirement
        ('METADATA-DELETED', 'PURGING', 'cluster-purge'),
        ('DELETED', 'PURGING', 'cluster-purge'),
        ('RUNNING', 'MODIFYING', 'cluster-maintenance'),
        ('MODIFY-ERROR', 'MODIFYING', 'cluster-maintenance'),
        -- delayed tasks should have same status transitions
        -- so interrupted tasks can be acquired
        ('PURGING', 'PURGING', 'cluster-purge'),
        ('MODIFYING', 'MODIFYING', 'cluster-maintenance'),
        ('METADATA-DELETING', 'METADATA-DELETING', 'cluster-delete-metadata'),
        ('STOPPED', 'MAINTAINING-OFFLINE', 'cluster-maintenance'),
        -- failed offline maintenance can be restart
        ('MAINTAIN-OFFLINE-ERROR', 'MAINTAINING-OFFLINE', 'cluster-maintenance'),
        -- running maintenance cat be released and restarted by another worker
        ('MAINTAINING-OFFLINE', 'MAINTAINING-OFFLINE', 'cluster-maintenance')
       ) AS t(from_status, to_status, action);
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.cluster_status_finish_transitions()
RETURNS TABLE (
    from_status dbaas.cluster_status,
    to_status dbaas.cluster_status,
    action dbaas.action,
    result boolean
) AS $$
SELECT from_status::dbaas.cluster_status,
       to_status::dbaas.cluster_status,
       action::dbaas.action,
       result::boolean
  FROM (
    VALUES
        ('CREATING', 'RUNNING', 'cluster-create', true),
        ('CREATING', 'CREATE-ERROR', 'cluster-create', false),
        ('STOPPING', 'STOPPED', 'cluster-stop', true),
        ('STOPPING', 'STOP-ERROR', 'cluster-stop', false),
        ('STARTING', 'RUNNING', 'cluster-start', true),
        ('STARTING', 'START-ERROR', 'cluster-start', false),
        ('MODIFYING', 'RUNNING', 'cluster-modify', true),
        ('MODIFYING', 'MODIFY-ERROR', 'cluster-modify', false),
        ('RESTORING-ONLINE', 'RUNNING', 'cluster-resetup', true),
        ('RESTORING-ONLINE', 'RESTORE-ONLINE-ERROR', 'cluster-resetup', false),
        ('RESTORING-OFFLINE', 'STOPPED', 'cluster-offline-resetup', true),
        ('RESTORING-OFFLINE', 'RESTORE-OFFLINE-ERROR', 'cluster-offline-resetup', false),
        ('MODIFYING', 'RUNNING', 'cluster-maintenance', true),
        ('MODIFYING', 'MODIFY-ERROR', 'cluster-maintenance', false),
        ('DELETING', 'DELETED', 'cluster-delete', true),
        ('DELETING', 'DELETE-ERROR', 'cluster-delete', false),
        ('METADATA-DELETING', 'METADATA-DELETED', 'cluster-delete-metadata', true),
        ('METADATA-DELETING', 'METADATA-DELETE-ERROR', 'cluster-delete-metadata', false),
        ('PURGING', 'PURGED', 'cluster-purge', true),
        ('PURGING', 'PURGE-ERROR', 'cluster-purge', false),
        ('MAINTAINING-OFFLINE', 'STOPPED', 'cluster-maintenance', true),
        ('MAINTAINING-OFFLINE', 'MAINTAIN-OFFLINE-ERROR', 'cluster-maintenance', false)
) AS t(from_status, to_status, action, result);
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.cluster_status_add_transitions()
RETURNS TABLE (
    from_status dbaas.cluster_status,
    to_status dbaas.cluster_status,
    action dbaas.action
) AS $$
SELECT from_status::dbaas.cluster_status,
       to_status::dbaas.cluster_status,
       action::dbaas.action
  FROM (
    VALUES
        ('CREATING', 'CREATING', 'cluster-create'),
        ('RUNNING', 'MODIFYING', 'cluster-modify'),
        ('RUNNING', 'STOPPING', 'cluster-stop'),
        ('STOPPED', 'STARTING', 'cluster-start'),
        ('RUNNING', 'RESTORING-ONLINE', 'cluster-resetup'),
        ('STOPPED', 'RESTORING-OFFLINE', 'cluster-offline-resetup'),
        ('RUNNING', 'DELETING', 'cluster-delete'),
        ('STOPPED', 'DELETING', 'cluster-delete'),
        ('MODIFYING', 'MODIFYING', 'noop'),
        ('CREATE-ERROR', 'DELETING', 'cluster-delete'),
        ('MODIFY-ERROR', 'DELETING', 'cluster-delete'),
        ('RESTORE-ONLINE-ERROR', 'DELETING', 'cluster-delete'),
        ('RESTORE-OFFLINE-ERROR', 'DELETING', 'cluster-delete'),
        ('STOP-ERROR', 'DELETING', 'cluster-delete'),
        ('START-ERROR', 'DELETING', 'cluster-delete'),
        ('DELETING', 'DELETING', 'cluster-delete-metadata'),
        ('MAINTAIN-OFFLINE-ERROR', 'DELETING', 'cluster-delete')
) AS t(from_status, to_status, action);
$$ LANGUAGE SQL IMMUTABLE;

--head/code/15_label_casts.sql

CREATE OR REPLACE FUNCTION code.labels_from_jsonb(
    ld jsonb
) RETURNS code.label[] AS $$
SELECT ARRAY(
    SELECT (key, value)::code.label
        FROM jsonb_each_text(ld)
);
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.labels_to_jsonb(
    ld code.label[]
) RETURNS jsonb AS $$
SELECT coalesce(js, '{}'::jsonb)
  FROM (
      SELECT jsonb_object_agg(key, value) js
        FROM unnest(ld)
  ) x;
$$ LANGUAGE SQL IMMUTABLE;

DROP CAST IF EXISTS (jsonb AS code.label[]);
CREATE CAST (jsonb AS code.label[]) WITH FUNCTION code.labels_from_jsonb(jsonb);
DROP CAST IF EXISTS (code.label[] AS jsonb);
CREATE CAST (code.label[] AS jsonb) WITH FUNCTION code.labels_to_jsonb(code.label[]);

--head/code/15_task_status.sql
CREATE OR REPLACE FUNCTION code.task_status(
    q dbaas.worker_queue
) RETURNS code.operation_status AS $$
SELECT
    (CASE
    WHEN q.end_ts IS NULL AND q.start_ts IS NULL
        THEN 'PENDING'
    WHEN q.start_ts IS NOT NULL AND q.end_ts IS NULL
        THEN 'RUNNING'
    WHEN q.start_ts IS NOT NULL AND q.end_ts IS NOT NULL AND q.result IS FALSE
        THEN 'FAILED'
    WHEN q.start_ts IS NOT NULL AND q.end_ts IS NOT NULL AND q.result IS TRUE
        THEN 'DONE'
    END)::code.operation_status;
$$ LANGUAGE SQL IMMUTABLE;

--head/code/20_as_cluster_with_labels.sql
CREATE OR REPLACE FUNCTION code.as_cluster_with_labels(
    c   dbaas.clusters,
    b   jsonb,
    pl  jsonb,
    l   code.label[],
    mws dbaas.maintenance_window_settings,
    mt  dbaas.maintenance_tasks,
    sgs text[]
) RETURNS code.cluster_with_labels AS $$
SELECT
    c.cid,
    c.actual_rev,
    c.next_rev,
    c.name,
    c.type,
    c.folder_id,
    c.env,
    c.created_at,
    c.status,
    coalesce(pl, '{}'),
    c.network_id,
    c.description,
    coalesce(l, '{}'::code.label[]),
    mws.day AS mw_day,
    mws.hour AS mw_hour,
    mt.config_id AS mw_config_id,
    mt.max_delay AS mw_max_delay,
    mt.plan_ts AS mw_delayed_until,
    mt.create_ts AS mw_create_ts,
    mt.info AS mw_info,
    json_build_object(
        'day', mws.day,
        'hour', mws.hour
    )::jsonb,
    coalesce(b, '{}'),
    coalesce(sgs, '{}'),
    coalesce(c.host_group_ids, '{}'),
    c.deletion_protection;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.as_cluster_with_labels(
    dbaas.clusters
) RETURNS code.cluster_with_labels AS $$
SELECT code.as_cluster_with_labels(
    $1,
    (SELECT backup_schedule.schedule FROM dbaas.backup_schedule WHERE cid = ($1).cid),
    (SELECT pillar.value FROM dbaas.pillar WHERE cid = ($1).cid),
    (SELECT array_agg(
              (label_key, label_value)::code.label
              ORDER BY label_key, label_value) la
      FROM dbaas.cluster_labels cl
     WHERE cl.cid = ($1).cid),
    mws,
    mt,
    ARRAY(SELECT sg_ext_id
            FROM dbaas.sgroups
           WHERE sgroups.cid=($1).cid AND sg_type='user'
           ORDER BY sg_ext_id)
)
FROM (VALUES(($1).cid)) AS c (cid)
LEFT JOIN dbaas.maintenance_window_settings mws USING (cid)
LEFT JOIN dbaas.maintenance_tasks mt ON ($1).cid = mt.cid AND mt.status='PLANNED'::dbaas.maintenance_task_status;
$$ LANGUAGE SQL STABLE;

--head/code/20_as_operation.sql
CREATE OR REPLACE FUNCTION code.as_operation(
    q dbaas.worker_queue,
    c dbaas.clusters
) RETURNS code.operation AS $$
SELECT
    q.task_id,
    coalesce(q.cid, (q.task_args->>'cid')::text),
    q.cid,
    c.type,
    c.env,
    q.operation_type,
    q.created_by,
    q.create_ts,
    q.start_ts,
    coalesce(q.end_ts, q.start_ts, q.create_ts),
    code.task_status(q) AS status,
    q.metadata,
    q.task_args,
    q.hidden,
    q.required_task_id,
    q.errors;
$$ LANGUAGE SQL IMMUTABLE;

--head/code/20_as_worker_task.sql
CREATE OR REPLACE FUNCTION code.as_worker_task(
    q dbaas.worker_queue, f dbaas.folders
) RETURNS code.worker_task AS $$
SELECT
    q.task_id,
    q.cid,
    q.task_type,
    q.task_args,
    q.created_by,
    q.context,
    f.folder_ext_id,
    q.hidden,
    q.timeout,
    q.tracing
$$ LANGUAGE SQL IMMUTABLE;

--head/code/20_describe_not_acquired_task.sql
CREATE OR REPLACE FUNCTION code.describe_not_acquired_task(
    i_worker_id code.worker_id,
    i_task_id   code.task_id
) RETURNS text AS $$
DECLARE
    v_task dbaas.worker_queue;
BEGIN
    SELECT *
      INTO v_task
      FROM dbaas.worker_queue
     WHERE task_id = i_task_id;

    IF found AND v_task.unmanaged THEN
        RETURN 'this task is unmanaged';
    END IF;

    IF found THEN
        RETURN format(
            'another worker %L != %L acquire it',
            v_task.worker_id,
            i_worker_id
        );
    END IF;
    RETURN 'cause no task with this id found';
END;
$$ LANGUAGE plpgsql STABLE;

--head/code/20_format_cloud.sql
CREATE OR REPLACE FUNCTION code.format_cloud(
    c dbaas.clouds
) RETURNS code.cloud AS $$
SELECT
    $1.cloud_id,
    $1.cloud_ext_id,
    0::bigint, -- cloud_rev
    $1.cpu_quota,
    $1.gpu_quota,
    $1.memory_quota,
    $1.ssd_space_quota,
    $1.hdd_space_quota,
    $1.clusters_quota,
    $1.cpu_used,
    $1.gpu_used,
    $1.memory_used,
    $1.ssd_space_used,
    $1.hdd_space_used,
    $1.clusters_used;
$$ LANGUAGE SQL IMMUTABLE;

--head/code/20_format_host.sql
CREATE OR REPLACE FUNCTION code.format_host(
    h dbaas.hosts,
    c dbaas.clusters,
    s dbaas.subclusters,
    g dbaas.geo,
    d dbaas.disk_type,
    f dbaas.flavors
) RETURNS code.host AS $$
SELECT
    h.subcid,
    h.shard_id,
    h.space_limit,
    h.flavor,
    h.subnet_id,
    g.name,
    d.disk_type_ext_id AS disk_type_id,
    h.fqdn,
    f.vtype,
    h.vtype_id,
    s.roles,
    s.name,
    h.assign_public_ip,
    c.env,
    f.name,
    h.created_at,
    c.host_group_ids,
    c.type as cluster_type;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.format_host(
    h dbaas.hosts
) RETURNS code.host AS $$
SELECT fmt.*
  FROM dbaas.subclusters s,
       dbaas.clusters c,
       dbaas.geo g,
       dbaas.disk_type d,
       dbaas.flavors f,
       code.format_host(h, c, s, g, d, f) fmt
 WHERE s.subcid = (h).subcid
   AND c.cid = s.cid
   AND g.geo_id = (h).geo_id
   AND d.disk_type_id = (h).disk_type_id
   AND f.id = (h).flavor;
$$ LANGUAGE SQL STABLE;

DROP FUNCTION IF EXISTS code.format_host(
    h dbaas.hosts_revs,
    c dbaas.clusters,
    s dbaas.subclusters_revs,
    g dbaas.geo,
    d dbaas.disk_type,
    f dbaas.flavors
);

CREATE OR REPLACE FUNCTION code.format_host(
    h dbaas.hosts_revs,
    c dbaas.clusters,
    cr dbaas.clusters_revs,
    s dbaas.subclusters_revs,
    g dbaas.geo,
    d dbaas.disk_type,
    f dbaas.flavors
) RETURNS code.host AS $$
SELECT
    h.subcid,
    h.shard_id,
    h.space_limit,
    h.flavor,
    h.subnet_id,
    g.name,
    d.disk_type_ext_id AS disk_type_id,
    h.fqdn,
    f.vtype,
    h.vtype_id,
    s.roles,
    s.name,
    h.assign_public_ip,
    c.env,
    f.name,
    h.created_at,
    cr.host_group_ids,
    c.type as cluster_type;
$$ LANGUAGE SQL IMMUTABLE;

--head/code/20_get_cluster_labels.sql
CREATE OR REPLACE FUNCTION code.get_cluster_labels(
    i_cid text
) RETURNS code.label[] AS $$
SELECT ARRAY(
    SELECT (label_key, label_value)::code.label
      FROM dbaas.cluster_labels
      WHERE cid = i_cid
    );
$$ LANGUAGE SQL STABLE;

--head/code/20_get_cluster_labels_at_rev.sql
CREATE OR REPLACE FUNCTION code.get_cluster_labels_at_rev(
    i_cid text,
    i_rev bigint
) RETURNS code.label[] AS $$
SELECT ARRAY(
    SELECT (label_key, label_value)::code.label
      FROM dbaas.cluster_labels_revs
      WHERE cid = i_cid
        AND rev = i_rev
    );
$$ LANGUAGE SQL STABLE;

--head/code/20_get_coords.sql
CREATE OR REPLACE FUNCTION code.get_coords(
    i_cid text
) RETURNS code.cluster_coords AS $$
SELECT (i_cid, subcids, shard_ids, fqdns)::code.cluster_coords
  FROM (
      SELECT array_agg(subcid) AS subcids
        FROM dbaas.subclusters
       WHERE subclusters.cid = i_cid) sc,
  LATERAL (
      SELECT array_agg(shard_id) AS shard_ids
        FROM dbaas.shards
       WHERE shards.subcid = ANY(subcids)) sha,
  LATERAL (
      SELECT array_agg(fqdn) AS fqdns
        FROM dbaas.hosts
       WHERE hosts.subcid = ANY(subcids)) h;
$$ LANGUAGE SQL STABLE;

--head/code/20_get_coords_at_rev.sql
CREATE OR REPLACE FUNCTION code.get_coords_at_rev(
    i_cid text,
    i_rev bigint
) RETURNS code.cluster_coords AS $$
SELECT (i_cid, subcids, shard_ids, fqdns)::code.cluster_coords
  FROM (
      SELECT array_agg(subcid) AS subcids
        FROM dbaas.subclusters_revs
       WHERE subclusters_revs.cid = i_cid
         AND rev = i_rev) sc,
  LATERAL (
      SELECT array_agg(shard_id) AS shard_ids
        FROM dbaas.shards_revs
       WHERE shards_revs.subcid = ANY(subcids)
         AND rev = i_rev) sha,
  LATERAL (
      SELECT array_agg(fqdn) AS fqdns
        FROM dbaas.hosts_revs
       WHERE hosts_revs.subcid = ANY(subcids)
         AND rev = i_rev) h;
$$ LANGUAGE SQL STABLE;
--head/code/20_make_quota.sql
CREATE OR REPLACE FUNCTION code.make_quota(
    i_cpu       real,
    i_memory    bigint,
    i_network   bigint DEFAULT 0,
    i_io        bigint DEFAULT 0,
    i_clusters  bigint DEFAULT 0,
    i_ssd_space bigint DEFAULT 0,
    i_hdd_space bigint DEFAULT 0,
    i_gpu       bigint DEFAULT 0
) RETURNS code.quota AS $$
SELECT (i_cpu, i_memory, i_network, i_io, i_ssd_space, i_hdd_space, i_clusters, i_gpu)::code.quota;
$$ LANGUAGE SQL IMMUTABLE;

--head/code/20_managed.sql
CREATE OR REPLACE FUNCTION code.managed(
    dbaas.clusters
) RETURNS bool AS $$
SELECT $1.type NOT IN ('hadoop_cluster'::dbaas.cluster_type);
$$ LANGUAGE SQL IMMUTABLE;

--head/code/20_multiply_quota.sql
-- Multiply quota by given multiplier
CREATE OR REPLACE FUNCTION code.multiply_quota(
    code.quota,
    int
) RETURNS code.quota AS $$
SELECT code.make_quota(
    i_cpu       => code.round_cpu_quota(($1.cpu * $2)::real),
    i_gpu       => $1.gpu * $2,
    i_memory    => $1.memory * $2,
    i_clusters  => $1.clusters * $2,
    i_ssd_space => $1.ssd_space * $2,
    i_hdd_space => $1.hdd_space * $2
);
$$ LANGUAGE SQL IMMUTABLE;

--head/code/20_rev.sql
CREATE OR REPLACE FUNCTION code.rev(
    dbaas.clusters
) RETURNS bigint AS $$
SELECT $1.actual_rev;
$$ LANGUAGE SQL IMMUTABLE;

--head/code/20_to_rev.sql
CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.clusters
) RETURNS dbaas.clusters_revs AS $$
SELECT
    $1.cid,
    $1.next_rev,
    $1.name,
    $1.network_id,
    $1.folder_id,
    $1.description,
    $1.status,
    $1.host_group_ids;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.subclusters,
    bigint
) RETURNS dbaas.subclusters_revs AS $$
SELECT
    $1.subcid,
    $2,
    $1.cid,
    $1.name,
    $1.roles,
    $1.created_at;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.shards,
    bigint
) RETURNS dbaas.shards_revs AS $$
SELECT
    $1.subcid,
    $1.shard_id,
    $2,
    $1.name,
    $1.created_at;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.hosts,
    bigint
) RETURNS dbaas.hosts_revs AS $$
SELECT
    $1.subcid,
    $1.shard_id,
    $1.flavor,
    $1.space_limit,
    $1.fqdn,
    $2,
    $1.vtype_id,
    $1.geo_id,
    $1.disk_type_id,
    $1.subnet_id,
    $1.assign_public_ip,
    $1.created_at;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.pillar,
    bigint
) RETURNS dbaas.pillar_revs AS $$
SELECT
    $2,
    $1.cid,
    $1.subcid,
    $1.shard_id,
    $1.fqdn,
    $1.value;
$$ LANGUAGE SQL IMMUTABLE;


CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.cluster_labels,
    bigint
) RETURNS dbaas.cluster_labels_revs AS $$
SELECT
    $1.cid,
    $1.label_key,
    $2,
    $1.label_value;
$$ LANGUAGE SQL IMMUTABLE;


CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.maintenance_window_settings,
    bigint
) RETURNS dbaas.maintenance_window_settings_revs AS $$
SELECT
    $1.cid,
    $2,
    $1.day,
    $1.hour;
$$ LANGUAGE SQL IMMUTABLE;


CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.backup_schedule,
    bigint
) RETURNS dbaas.backup_schedule_revs AS $$
SELECT
    $1.cid,
    $2,
    $1.schedule;
$$ LANGUAGE SQL IMMUTABLE;


CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.instance_groups,
    bigint
) RETURNS dbaas.instance_groups_revs AS $$
SELECT
    $1.subcid,
    $1.instance_group_id,
    $2;
$$ LANGUAGE SQL IMMUTABLE;


CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.sgroups,
    bigint
) RETURNS dbaas.sgroups_revs AS $$
SELECT
    $1.cid,
    $1.sg_ext_id,
    $1.sg_type,
    $2,
    $1.sg_hash,
    $1.sg_allow_all;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.versions,
    bigint
) RETURNS dbaas.versions_revs AS $$
SELECT
    $1.cid,
    $1.subcid,
    $1.shard_id,
    $1.component,
    $1.major_version,
    $1.minor_version,
    $1.package_version,
    $2,
    $1.edition,
    $1.pinned;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.disk_placement_groups,
    bigint
) RETURNS dbaas.disk_placement_groups_revs AS $$
SELECT
    $2,
    $1.pg_id,
    $1.cid,
    $1.local_id,
    $1.disk_placement_group_id,
    $1.status;
$$ LANGUAGE SQL IMMUTABLE;


CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.disks,
    bigint
) RETURNS dbaas.disks_revs AS $$
SELECT
    $2,
    $1.d_id,
    $1.pg_id,
    $1.fqdn,
    $1.mount_point,
    $1.disk_id,
    $1.host_disk_id,
    $1.status,
    $1.cid;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.alert_group,
    bigint
) RETURNS dbaas.alert_group_revs AS $$
SELECT
    $1.alert_group_id,
    $2,
    $1.cid,
    $1.monitoring_folder_id,
    $1.managed,
    $1.status;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.alert,
    bigint
) RETURNS dbaas.alert_revs AS $$
SELECT
    $1.alert_ext_id,
    $1.alert_group_id,
    $1.template_id,
    $2,
    $1.notification_channels,
    $1.disabled,
    $1.critical_threshold,
    $1.warning_threshold,
    $1.default_thresholds,
    $1.status;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.placement_groups,
    bigint
) RETURNS dbaas.placement_groups_revs AS $$
SELECT
    $1.pg_id,
    $2,
    $1.cid,
    $1.subcid,
    $1.shard_id,
    $1.placement_group_id,
    $1.status;
$$ LANGUAGE SQL IMMUTABLE;


--head/code/20_unquoted_clusters.sql
CREATE OR REPLACE FUNCTION code.unquoted_clusters()
RETURNS dbaas.cluster_type[] AS $$
SELECT ARRAY['hadoop_cluster'::dbaas.cluster_type];
$$ LANGUAGE SQL IMMUTABLE;

--head/code/20_visible.sql
CREATE OR REPLACE FUNCTION code.visible(
    dbaas.clusters
) RETURNS bool AS $$
SELECT dbaas.visible_cluster_status($1.status);
$$ LANGUAGE SQL IMMUTABLE;

--head/code/25_get_usage_from_cloud.sql
CREATE OR REPLACE FUNCTION code.get_usage_from_cloud(
    dbaas.clouds
) RETURNS code.quota AS $$
SELECT code.make_quota(
    i_cpu       => $1.cpu_used,
    i_gpu       => $1.gpu_used,
    i_memory    => $1.memory_used,
    i_ssd_space => $1.ssd_space_used,
    i_hdd_space => $1.hdd_space_used,
    i_clusters  => $1.clusters_used
);
$$ LANGUAGE SQL IMMUTABLE;

--head/code/25_match_visibility.sql
CREATE OR REPLACE FUNCTION code.match_visibility(
    c dbaas.clusters,
    v code.visibility
) RETURNS boolean AS $$
SELECT
    CASE v
        WHEN 'all' THEN true
        WHEN 'visible+deleted' THEN c.status NOT IN ('PURGING', 'PURGED', 'PURGE-ERROR')
        ELSE code.visible(c)
    END;
$$ LANGUAGE SQL IMMUTABLE;

--head/code/30_flavor_as_quota.sql
-- Create code.quota from flavor
CREATE OR REPLACE FUNCTION code.flavor_as_quota(
    i_flavor_name text,
    i_ssd_space   bigint DEFAULT NULL,
    i_hdd_space   bigint DEFAULT NULL
) RETURNS SETOF code.quota AS $$
 SELECT q.*
   FROM dbaas.flavors,
LATERAL code.make_quota(
            i_cpu       => cpu_guarantee,
            i_gpu       => gpu_limit,
            i_memory    => memory_guarantee,
            i_clusters  => NULL,
            i_ssd_space => i_ssd_space,
            i_hdd_space => i_hdd_space
        ) q
  WHERE flavors.name = i_flavor_name;
$$ LANGUAGE SQL STABLE;

--head/code/30_get_host.sql
CREATE OR REPLACE FUNCTION code.get_host(
    i_fqdn text
) RETURNS SETOF code.host AS $$
SELECT fmt.*
  FROM dbaas.subclusters s
  JOIN dbaas.hosts h
 USING (subcid)
  JOIN dbaas.clusters c
 USING (cid)
  JOIN dbaas.geo g
 USING (geo_id)
  JOIN dbaas.disk_type d
 USING (disk_type_id)
  JOIN dbaas.flavors f
    ON (h.flavor = f.id),
       code.format_host(h, c, s, g, d, f) fmt
 WHERE h.fqdn = i_fqdn;
$$ LANGUAGE SQL STABLE;

--head/code/30_get_hosts_by_cid.sql
CREATE OR REPLACE FUNCTION code.get_hosts_by_cid(
    i_cid        text,
    i_visibility code.visibility DEFAULT 'visible'
) RETURNS SETOF code.host AS $$
SELECT fmt.*
  FROM dbaas.clusters c
  JOIN dbaas.subclusters s
 USING (cid)
  JOIN dbaas.hosts h
 USING (subcid)
  JOIN dbaas.geo g
 USING (geo_id)
  JOIN dbaas.disk_type d
 USING (disk_type_id)
  JOIN dbaas.flavors f
    ON (h.flavor = f.id),
       code.format_host(h, c, s, g, d, f) fmt
 WHERE c.cid = i_cid
   AND code.match_visibility(c, i_visibility);
$$ LANGUAGE SQL STABLE;

--head/code/30_get_hosts_by_cid_at_rev.sql
CREATE OR REPLACE FUNCTION code.get_hosts_by_cid_at_rev(
    i_cid text,
    i_rev bigint
) RETURNS SETOF code.host AS $$
SELECT fmt.*
  FROM dbaas.clusters c
  JOIN dbaas.clusters_revs cr
 USING (cid)
  JOIN dbaas.subclusters_revs s
 USING (cid, rev)
  JOIN dbaas.hosts_revs h
 USING (subcid, rev)
  JOIN dbaas.geo g
 USING (geo_id)
  JOIN dbaas.disk_type d
 USING (disk_type_id)
  JOIN dbaas.flavors f
    ON (h.flavor = f.id),
       code.format_host(h, c, cr, s, g, d, f) fmt
 WHERE c.cid = i_cid
   AND cr.rev = i_rev;
$$ LANGUAGE SQL STABLE;

--head/code/35_get_cloud_feature_flags.sql
CREATE OR REPLACE FUNCTION code.get_cloud_feature_flags(
    i_cloud_id bigint
) RETURNS text[] AS $$
SELECT ARRAY(
    SELECT flag_name
        FROM dbaas.default_feature_flags
    UNION
    SELECT flag_name
        FROM dbaas.cloud_feature_flags
        WHERE cloud_id = i_cloud_id
    ORDER BY flag_name);
$$ LANGUAGE SQL STABLE;

--head/code/35_get_folder_id_by_operation.sql
CREATE OR REPLACE FUNCTION code.get_folder_id_by_operation(
    i_operation_id text
) RETURNS bigint AS $$
SELECT folder_id
  FROM dbaas.worker_queue
 WHERE task_id = i_operation_id;
$$ LANGUAGE SQL STABLE;

--head/code/35_get_operation_by_id.sql
CREATE OR REPLACE FUNCTION code.get_operation_by_id(
    i_folder_id    bigint,
    i_operation_id text
) RETURNS SETOF code.operation AS $$
SELECT fmt.*
  FROM dbaas.worker_queue q
  JOIN dbaas.clusters c
 USING (cid),
       code.as_operation(q, c) fmt
 WHERE task_id = i_operation_id
   AND q.folder_id = i_folder_id;
$$ LANGUAGE SQL STABLE;

--head/code/35_get_operation_id_by_idempotence.sql
CREATE OR REPLACE FUNCTION code.get_operation_id_by_idempotence(
    i_idempotence_id uuid,
    i_folder_id      bigint,
    i_user_id        text
) RETURNS SETOF code.operation_id_by_idempotence AS $$
SELECT task_id, request_hash
  FROM dbaas.idempotence
 WHERE idempotence_id = i_idempotence_id
   AND folder_id = i_folder_id
   AND user_id = i_user_id;
$$ LANGUAGE SQL STABLE;

--head/code/35_get_operations.sql
CREATE OR REPLACE FUNCTION code.get_operations(
    i_folder_id             bigint,
    i_cid                   text,
    i_limit                 integer,
    i_env                   dbaas.env_type            DEFAULT NULL,
    i_cluster_type          dbaas.cluster_type        DEFAULT NULL,
    i_type                  text                      DEFAULT NULL,
    i_created_by            text                      DEFAULT NULL,
    i_page_token_id         text                      DEFAULT NULL,
    i_page_token_create_ts  timestamp with time zone  DEFAULT NULL,
    i_include_hidden        boolean                   DEFAULT NULL
) RETURNS SETOF code.operation AS $$
SELECT fmt.*
  FROM dbaas.worker_queue q
  JOIN dbaas.clusters c
 USING (cid),
       code.as_operation(q, c) fmt
 WHERE (i_cid IS NULL OR c.cid = i_cid)
   AND (i_folder_id IS NULL OR q.folder_id = i_folder_id)
   AND (i_env IS NULL OR c.env = i_env)
   AND (i_cluster_type IS NULL OR c.type = i_cluster_type)
   AND (i_type IS NULL OR q.task_type = i_type)
   AND (i_created_by IS NULL OR q.created_by = i_created_by)
   AND (
        (i_page_token_create_ts IS NULL AND i_page_token_id IS NULL)
        OR (q.create_ts, q.task_id) < (i_page_token_create_ts, i_page_token_id)
   )
   AND (i_include_hidden = true OR q.hidden = false)
 ORDER BY q.create_ts DESC, q.task_id DESC
 LIMIT i_limit;
$$ LANGUAGE SQL STABLE;

--head/code/40_save_idempotence.sql
CREATE OR REPLACE FUNCTION code.save_idempotence(
    i_operation_id      text,
    i_folder_id         bigint,
    i_user_id           text,
    i_idempotence_data  code.idempotence_data DEFAULT NULL
) RETURNS code.operation AS $$
DECLARE
    v_task    dbaas.worker_queue;
    v_cluster dbaas.clusters;
BEGIN
    IF i_idempotence_data.idempotence_id IS NOT NULL AND i_idempotence_data.request_hash IS NOT NULL
    THEN
        INSERT INTO dbaas.idempotence (
            idempotence_id,
            task_id,
            folder_id,
            user_id,
            request_hash
        )
        VALUES (
            i_idempotence_data.idempotence_id,
            i_operation_id,
            i_folder_id,
            i_user_id,
            i_idempotence_data.request_hash
        );
    END IF;

    RETURN code.as_operation(v_task, v_cluster);
END;
$$ LANGUAGE plpgsql;

--head/code/40_update_cluster_change.sql
CREATE OR REPLACE FUNCTION code.update_cluster_change(
    i_cid     text,
    i_rev     bigint,
    i_change  jsonb
) RETURNS void AS $$
DECLARE
    v_change_xid bigint;
BEGIN
    UPDATE dbaas.clusters_changes
       SET changes = changes || jsonb_build_array(i_change)
     WHERE cid = i_cid
       AND rev = i_rev
    RETURNING commit_id INTO v_change_xid;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to update cluster change cid=%, rev=%', i_cid, i_rev;
    END IF;

    IF v_change_xid IS DISTINCT FROM txid_current() THEN
        RAISE EXCEPTION 'Attempt to modify cluster in different transaction. Change xid: %. Current xid: %.', v_change_xid, txid_current()
            USING HINT = 'Wrap your call in code.lock_cluster() ... code.complete_cluster_change() block';
    END IF;
END;
$$ LANGUAGE plpgsql;

--head/code/45_add_cloud.sql
CREATE OR REPLACE FUNCTION code.add_cloud(
    i_cloud_ext_id text,
    i_quota        code.quota,
    i_x_request_id text
) RETURNS code.cloud AS $$
DECLARE
    v_cloud dbaas.clouds;
BEGIN
    INSERT INTO dbaas.clouds (
        cloud_ext_id,
        cpu_quota,
        gpu_quota,
        memory_quota,
        ssd_space_quota,
        hdd_space_quota,
        clusters_quota,
        actual_cloud_rev
    ) VALUES (
        i_cloud_ext_id,
        (i_quota).cpu,
        (i_quota).gpu,
        (i_quota).memory,
        (i_quota).ssd_space,
        (i_quota).hdd_space,
        (i_quota).clusters,
        1
    ) RETURNING * INTO v_cloud;

    PERFORM code.log_cloud_rev(v_cloud, i_x_request_id);

    RETURN code.format_cloud(v_cloud);
END;
$$ LANGUAGE plpgsql;

--head/code/45_fix_cloud_usage.sql
CREATE OR REPLACE FUNCTION code.fix_cloud_usage(
    i_cloud_ext_id text,
    i_x_request_id text DEFAULT NULL
) RETURNS code.cloud AS $$
DECLARE
    v_cloud dbaas.clouds;
BEGIN
    -- just lock
    SELECT *
      INTO v_cloud
      FROM dbaas.clouds
     WHERE cloud_ext_id = i_cloud_ext_id
       FOR UPDATE;

    UPDATE dbaas.clouds
       SET cpu_used = cpu,
           gpu_used = gpu,
           memory_used = memory,
           ssd_space_used = ssd_space,
           hdd_space_used = hdd_space,
           clusters_used = clusters,
           actual_cloud_rev = actual_cloud_rev + 1
      FROM code.get_cloud_real_usage(i_cloud_ext_id)
     WHERE cloud_ext_id = i_cloud_ext_id
    RETURNING * INTO v_cloud;

    PERFORM code.log_cloud_rev(v_cloud, i_x_request_id);

    RETURN code.format_cloud(v_cloud);
END;
$$ LANGUAGE plpgsql;

--head/code/45_get_cloud.sql
CREATE OR REPLACE FUNCTION code.get_cloud(
    i_cloud_id     bigint,
    i_cloud_ext_id text
) RETURNS SETOF code.cloud AS $$
SELECT fmt.*
  FROM dbaas.clouds c,
       code.format_cloud(c) fmt
 WHERE (i_cloud_id IS NULL OR c.cloud_id = i_cloud_id)
   AND (i_cloud_ext_id IS NULL OR c.cloud_ext_id = i_cloud_ext_id);
$$ LANGUAGE SQL STABLE;

--head/code/45_get_cloud_real_usage.sql
CREATE OR REPLACE FUNCTION code.get_cloud_real_usage(
    i_cloud_ext_id text
) RETURNS code.quota AS $$
-- we count usage by guarantee
SELECT code.make_quota(
    i_cpu       => coalesce(cpu::real, 0),
    i_gpu       => coalesce(gpu::bigint, 0),
    i_memory    => coalesce(memory::bigint, 0),
    i_ssd_space => coalesce(ssd_space::bigint, 0),
    i_hdd_space => coalesce(hdd_space::bigint, 0),
    i_clusters  => coalesce(clusters_count, 0))
  FROM (
      SELECT sum(cpu_guarantee) AS cpu,
             sum(gpu_limit) AS gpu,
             sum(memory_guarantee) AS memory,
             sum(space_limit) FILTER (WHERE quota_type = 'ssd'::dbaas.space_quota_type) AS ssd_space,
             sum(space_limit) FILTER (WHERE quota_type = 'hdd'::dbaas.space_quota_type) AS hdd_space,
             count(DISTINCT cid) AS clusters_count
        FROM dbaas.clouds
        JOIN dbaas.folders USING (cloud_id)
        JOIN dbaas.clusters USING (folder_id)
        JOIN dbaas.subclusters USING (cid)
        JOIN dbaas.hosts USING (subcid)
        JOIN dbaas.disk_type USING (disk_type_id)
        JOIN dbaas.flavors ON (hosts.flavor = flavors.id)
       WHERE dbaas.visible_cluster_status(clusters.status)
         AND clusters.type != ANY(code.unquoted_clusters())
         AND cloud_ext_id = i_cloud_ext_id) a;
$$ LANGUAGE SQL STABLE;

--head/code/45_lock_cloud.sql
CREATE OR REPLACE FUNCTION code.lock_cloud(
    i_cloud_id bigint
) RETURNS SETOF code.cloud AS $$
SELECT fmt.*
  FROM dbaas.clouds c,
       code.format_cloud(c) fmt
 WHERE c.cloud_id = i_cloud_id
   FOR UPDATE;
$$ LANGUAGE SQL;

--head/code/45_log_cloud_rev.sql
CREATE OR REPLACE FUNCTION code.log_cloud_rev(
    i_cloud        dbaas.clouds,
    i_x_request_id text
) RETURNS void AS $$
INSERT INTO dbaas.clouds_revs
    (cloud_id, cloud_rev,
     cpu_quota, gpu_quota, memory_quota, ssd_space_quota,
     hdd_space_quota, clusters_quota,
     cpu_used, gpu_used, memory_used, ssd_space_used,
     hdd_space_used, clusters_used,
     x_request_id)
SELECT
     $1.cloud_id, $1.actual_cloud_rev,
     $1.cpu_quota, $1.gpu_quota, $1.memory_quota, $1.ssd_space_quota,
     $1.hdd_space_quota, $1.clusters_quota,
     $1.cpu_used, $1.gpu_used, $1.memory_used, $1.ssd_space_used,
     $1.hdd_space_used, $1.clusters_used,
     i_x_request_id;
$$ LANGUAGE SQL;

--head/code/45_set_cloud_quota.sql
CREATE OR REPLACE FUNCTION code.set_cloud_quota(
    i_cloud_ext_id text,
    i_quota        code.quota,
    i_x_request_id text
) RETURNS code.cloud AS $$
DECLARE
    v_cloud dbaas.clouds;
BEGIN
    UPDATE dbaas.clouds
       SET cpu_quota = coalesce((i_quota).cpu, cpu_quota),
           gpu_quota = coalesce((i_quota).gpu, gpu_quota),
           memory_quota = coalesce((i_quota).memory, memory_quota),
           ssd_space_quota = coalesce((i_quota).ssd_space, ssd_space_quota),
           hdd_space_quota = coalesce((i_quota).hdd_space, hdd_space_quota),
           clusters_quota = coalesce((i_quota).clusters, clusters_quota),
           actual_cloud_rev = actual_cloud_rev + 1
     WHERE cloud_ext_id = i_cloud_ext_id
    RETURNING * INTO v_cloud;

    PERFORM code.log_cloud_rev(v_cloud, i_x_request_id);

    RETURN code.format_cloud(v_cloud);
END;
$$ LANGUAGE plpgsql;

--head/code/45_update_cloud_quota.sql
CREATE OR REPLACE FUNCTION code.update_cloud_quota(
    i_cloud_ext_id text,
    i_delta        code.quota,
    i_x_request_id text
) RETURNS code.cloud AS $$
DECLARE
    v_cloud dbaas.clouds;
BEGIN
    UPDATE dbaas.clouds
       SET cpu_quota = code.round_cpu_quota(cpu_quota + coalesce((i_delta).cpu, 0)),
           gpu_quota = gpu_quota + coalesce((i_delta).gpu, 0),
           memory_quota = memory_quota + coalesce((i_delta).memory, 0),
           ssd_space_quota = ssd_space_quota + coalesce((i_delta).ssd_space, 0),
           hdd_space_quota = hdd_space_quota + coalesce((i_delta).hdd_space, 0),
           clusters_quota = clusters_quota + coalesce((i_delta).clusters, 0),
           actual_cloud_rev = actual_cloud_rev + 1
     WHERE cloud_ext_id = i_cloud_ext_id
    RETURNING * INTO v_cloud;

    PERFORM code.log_cloud_rev(v_cloud, i_x_request_id);

    RETURN code.format_cloud(v_cloud);
END;
$$ LANGUAGE plpgsql;

--head/code/45_update_cloud_usage.sql
CREATE OR REPLACE FUNCTION code.update_cloud_usage(
    i_cloud_id     bigint,
    i_delta  code.quota,
    i_x_request_id text
) RETURNS code.cloud AS $$
DECLARE
    v_cloud dbaas.clouds;
BEGIN
    UPDATE dbaas.clouds
       SET cpu_used = code.round_cpu_quota(cpu_used + coalesce((i_delta).cpu, 0)),
           gpu_used = gpu_used + coalesce((i_delta).gpu, 0),
           memory_used = memory_used + coalesce((i_delta).memory, 0),
           ssd_space_used = ssd_space_used + coalesce((i_delta).ssd_space, 0),
           hdd_space_used = hdd_space_used + coalesce((i_delta).hdd_space, 0),
           clusters_used = clusters_used + coalesce((i_delta).clusters, 0),
           actual_cloud_rev = actual_cloud_rev + 1
     WHERE cloud_id = i_cloud_id
    RETURNING * INTO v_cloud;

    PERFORM code.log_cloud_rev(v_cloud, i_x_request_id);

    RETURN code.format_cloud(v_cloud);
END;
$$ LANGUAGE plpgsql;

--head/code/45_verify_clouds_quota_usage.sql
CREATE OR REPLACE FUNCTION code.verify_clouds_quota_usage() RETURNS TABLE (
    o_cloud_ext_id text,
    o_cloud_id bigint,
    o_cpu real,
    o_gpu bigint,
    o_memory bigint,
    o_ssd_space bigint,
    o_hdd_space bigint,
    o_clusters bigint
) AS $$
SELECT cloud_ext_id,
       cloud_id,
       cpu - cpu_used,
       gpu - gpu_used,
       memory - memory_used,
       hdd_space - hdd_space_used,
       ssd_space - ssd_space_used,
       clusters - clusters_used
  FROM dbaas.clouds,
       code.get_cloud_real_usage(cloud_ext_id) real_usage
 WHERE code.get_usage_from_cloud(clouds) != real_usage;
$$ LANGUAGE SQL STABLE;

--head/code/50_add_pillar.sql
CREATE OR REPLACE FUNCTION code.add_pillar(
    i_cid   text,
    i_rev   bigint,
    i_key   code.pillar_key,
    i_value jsonb
) RETURNS void AS $$
BEGIN
    PERFORM code.check_pillar_key(i_key);
    INSERT INTO dbaas.pillar (
        cid, subcid,
        shard_id, fqdn,
        value
    ) VALUES (
        (i_key).cid, (i_key).subcid,
        (i_key).shard_id, (i_key).fqdn,
        i_value
    );

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_pillar',
            jsonb_build_object(
                'cid', (i_key).cid,
                'subcid', (i_key).subcid,
                'shard_id', (i_key).shard_id,
                'fqdn', (i_key).fqdn
            )
        )
    );
END;
$$ LANGUAGE plpgsql;

--head/code/50_get_pillar_by_host.sql
CREATE OR REPLACE FUNCTION code.get_pillar_by_host(
    i_fqdn      text,
    i_target_id text DEFAULT NULL
) RETURNS SETOF code.pillar_with_priority AS $$
WITH host AS (
    SELECT cid,
           type,
           subcid,
           roles,
           shard_id
      FROM dbaas.hosts
      JOIN dbaas.subclusters USING (subcid)
      JOIN dbaas.clusters USING (cid)
     WHERE fqdn = i_fqdn
       AND code.visible(clusters)
)
SELECT value, 'default'::code.pillar_priority
  FROM dbaas.default_pillar
 WHERE id = 1

 UNION ALL

SELECT value, 'cluster_type'::code.pillar_priority
  FROM dbaas.cluster_type_pillar
 WHERE type = (SELECT type FROM host)

 UNION ALL

SELECT value, 'role'::code.pillar_priority
  FROM dbaas.role_pillar p
  JOIN host h ON (p.type = h.type AND p.role = ANY(h.roles))

 UNION ALL

SELECT value, 'cid'::code.pillar_priority
  FROM dbaas.pillar
 WHERE cid = (SELECT cid FROM host)
 UNION ALL
SELECT value, 'target_cid'::code.pillar_priority
  FROM dbaas.target_pillar
 WHERE cid = (SELECT cid FROM host)
   AND target_id = i_target_id

 UNION ALL

SELECT value, 'subcid'::code.pillar_priority
  FROM dbaas.pillar
 WHERE subcid = (SELECT subcid FROM host)
 UNION ALL
SELECT value, 'target_subcid'::code.pillar_priority
  FROM dbaas.target_pillar
 WHERE subcid = (SELECT subcid FROM host)
   AND target_id = i_target_id

 UNION ALL

SELECT value, 'shard_id'::code.pillar_priority
  FROM dbaas.pillar
 WHERE shard_id = (SELECT shard_id FROM host)
 UNION ALL
SELECT value, 'target_shard_id'::code.pillar_priority
  FROM dbaas.target_pillar
 WHERE shard_id = (SELECT shard_id FROM host)
   AND target_id = i_target_id

 UNION ALL

SELECT value, 'fqdn'::code.pillar_priority
  FROM dbaas.pillar
 WHERE fqdn = i_fqdn
 UNION ALL
SELECT value, 'target_fqdn'::code.pillar_priority
  FROM dbaas.target_pillar
 WHERE fqdn = i_fqdn
   AND target_id = i_target_id
$$ LANGUAGE SQL STABLE;

--head/code/50_get_rev_pillar_by_host.sql
CREATE OR REPLACE FUNCTION code.get_rev_pillar_by_host(
    i_fqdn      text,
    i_rev       bigint,
    i_target_id text DEFAULT NULL
) RETURNS SETOF code.pillar_with_priority AS $$
WITH host AS (
    SELECT cid,
           type,
           subcid,
           roles,
           shard_id
      FROM dbaas.hosts_revs h
      JOIN dbaas.subclusters_revs sc USING (subcid)
      JOIN dbaas.clusters USING (cid)
     WHERE fqdn = i_fqdn
       AND code.visible(clusters)
       AND h.rev = i_rev
       AND sc.rev = i_rev
)
SELECT value, 'default'::code.pillar_priority
  FROM dbaas.default_pillar
 WHERE id = 1

 UNION ALL

SELECT value, 'cluster_type'::code.pillar_priority
  FROM dbaas.cluster_type_pillar
 WHERE type = (SELECT type FROM host)

 UNION ALL

SELECT value, 'role'::code.pillar_priority
  FROM dbaas.role_pillar p
  JOIN host h ON (p.type = h.type AND p.role = ANY(h.roles))

 UNION ALL

SELECT value, 'cid'::code.pillar_priority
  FROM dbaas.pillar_revs
 WHERE cid = (SELECT cid FROM host)
   AND rev = i_rev
 UNION ALL
SELECT value, 'target_cid'::code.pillar_priority
  FROM dbaas.target_pillar
 WHERE cid = (SELECT cid FROM host)
   AND target_id = i_target_id

 UNION ALL

SELECT value, 'subcid'::code.pillar_priority
  FROM dbaas.pillar_revs
 WHERE subcid = (SELECT subcid FROM host)
   AND rev = i_rev
 UNION ALL
SELECT value, 'target_subcid'::code.pillar_priority
  FROM dbaas.target_pillar
 WHERE subcid = (SELECT subcid FROM host)
   AND target_id = i_target_id

 UNION ALL

SELECT value, 'shard_id'::code.pillar_priority
  FROM dbaas.pillar_revs
 WHERE shard_id = (SELECT shard_id FROM host)
   AND rev = i_rev
 UNION ALL
SELECT value, 'target_shard_id'::code.pillar_priority
  FROM dbaas.target_pillar
 WHERE shard_id = (SELECT shard_id FROM host)
   AND target_id = i_target_id

 UNION ALL

SELECT value, 'fqdn'::code.pillar_priority
  FROM dbaas.pillar_revs
 WHERE fqdn = i_fqdn
   AND rev = i_rev
 UNION ALL
SELECT value, 'target_fqdn'::code.pillar_priority
  FROM dbaas.target_pillar
 WHERE fqdn = i_fqdn
   AND target_id = i_target_id
$$ LANGUAGE SQL STABLE;

--head/code/50_update_pillar.sql
CREATE OR REPLACE FUNCTION code.update_pillar(
    i_cid   text,
    i_rev   bigint,
    i_key   code.pillar_key,
    i_value jsonb
) RETURNS void AS $$
DECLARE
BEGIN
    PERFORM code.check_pillar_key(i_key);
    UPDATE dbaas.pillar
       SET value = i_value
     WHERE ((i_key).cid IS NULL OR (cid = (i_key).cid AND cid IS NOT NULL))
       AND ((i_key).subcid IS NULL OR (subcid = (i_key).subcid AND subcid IS NOT NULL))
       AND ((i_key).shard_id IS NULL OR (shard_id = (i_key).shard_id AND shard_id IS NOT NULL))
       AND ((i_key).fqdn IS NULL OR (fqdn = (i_key).fqdn AND fqdn IS NOT NULL));

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_pillar',
            jsonb_build_object(
                'cid', (i_key).cid,
                'subcid', (i_key).subcid,
                'shard_id', (i_key).shard_id,
                'fqdn', (i_key).fqdn
            )
        )
    );
END;
$$ LANGUAGE plpgsql;

--head/code/50_upsert_pillar.sql
CREATE OR REPLACE FUNCTION code.upsert_pillar(
    i_cid   text,
    i_rev   bigint,
    i_value jsonb,
    i_key   code.pillar_key
) RETURNS void AS $$
BEGIN
    PERFORM code.check_pillar_key(i_key);
    CASE
        WHEN i_key.cid IS NOT NULL THEN
            INSERT INTO dbaas.pillar
                   (cid,
                    value)
            VALUES ((i_key).cid,
                    i_value)
            ON CONFLICT (cid) WHERE cid IS NOT NULL
               DO UPDATE
                 SET value = excluded.value;
        WHEN i_key.subcid IS NOT NULL THEN
            INSERT INTO dbaas.pillar
                   (subcid,
                    value)
            VALUES ((i_key).subcid,
                    i_value)
            ON CONFLICT (subcid) WHERE subcid IS NOT NULL
               DO UPDATE
                 SET value = excluded.value;
        WHEN i_key.shard_id IS NOT NULL THEN
            INSERT INTO dbaas.pillar
                   (shard_id,
                    value)
            VALUES ((i_key).shard_id,
                    i_value)
            ON CONFLICT (shard_id) WHERE shard_id IS NOT NULL
               DO UPDATE
                 SET value = excluded.value;
        WHEN i_key.fqdn IS NOT NULL THEN
            INSERT INTO dbaas.pillar
                   (fqdn,
                    value)
            VALUES ((i_key).fqdn,
                    i_value)
            ON CONFLICT (fqdn) WHERE fqdn IS NOT NULL
               DO UPDATE
                 SET value = excluded.value;
    END CASE;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'upsert_pillar',
            jsonb_build_object(
                'cid', (i_key).cid,
                'subcid', (i_key).subcid,
                'shard_id', (i_key).shard_id,
                'fqdn', (i_key).fqdn
            )
        )
    );

END;
$$ LANGUAGE plpgsql;

--head/code/54_cancel_maintenance_tasks.sql
CREATE OR REPLACE FUNCTION code.cancel_maintenance_tasks(
    i_cid   text,
    i_rev   bigint,
    i_msg   text default 'The job terminated due to cluster or maintenance changes'
) RETURNS void AS $$
BEGIN
    WITH maintenance_task_ids AS (
        UPDATE dbaas.maintenance_tasks
        SET status = 'CANCELED'::dbaas.maintenance_task_status
        WHERE cid = i_cid
          AND status = 'PLANNED'::dbaas.maintenance_task_status
        RETURNING task_id
    )
    UPDATE dbaas.worker_queue
    SET start_ts = now(),
        end_ts = now(),
        result = false,
        changes = '{}'::jsonb,
        errors = concat('[{"code": 1, "type": "Cancelled", "message": "', i_msg, '","exposable": true}]')::jsonb,
        context = NULL,
        finish_rev = i_rev
    WHERE task_id IN (SELECT task_id FROM maintenance_task_ids)
        AND worker_id IS NULL;
END
$$ LANGUAGE plpgsql;

--head/code/54_forward_cluster_revision.sql
CREATE OR REPLACE FUNCTION code.forward_cluster_revision(
    i_cid          text,
    i_x_request_id text DEFAULT NULL
) RETURNS dbaas.clusters AS $$
DECLARE
    v_cluster dbaas.clusters;
BEGIN
    UPDATE dbaas.clusters
    SET actual_rev = next_rev + 1,
        next_rev = next_rev + 1
    WHERE cid = i_cid
    RETURNING * INTO v_cluster;

    IF NOT found THEN
      RAISE EXCEPTION 'Unable to find cluster cid=% to lock', i_cid
        USING TABLE = 'dbaas.clusters';
    END IF;

    INSERT INTO dbaas.clusters_changes
      (cid, rev, x_request_id)
    VALUES
      (i_cid, (v_cluster).actual_rev, i_x_request_id);

    RETURN v_cluster;
END;
$$ LANGUAGE plpgsql;

--head/code/55_complete_cluster_change.sql
CREATE OR REPLACE FUNCTION code.complete_cluster_change(
    i_cid  text,
    i_rev  bigint
) RETURNS void AS $$
DECLARE
    v_coords code.cluster_coords;
BEGIN
    IF NOT EXISTS (
        SELECT 1
          FROM dbaas.clusters_changes
         WHERE cid = i_cid
           AND rev = i_rev) THEN
        RAISE EXCEPTION 'Unable to find cluster change cid=%, rev=%', i_cid, i_rev
            USING TABLE = 'dbaas.cluster_changes',
                   HINT = 'initiate cluster change with lock_cluster';
    END IF;

    v_coords := code.get_coords(i_cid);

    INSERT INTO dbaas.clusters_revs
    SELECT fmt.*
      FROM dbaas.clusters c,
           code.to_rev(c) fmt
     WHERE c.cid = i_cid;

    INSERT INTO dbaas.subclusters_revs
    SELECT fmt.*
      FROM dbaas.subclusters s,
           code.to_rev(s, i_rev) fmt
     WHERE s.subcid = ANY((v_coords).subcids);

    INSERT INTO dbaas.shards_revs
    SELECT fmt.*
      FROM dbaas.shards s,
           code.to_rev(s, i_rev) fmt
     WHERE s.shard_id = ANY((v_coords).shard_ids);

    INSERT INTO dbaas.hosts_revs
    SELECT fmt.*
      FROM dbaas.hosts h,
           code.to_rev(h, i_rev) fmt
     WHERE h.fqdn = ANY((v_coords).fqdns);

    INSERT INTO dbaas.pillar_revs
    SELECT fmt.*
      FROM dbaas.pillar p,
           code.to_rev(p, i_rev) fmt
     WHERE p.cid IS NOT NULL
       AND p.cid = i_cid;

    INSERT INTO dbaas.pillar_revs
    SELECT fmt.*
      FROM dbaas.pillar p,
           code.to_rev(p, i_rev) fmt
     WHERE p.subcid IS NOT NULL
       AND p.subcid = ANY((v_coords).subcids);

    INSERT INTO dbaas.pillar_revs
    SELECT fmt.*
      FROM dbaas.pillar p,
           code.to_rev(p, i_rev) fmt
     WHERE p.shard_id IS NOT NULL
       AND p.shard_id = ANY((v_coords).shard_ids);

    INSERT INTO dbaas.pillar_revs
    SELECT fmt.*
      FROM dbaas.pillar p,
           code.to_rev(p, i_rev) fmt
     WHERE p.fqdn IS NOT NULL
       AND p.fqdn = ANY((v_coords).fqdns);

    INSERT INTO dbaas.cluster_labels_revs
    SELECT fmt.*
      FROM dbaas.cluster_labels l,
           code.to_rev(l, i_rev) fmt
     WHERE l.cid = i_cid;

    INSERT INTO dbaas.maintenance_window_settings_revs
    SELECT fmt.*
    FROM dbaas.maintenance_window_settings s,
         code.to_rev(s, i_rev) fmt
    WHERE s.cid = i_cid;

    INSERT INTO dbaas.backup_schedule_revs
    SELECT fmt.*
      FROM dbaas.backup_schedule b,
           code.to_rev(b, i_rev) fmt
     WHERE b.cid = i_cid;

    INSERT INTO dbaas.instance_groups_revs
    SELECT fmt.*
      FROM dbaas.instance_groups i,
           code.to_rev(i, i_rev) fmt
     WHERE i.subcid = ANY((v_coords).subcids);

    INSERT INTO dbaas.sgroups_revs
    SELECT fmt.*
      FROM dbaas.sgroups i,
           code.to_rev(i, i_rev) fmt
     WHERE i.cid = i_cid;

    INSERT INTO dbaas.versions_revs
    SELECT fmt.*
      FROM dbaas.versions v,
           code.to_rev(v, i_rev) fmt
     WHERE v.cid IS NOT NULL
       AND v.cid = i_cid;

    INSERT INTO dbaas.versions_revs
    SELECT fmt.*
      FROM dbaas.versions v,
           code.to_rev(v, i_rev) fmt
     WHERE v.subcid IS NOT NULL
       AND v.subcid = ANY((v_coords).subcids);

    INSERT INTO dbaas.versions_revs
    SELECT fmt.*
      FROM dbaas.versions v,
           code.to_rev(v, i_rev) fmt
     WHERE v.shard_id IS NOT NULL
       AND v.shard_id = ANY((v_coords).shard_ids);

    INSERT INTO dbaas.disk_placement_groups_revs
    SELECT fmt.*
      FROM dbaas.disk_placement_groups v,
           code.to_rev(v, i_rev) fmt
     WHERE v.cid = i_cid;

    INSERT INTO dbaas.disks_revs
    SELECT fmt.*
      FROM dbaas.disks v,
           code.to_rev(v, i_rev) fmt
     WHERE v.cid = i_cid;

	INSERT INTO dbaas.alert_group_revs
    SELECT fmt.*
      FROM dbaas.alert_group v,
           code.to_rev(v, i_rev) fmt
     WHERE v.cid = i_cid;

    INSERT INTO dbaas.placement_groups_revs
    SELECT fmt.*
      FROM dbaas.placement_groups v,
           code.to_rev(v, i_rev) fmt
     WHERE v.cid = i_cid;
END;
$$ LANGUAGE plpgsql;

--head/code/55_create_cluster.sql
CREATE OR REPLACE FUNCTION code.create_cluster(
    i_cid                 text,
    i_name                text,
    i_type                dbaas.cluster_type,
    i_env                 dbaas.env_type,
    i_public_key          bytea,
    i_network_id          text,
    i_folder_id           bigint,
    i_description         text,
    i_x_request_id        text DEFAULT NULL,
    i_host_group_ids      text[] DEFAULT NULL,
    i_deletion_protection boolean DEFAULT FALSE
) RETURNS code.cluster_with_labels AS $$
DECLARE
    v_cluster dbaas.clusters;
    -- cast, cause I got a problems with fresh plpgsql_check
    -- it complaints: `Hidden casting can be a performance issue.`
    v_rev     constant bigint := CAST(1 AS bigint);
BEGIN
    INSERT INTO dbaas.clusters (
        cid, name, type,
        env, public_key, network_id,
        folder_id, description,
        actual_rev, next_rev, host_group_ids,
        deletion_protection
    )
    VALUES (
        i_cid, i_name, i_type,
        i_env, i_public_key, i_network_id,
        i_folder_id, i_description,
        v_rev, v_rev, i_host_group_ids,
        i_deletion_protection
    )
    RETURNING * INTO v_cluster;

    INSERT INTO dbaas.clusters_changes (
        cid,
        rev,
        changes,
        x_request_id
    ) VALUES (
        i_cid,
        v_rev,
        jsonb_build_array(
            jsonb_build_object(
                'create_cluster', jsonb_build_object()
            )
        ),
        i_x_request_id
    );

    RETURN code.as_cluster_with_labels(
        v_cluster, null, null, null, null, null, '{}'
    );
END;
$$ LANGUAGE plpgsql;

--head/code/55_get_cluster_quota_usage.sql
CREATE OR REPLACE FUNCTION code.get_cluster_quota_usage(
    i_cid text
) RETURNS code.quota AS $$
-- we count usage by guarantee
SELECT code.make_quota(
    i_cpu       => coalesce(cpu::real, 0),
    i_gpu       => coalesce(gpu::bigint, 0),
    i_memory    => coalesce(memory::bigint, 0),
    i_network   => coalesce(network::bigint, 0),
    i_io        => coalesce(io::bigint, 0),
    i_ssd_space => coalesce(ssd_space::bigint, 0),
    i_hdd_space => coalesce(hdd_space::bigint, 0),
    i_clusters  => coalesce(clusters_count, 0))
  FROM (
      SELECT sum(cpu_guarantee) AS cpu,
             sum(gpu_limit) AS gpu,
             sum(memory_guarantee) AS memory,
             sum(network_guarantee) AS network,
             sum(io_limit) AS io,
             sum(space_limit) FILTER (WHERE quota_type = 'ssd'::dbaas.space_quota_type) AS ssd_space,
             sum(space_limit) FILTER (WHERE quota_type = 'hdd'::dbaas.space_quota_type) AS hdd_space,
             count(DISTINCT cid) AS clusters_count
        FROM dbaas.clusters
        JOIN dbaas.subclusters USING (cid)
        JOIN dbaas.hosts USING (subcid)
        JOIN dbaas.disk_type USING (disk_type_id)
        JOIN dbaas.flavors ON (hosts.flavor = flavors.id)
       WHERE clusters.cid = i_cid
         AND clusters.type != ANY(code.unquoted_clusters())) a;
$$ LANGUAGE SQL STABLE;

--head/code/55_get_clusters.sql
CREATE OR REPLACE FUNCTION code.get_clusters(
    i_folder_id         bigint,
    i_limit             integer,
    i_cid               text                DEFAULT NULL,
    i_cluster_name      text                DEFAULT NULL,
    i_env               dbaas.env_type      DEFAULT NULL,
    i_cluster_type      dbaas.cluster_type  DEFAULT NULL,
    i_page_token_name   text                DEFAULT NULL,
    i_visibility        code.visibility     DEFAULT 'visible'
) RETURNS SETOF code.cluster_with_labels AS $$
SELECT fmt.*
  FROM dbaas.clusters c
  LEFT JOIN dbaas.pillar pl ON (c.cid = pl.cid)
  LEFT JOIN dbaas.maintenance_window_settings mws ON (c.cid = mws.cid)
  LEFT JOIN dbaas.maintenance_tasks mt ON (c.cid = mt.cid AND mt.status='PLANNED'::dbaas.maintenance_task_status)
  LEFT JOIN dbaas.backup_schedule b ON (c.cid = b.cid)
  , LATERAL (
    SELECT array_agg(
              (label_key, label_value)::code.label
              ORDER BY label_key, label_value) la
      FROM dbaas.cluster_labels cl
     WHERE cl.cid = c.cid
  ) x
  , LATERAL (
    SELECT array_agg(sg_ext_id ORDER BY sg_ext_id) sg_ids
      FROM dbaas.sgroups
     WHERE sgroups.cid = c.cid
       AND sg_type = 'user') sg,
      code.as_cluster_with_labels(c, b.schedule, pl.value, la,
                                  mws,
                                  mt,
                                  sg.sg_ids) fmt
WHERE c.folder_id = i_folder_id
  AND (i_cid IS NULL OR c.cid = i_cid)
  AND (i_cluster_name IS NULL OR c.name = i_cluster_name)
  AND (i_env IS NULL OR c.env = i_env)
  AND (i_cluster_type IS NULL OR c.type = i_cluster_type)
  AND (i_page_token_name IS NULL OR c.name > i_page_token_name)
  AND code.match_visibility(c, i_visibility)
ORDER BY c.name ASC
LIMIT i_limit;
$$ LANGUAGE SQL STABLE;

--head/code/55_lock_cluster.sql
CREATE OR REPLACE FUNCTION code.lock_cluster(
    i_cid          text,
    i_x_request_id text DEFAULT NULL
) RETURNS SETOF code.cluster_with_labels AS $$
DECLARE
  v_cluster dbaas.clusters;
  v_cluster_in_maintenance boolean DEFAULT false;
BEGIN
  SELECT actual_rev <> next_rev
  INTO v_cluster_in_maintenance
  FROM dbaas.clusters
  WHERE cid = i_cid;

  v_cluster := code.forward_cluster_revision(
      i_cid => i_cid,
      i_x_request_id => i_x_request_id);

  IF v_cluster_in_maintenance THEN
    PERFORM code.cancel_maintenance_tasks(
      i_cid => i_cid,
      i_rev => (v_cluster).actual_rev);
  END IF;

  RETURN QUERY
    SELECT cl.*
      FROM code.as_cluster_with_labels(v_cluster) cl;
END;
$$ LANGUAGE plpgsql;

--head/code/55_update_cluster_deletion_protection.sql
CREATE OR REPLACE FUNCTION code.update_cluster_deletion_protection(
    i_cid                 text,
    i_deletion_protection bool,
    i_rev                 bigint
) RETURNS SETOF code.cluster_with_labels AS $$
DECLARE
    v_cluster dbaas.clusters;
BEGIN
    UPDATE dbaas.clusters
       SET deletion_protection = i_deletion_protection
     WHERE cid = i_cid
    RETURNING * INTO v_cluster;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_cluster_deletion_protection',
            jsonb_build_object(
                'deletion_protection', i_deletion_protection
            )
        )
    );
    RETURN QUERY
        SELECT cl.*
          FROM code.as_cluster_with_labels(v_cluster) cl;
END;
$$ LANGUAGE plpgsql;

--head/code/55_update_cluster_description.sql
CREATE OR REPLACE FUNCTION code.update_cluster_description(
    i_cid         text,
    i_description text,
    i_rev         bigint
) RETURNS SETOF code.cluster_with_labels AS $$
DECLARE
    v_cluster dbaas.clusters;
BEGIN
    UPDATE dbaas.clusters
       SET description = i_description
     WHERE cid = i_cid
    RETURNING * INTO v_cluster;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_cluster_description',
            jsonb_build_object(
                'description', i_description
            )
        )
    );
    RETURN QUERY
        SELECT cl.*
          FROM code.as_cluster_with_labels(v_cluster) cl;
END;
$$ LANGUAGE plpgsql;

--head/code/55_update_cluster_folder.sql
CREATE OR REPLACE FUNCTION code.update_cluster_folder(
    i_cid       text,
    i_folder_id bigint,
    i_rev       bigint
) RETURNS SETOF code.cluster_with_labels AS $$
DECLARE
    v_cluster dbaas.clusters;
BEGIN
    UPDATE dbaas.clusters
       SET folder_id = i_folder_id
     WHERE cid = i_cid
       AND folder_id != i_folder_id
       AND code.visible(clusters)
    RETURNING * INTO v_cluster;

    IF NOT found THEN
        RAISE EXCEPTION 'Cluster % not exists, invisible or already in folder %', i_cid, i_folder_id
            USING TABLE = 'dbaas.clusters';
    END IF;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_cluster_folder',
            jsonb_build_object(
                'folder_id', i_folder_id
            )
        )
    );

    RETURN QUERY
        SELECT cl.*
          FROM code.as_cluster_with_labels(v_cluster) cl;
END;
$$ LANGUAGE plpgsql;

--head/code/55_update_cluster_name.sql
CREATE OR REPLACE FUNCTION code.update_cluster_name(
    i_cid   text,
    i_name  text,
    i_rev   bigint
) RETURNS SETOF code.cluster_with_labels AS $$
DECLARE
    v_cluster dbaas.clusters;
BEGIN
    UPDATE dbaas.clusters
       SET name = i_name
     WHERE cid = i_cid
    RETURNING * INTO v_cluster;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_cluster_name',
            jsonb_build_object(
                'name', i_name
            )
        )
    );

    RETURN QUERY
        SELECT cl.*
          FROM code.as_cluster_with_labels(v_cluster) cl;
END;
$$ LANGUAGE plpgsql;

--head/code/56_add_subcluster.sql
CREATE OR REPLACE FUNCTION code.add_subcluster(
    i_cid         text,
    i_subcid      text,
    i_name        text,
    i_roles       dbaas.role_type[],
    i_rev         bigint
) RETURNS TABLE (cid text, subcid text, name text, roles dbaas.role_type[]) AS $$
BEGIN
    INSERT INTO dbaas.subclusters (
        cid, subcid,
        name, roles
    ) VALUES (
        i_cid, i_subcid,
        i_name, i_roles
    );

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_subcluster',
             jsonb_build_object(
                'subcid', i_subcid,
                'name', i_name,
                'roles', i_roles::text[]
            )
        )
    );

    RETURN QUERY SELECT i_cid, i_subcid, i_name, i_roles;
END;
$$ LANGUAGE plpgsql;

--head/code/56_delete_subcluster.sql
CREATE OR REPLACE FUNCTION code.delete_subcluster(
    i_cid         text,
    i_subcid      text,
    i_rev         bigint
) RETURNS void AS $$
BEGIN
    DELETE FROM dbaas.pillar
     WHERE subcid = i_subcid;
    DELETE FROM dbaas.instance_groups
     WHERE subcid = i_subcid;
    DELETE FROM dbaas.subclusters
     WHERE subcid = i_subcid;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'delete_subcluster',
             jsonb_build_object(
                'subcid', i_subcid
            )
        )
    );
END;
$$ LANGUAGE plpgsql;

--head/code/57_add_instance_group_subcluster.sql
CREATE OR REPLACE FUNCTION code.add_instance_group_subcluster(
    i_cid         text,
    i_subcid      text,
    i_name        text,
    i_roles       dbaas.role_type[],
    i_rev         bigint
) RETURNS TABLE (cid text, subcid text, name text, roles dbaas.role_type[]) AS $$
BEGIN
    PERFORM code.add_subcluster(
        i_cid,
        i_subcid,
        i_name,
        i_roles,
        i_rev
    );

    INSERT INTO dbaas.instance_groups (
        subcid
    ) VALUES (
        i_subcid
    );

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_instance_group_subcluster',
            jsonb_build_object(
                'subcid', i_subcid,
                'name', i_name,
                'roles', i_roles::text[]
            )
        )
    );

    RETURN QUERY SELECT i_cid, i_subcid, i_name, i_roles;
END;
$$ LANGUAGE plpgsql;


--head/code/57_add_shard.sql
CREATE OR REPLACE FUNCTION code.add_shard(
    i_subcid    text,
    i_shard_id  text,
    i_name      text,
    i_cid       text,
    i_rev       bigint
) RETURNS TABLE (subcid text, shard_id text, name text) AS $$
BEGIN
    INSERT INTO dbaas.shards (
        subcid, shard_id,
        name
    ) VALUES (
        i_subcid, i_shard_id,
        i_name
    );

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_shard',
             jsonb_build_object(
                'subcid', i_subcid,
                'shard_id', i_shard_id,
                'name', i_name
            )
        )
    );

    RETURN QUERY SELECT i_subcid, i_shard_id, i_name;
END;
$$ LANGUAGE plpgsql;

--head/code/57_delete_shard.sql
CREATE OR REPLACE FUNCTION code.delete_shard(
    i_cid       text,
    i_shard_id  text,
    i_rev       bigint
) RETURNS void AS $$
BEGIN
    DELETE FROM dbaas.pillar
     WHERE shard_id = i_shard_id;

    DELETE FROM dbaas.shards
     WHERE shard_id = i_shard_id;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'delete_shard',
             jsonb_build_object(
                'shard_id', i_shard_id
            )
        )
    );
END;
$$ LANGUAGE plpgsql;

--head/code/57_update_instance_group.sql
CREATE OR REPLACE FUNCTION code.update_instance_group(
    i_cid                 text,
    i_instance_group_id   text,
    i_subcid              text,
    i_rev                 bigint
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.instance_groups
    SET instance_group_id = i_instance_group_id
    WHERE subcid = i_subcid;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find subcluster %', i_subcid
              USING TABLE = 'dbaas.instance_groups';
    END IF;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_instance_group',
            jsonb_build_object(
                'subcid', i_subcid,
                'instance_group_id', i_instance_group_id
            )
        )
    );

END;
$$ LANGUAGE plpgsql;

--head/code/60_add_hadoop_job.sql
CREATE OR REPLACE FUNCTION code.add_hadoop_job(
    i_job_id     text,
    i_cid        text,
    i_name       text,
    i_created_by text,
    i_job_spec   jsonb
) RETURNS TABLE (job_id text, cid text, name text, created_by text, job_spec jsonb) AS $$
BEGIN
    INSERT INTO dbaas.hadoop_jobs (
        job_id, cid, name, created_by, job_spec
    ) VALUES (
        i_job_id, i_cid, i_name, i_created_by, i_job_spec
    );

    RETURN QUERY SELECT i_job_id, i_cid, i_name, i_created_by, i_job_spec;
END;
$$ LANGUAGE plpgsql;

--head/code/60_add_host.sql
CREATE OR REPLACE FUNCTION code.add_host(
    i_subcid           text,
    i_shard_id         text,
    i_space_limit      bigint,
    i_flavor_id        uuid,
    i_geo              text,
    i_fqdn             text,
    i_disk_type        text,
    i_subnet_id        text,
    i_assign_public_ip boolean,
    i_cid              text,
    i_rev              bigint
) RETURNS SETOF code.host AS $$
DECLARE
    v_host      dbaas.hosts;
    v_geo       dbaas.geo;
    v_disk_type dbaas.disk_type;
    v_flavor    dbaas.flavors;
BEGIN
    SELECT *
      INTO v_geo
      FROM dbaas.geo
     WHERE name = i_geo;

    IF NOT found THEN
        RAISE EXCEPTION 'Can''t find geo_id for geo: %', i_geo
              USING TABLE = 'dbaas.geo';
    END IF;

    SELECT *
    INTO v_disk_type
    FROM dbaas.disk_type
    WHERE disk_type_ext_id = i_disk_type;

    IF NOT found THEN
        RAISE EXCEPTION
            'Can''t find disk_type_id for disk_type_ext_id: %', i_disk_type
            USING TABLE = 'dbaas.disk_type';
    END IF;

    SELECT *
    INTO v_flavor
    FROM dbaas.flavors
    WHERE id = i_flavor_id;

    IF NOT found THEN
        RAISE EXCEPTION
            'Can''t find flavor for id: %', i_flavor_id
            USING TABLE = 'dbaas.flavors';
    END IF;

    INSERT INTO dbaas.hosts
        (subcid, shard_id, space_limit,
        flavor, geo_id, fqdn, disk_type_id,
        subnet_id, assign_public_ip)
    VALUES
        (i_subcid, i_shard_id, i_space_limit,
        i_flavor_id, (v_geo).geo_id, i_fqdn,
        (v_disk_type).disk_type_id, i_subnet_id,
        i_assign_public_ip)
    RETURNING * INTO v_host;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_host',
            jsonb_build_object(
                'fqdn', i_fqdn
            )
        )
    );

    RETURN QUERY
        SELECT fmt.*
          FROM dbaas.subclusters s,
               dbaas.clusters c,
               code.format_host(v_host, c, s, v_geo, v_disk_type, v_flavor) fmt
         WHERE s.subcid = (v_host).subcid
           AND c.cid = s.cid;
END;
$$ LANGUAGE plpgsql;

--head/code/60_delete_hosts.sql
CREATE OR REPLACE FUNCTION code.delete_hosts(
    i_fqdns text[],
    i_cid   text,
    i_rev   bigint
) RETURNS SETOF code.host AS $$
DECLARE
    v_hosts dbaas.hosts[];
BEGIN
    DELETE FROM dbaas.pillar
     WHERE fqdn = ANY(i_fqdns);

    WITH deleted AS (
        DELETE FROM dbaas.hosts
         WHERE fqdn = ANY(i_fqdns)
        RETURNING *
    )
    SELECT array_agg(dh::dbaas.hosts)
      INTO v_hosts
      FROM deleted dh;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'delete_hosts',
            jsonb_build_object(
                'fqdns', i_fqdns
            )
        )
    );

    RETURN QUERY
        SELECT h.*
          FROM unnest(v_hosts) dh,
               code.format_host(dh) h;
END;
$$ LANGUAGE plpgsql;

--head/code/60_set_labels_on_cluster.sql
CREATE OR REPLACE FUNCTION code.set_labels_on_cluster(
    i_folder_id  bigint,
    i_cid        text,
    i_labels     code.label[],
    i_rev        bigint
) RETURNS void AS $$
BEGIN
    DELETE FROM dbaas.cluster_labels
     WHERE cid = i_cid;

    INSERT INTO dbaas.cluster_labels
        (cid, label_key, label_value)
    SELECT i_cid, key, value
      FROM unnest(i_labels);

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'set_labels_on_cluster',
            jsonb_build_object(
                'labels', i_labels
            )
        )
    );
END;
$$ LANGUAGE plpgsql;

--head/code/60_update_host.sql
CREATE OR REPLACE FUNCTION code.update_host(
    i_fqdn              text,
    i_cid               text,
    i_rev               bigint,
    i_space_limit       bigint DEFAULT NULL,
    i_flavor_id         uuid   DEFAULT NULL,
    i_vtype_id          text   DEFAULT NULL,
    i_disk_type         text   DEFAULT NULL,
    i_assign_public_ip  bool   DEFAULT NULL
) RETURNS SETOF code.host AS $$
DECLARE
    v_disk_type dbaas.disk_type;
BEGIN
    IF i_disk_type IS NOT NULL THEN
        SELECT *
        INTO v_disk_type
        FROM dbaas.disk_type
        WHERE disk_type_ext_id = i_disk_type;

        IF NOT found THEN
            RAISE EXCEPTION
                'Can''t find disk_type_id for disk_type_ext_id: %', i_disk_type
                USING TABLE = 'dbaas.disk_type';
        END IF;
    END IF;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_host',
            jsonb_build_object(
                'fqdn', i_fqdn
            )
        )
    );

RETURN QUERY
    WITH host_updated AS (
        UPDATE dbaas.hosts
           SET space_limit = coalesce(i_space_limit, space_limit),
               flavor = coalesce(i_flavor_id, flavor),
               vtype_id = coalesce(i_vtype_id, vtype_id),
               disk_type_id = coalesce((v_disk_type).disk_type_id, disk_type_id),
               assign_public_ip = coalesce(i_assign_public_ip, assign_public_ip)
         WHERE fqdn = i_fqdn
        RETURNING *)
    SELECT fmt.* 
      FROM host_updated h
      JOIN dbaas.subclusters s
     USING (subcid)
      JOIN dbaas.clusters c
     USING (cid)
      JOIN dbaas.geo g
     USING (geo_id)
      JOIN dbaas.disk_type d
     USING (disk_type_id)
      JOIN dbaas.flavors f
        ON (h.flavor = f.id),
           code.format_host(h, c, s, g, d, f) fmt;
END;
$$ LANGUAGE plpgsql;

--head/code/65_add_finished_operation.sql
CREATE OR REPLACE FUNCTION code.add_finished_operation(
    i_operation_id      text,
    i_cid               text,
    i_folder_id         bigint,
    i_operation_type    text,
    i_metadata          jsonb,
    i_user_id           text,
    i_version           integer,
    i_hidden            boolean DEFAULT false,
    i_idempotence_data  code.idempotence_data DEFAULT NULL,
    i_rev               bigint DEFAULT NULL
) RETURNS code.operation AS $$
DECLARE
    v_task    dbaas.worker_queue;
    v_cluster dbaas.clusters;
BEGIN
    INSERT INTO dbaas.worker_queue (
        task_id,
        cid,
        folder_id,
        result,
        start_ts,
        end_ts,
        task_type,
        task_args,
        created_by,
        operation_type,
        metadata,
        hidden,
        version,
        timeout,
        create_rev,
        acquire_rev,
        finish_rev,
        unmanaged
    )
    VALUES (
        i_operation_id,
        i_cid,
        i_folder_id,
        true,
        now(),
        now(),
        code.finished_operation_task_type(),
        '{}',
        i_user_id,
        i_operation_type,
        i_metadata,
        i_hidden,
        i_version,
        '1 second',
        i_rev,
        i_rev,
        i_rev,
        false
    )
    RETURNING * INTO v_task;

    SELECT *
      INTO v_cluster
      FROM dbaas.clusters
     WHERE cid = i_cid;

    PERFORM code.save_idempotence(
        i_operation_id     => i_operation_id,
        i_folder_id        => i_folder_id,
        i_user_id          => i_user_id,
        i_idempotence_data => i_idempotence_data
    );

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_finished_operation',
            jsonb_build_object(
                'operation_id', i_operation_id
            )
        )
    );

    RETURN code.as_operation(v_task, v_cluster);
END;
$$ LANGUAGE plpgsql;

--head/code/65_add_finished_operation_for_current_rev.sql
CREATE OR REPLACE FUNCTION code.add_finished_operation_for_current_rev(
    i_operation_id      text,
    i_cid               text,
    i_folder_id         bigint,
    i_operation_type    text,
    i_metadata          jsonb,
    i_user_id           text,
    i_version           integer,
    i_hidden            boolean DEFAULT false,
    i_idempotence_data  code.idempotence_data DEFAULT NULL,
    i_rev               bigint DEFAULT NULL
) RETURNS code.operation AS $$
DECLARE
    v_task    dbaas.worker_queue;
    v_cluster dbaas.clusters;
BEGIN
    INSERT INTO dbaas.worker_queue (
        task_id,
        cid,
        folder_id,
        result,
        start_ts,
        end_ts,
        task_type,
        task_args,
        created_by,
        operation_type,
        metadata,
        hidden,
        version,
        timeout,
        create_rev,
        acquire_rev,
        finish_rev,
        unmanaged
    )
    VALUES (
        i_operation_id,
        i_cid,
        i_folder_id,
        true,
        now(),
        now(),
        code.finished_operation_task_type(),
        '{}',
        i_user_id,
        i_operation_type,
        i_metadata,
        i_hidden,
        i_version,
        '1 second',
        i_rev,
        i_rev,
        i_rev,
        false
    )
    RETURNING * INTO v_task;

    SELECT *
      INTO v_cluster
      FROM dbaas.clusters
     WHERE cid = i_cid;

    PERFORM code.save_idempotence(
        i_operation_id     => i_operation_id,
        i_folder_id        => i_folder_id,
        i_user_id          => i_user_id,
        i_idempotence_data => i_idempotence_data
    );

    RETURN code.as_operation(v_task, v_cluster);
END;
$$ LANGUAGE plpgsql;

--head/code/65_add_operation.sql
CREATE OR REPLACE FUNCTION code.add_operation(
    i_operation_id          text,
    i_cid                   text,
    i_folder_id             bigint,
    i_operation_type        text,
    i_task_type             text,
    i_task_args             jsonb,
    i_metadata              jsonb,
    i_user_id               text,
    i_version               integer,
    i_hidden                boolean                  DEFAULT false,
    i_time_limit            interval                 DEFAULT NULL,
    i_idempotence_data      code.idempotence_data    DEFAULT NULL,
    i_delay_by              interval                 DEFAULT NULL,
    i_required_operation_id text                     DEFAULT NULL,
    i_rev                   bigint                   DEFAULT NULL,
    i_tracing               text                     DEFAULT NULL
) RETURNS code.operation AS $$
DECLARE
    v_task       dbaas.worker_queue;
    v_cluster    dbaas.clusters;
    v_new_status dbaas.cluster_status;
BEGIN
    SELECT *
      INTO v_cluster
      FROM dbaas.clusters
     WHERE cid = i_cid;

    -- change status only for realtime task (i_delay_by IS NULL)
    IF i_delay_by IS NULL THEN
        SELECT to_status
          INTO v_new_status
          FROM code.cluster_status_add_transitions() t
         WHERE from_status = (v_cluster).status
           AND action = code.task_type_action(i_task_type);

        IF NOT found THEN
            RAISE EXCEPTION 'Invalid cluster status % for adding %', (v_cluster).status, i_task_type
                  USING TABLE = 'dbaas.worker_queue';
        END IF;

        -- update v_cluster, cause we update our cluster
        UPDATE dbaas.clusters
           SET status = v_new_status
         WHERE cid = i_cid
        RETURNING * INTO v_cluster;
    END IF;

    INSERT INTO dbaas.worker_queue (
        task_id,
        cid,
        folder_id,
        task_type,
        task_args,
        created_by,
        operation_type,
        metadata,
        hidden,
        version,
        delayed_until,
        required_task_id,
        timeout,
        create_rev,
        unmanaged,
        tracing
    )
    VALUES (
        i_operation_id,
        i_cid,
        i_folder_id,
        i_task_type,
        i_task_args,
        i_user_id,
        i_operation_type,
        i_metadata,
        i_hidden,
        i_version,
        now() + i_delay_by,
        i_required_operation_id,
        coalesce(i_time_limit, '1 hour'),
        i_rev,
        false,
        i_tracing
    )
    RETURNING * INTO v_task;

    PERFORM code.save_idempotence(
        i_operation_id     => i_operation_id,
        i_folder_id        => i_folder_id,
        i_user_id          => i_user_id,
        i_idempotence_data => i_idempotence_data
    );

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_operation',
            jsonb_build_object(
                'operation_id', i_operation_id
            )
        )
    );
    RETURN code.as_operation(v_task, v_cluster);
END;
$$ LANGUAGE plpgsql;

--head/code/65_add_unmanaged_operation.sql
CREATE OR REPLACE FUNCTION code.add_unmanaged_operation(
    i_operation_id          text,
    i_cid                   text,
    i_folder_id             bigint,
    i_operation_type        text,
    i_task_type             text,
    i_task_args             jsonb,
    i_metadata              jsonb,
    i_user_id               text,
    i_version               integer,
    i_hidden                boolean                  DEFAULT false,
    i_time_limit            interval                 DEFAULT NULL,
    i_idempotence_data      code.idempotence_data    DEFAULT NULL,
    i_delay_by              interval                 DEFAULT NULL,
    i_required_operation_id text                     DEFAULT NULL,
    i_rev                   bigint                   DEFAULT NULL,
    i_tracing               text                     DEFAULT NULL
) RETURNS code.operation AS $$
DECLARE
    v_task       dbaas.worker_queue;
    v_cluster    dbaas.clusters;
BEGIN
    SELECT *
      INTO v_cluster
      FROM dbaas.clusters
     WHERE cid = i_cid;

    INSERT INTO dbaas.worker_queue (
        task_id,
        cid,
        start_ts,
        folder_id,
        task_type,
        task_args,
        created_by,
        operation_type,
        metadata,
        hidden,
        version,
        delayed_until,
        required_task_id,
        timeout,
        create_rev,
        unmanaged,
        tracing
    )
    VALUES (
        i_operation_id,
        i_cid,
        now(),
        i_folder_id,
        i_task_type,
        i_task_args,
        i_user_id,
        i_operation_type,
        i_metadata,
        i_hidden,
        i_version,
        now() + i_delay_by,
        i_required_operation_id,
        coalesce(i_time_limit, '1 day'),
        i_rev,
        true,
        i_tracing
    )
    RETURNING * INTO v_task;

    PERFORM code.save_idempotence(
        i_operation_id     => i_operation_id,
        i_folder_id        => i_folder_id,
        i_user_id          => i_user_id,
        i_idempotence_data => i_idempotence_data
    );

    RETURN code.as_operation(v_task, v_cluster);
END;
$$ LANGUAGE plpgsql;

--head/code/65_reset_cluster_to_rev.sql
CREATE OR REPLACE FUNCTION code.reset_cluster_to_rev(
    i_cid    text,
    i_rev    bigint
) RETURNS void AS $$
DECLARE
    v_head_coords code.cluster_coords;
    v_rev_coords  code.cluster_coords;

    v_old_cluster_revs dbaas.clusters_revs;
BEGIN
    SELECT *
    INTO v_old_cluster_revs
    FROM dbaas.clusters_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find clusters_revs cid=%, rev=%', i_cid, i_rev
            USING TABLE = 'dbaas.cluster_revs';
    END IF;

    v_head_coords := code.get_coords(i_cid);
    v_rev_coords  := code.get_coords_at_rev(i_cid, i_rev);

    -- delete all from pillar to subcluster
    DELETE FROM dbaas.cluster_labels
    WHERE cid = i_cid;

    DELETE FROM dbaas.pillar
    WHERE fqdn IS NOT NULL
      AND fqdn = ANY((v_head_coords).fqdns);

    DELETE FROM dbaas.pillar
    WHERE shard_id IS NOT NULL
      AND shard_id = ANY((v_head_coords).shard_ids);

    DELETE FROM dbaas.pillar
    WHERE subcid IS NOT NULL
      AND subcid = ANY((v_head_coords).subcids);

    DELETE FROM dbaas.pillar
    WHERE cid = i_cid;

    DELETE FROM dbaas.disks
    WHERE cid = i_cid;

    DELETE FROM dbaas.hosts
    WHERE fqdn = ANY((v_head_coords).fqdns);

    DELETE FROM dbaas.shards
    WHERE shard_id = ANY((v_head_coords).shard_ids);

    DELETE FROM dbaas.instance_groups
    WHERE subcid = ANY((v_head_coords).subcids);

    DELETE FROM dbaas.sgroups
    WHERE cid = i_cid;

    DELETE FROM dbaas.subclusters
    WHERE subcid = ANY((v_head_coords).subcids);

    DELETE FROM dbaas.maintenance_window_settings
    WHERE cid = i_cid;

    DELETE FROM dbaas.backup_schedule
    WHERE cid = i_cid;

    DELETE FROM dbaas.versions
    WHERE cid IS NOT NULL
      AND cid = i_cid;

    DELETE FROM dbaas.versions
    WHERE shard_id IS NOT NULL
      AND shard_id = ANY((v_head_coords).shard_ids);

    DELETE FROM dbaas.versions
    WHERE subcid IS NOT NULL
      AND subcid = ANY((v_head_coords).subcids);

    DELETE FROM dbaas.disk_placement_groups
    WHERE cid = i_cid;

    DELETE FROM dbaas.placement_groups
    WHERE cid = i_cid;

    -- restore from subcluster to pillar
    INSERT INTO dbaas.subclusters
    (cid, subcid, name, roles, created_at)
    SELECT
        cid, subcid, name, roles, created_at
    FROM dbaas.subclusters_revs
    WHERE subcid = ANY((v_rev_coords).subcids)
      AND rev = i_rev;

    INSERT INTO dbaas.instance_groups
    (instance_group_id, subcid)
    SELECT
        instance_group_id, subcid
    FROM dbaas.instance_groups_revs
    WHERE subcid = ANY((v_rev_coords).subcids)
      AND rev = i_rev;

    INSERT INTO dbaas.sgroups
    (cid, sg_ext_id, sg_type, sg_hash, sg_allow_all)
    SELECT
        cid, sg_ext_id, sg_type, sg_hash, sg_allow_all
    FROM dbaas.sgroups_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.shards
    (subcid, shard_id, name, created_at)
    SELECT
        subcid, shard_id, name, created_at
    FROM dbaas.shards_revs
    WHERE shard_id = ANY((v_rev_coords).shard_ids)
      AND rev = i_rev;

    INSERT INTO dbaas.hosts
    (subcid, shard_id, flavor, space_limit, fqdn, vtype_id,
     geo_id, disk_type_id, subnet_id, assign_public_ip, created_at)
    SELECT
        subcid, shard_id, flavor, space_limit, fqdn, vtype_id,
        geo_id, disk_type_id, subnet_id, assign_public_ip, created_at
    FROM dbaas.hosts_revs
    WHERE fqdn = ANY((v_rev_coords).fqdns)
      AND rev = i_rev;

    INSERT INTO dbaas.pillar
    (cid, value)
    SELECT cid, value
    FROM dbaas.pillar_revs
    WHERE cid IS NOT NULL
      AND cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.pillar
    (subcid, value)
    SELECT subcid, value
    FROM dbaas.pillar_revs
    WHERE subcid IS NOT NULL
      AND subcid = ANY((v_rev_coords).subcids)
      AND rev = i_rev;

    INSERT INTO dbaas.pillar
    (shard_id, value)
    SELECT shard_id, value
    FROM dbaas.pillar_revs
    WHERE shard_id IS NOT NULL
      AND shard_id = ANY((v_rev_coords).shard_ids)
      AND rev = i_rev;

    INSERT INTO dbaas.pillar
    (fqdn, value)
    SELECT fqdn, value
    FROM dbaas.pillar_revs
    WHERE fqdn IS NOT NULL
      AND fqdn = ANY((v_rev_coords).fqdns)
      AND rev = i_rev;

    INSERT INTO dbaas.cluster_labels
    (cid, label_key, label_value)
    SELECT
        cid, label_key, label_value
    FROM dbaas.cluster_labels_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.maintenance_window_settings
    (cid, day, hour)
    SELECT
        cid, day, hour
    FROM dbaas.maintenance_window_settings_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.backup_schedule
    (cid, schedule)
    SELECT
        cid, schedule
    FROM dbaas.backup_schedule_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.versions
    ( cid, subcid, shard_id, component, major_version, minor_version, package_version, edition )
    SELECT
        cid, subcid, shard_id, component, major_version, minor_version, package_version, edition
    FROM dbaas.versions_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.versions
    ( cid, subcid, shard_id, component, major_version, minor_version, package_version, edition )
    SELECT
        cid, subcid, shard_id, component, major_version, minor_version, package_version, edition
    FROM dbaas.versions_revs
    WHERE subcid IS NOT NULL
      AND subcid = ANY((v_rev_coords).subcids)
      AND rev = i_rev;

    INSERT INTO dbaas.versions
    ( cid, subcid, shard_id, component, major_version, minor_version, package_version, edition )
    SELECT
        cid, subcid, shard_id, component, major_version, minor_version, package_version, edition
    FROM dbaas.versions_revs
    WHERE shard_id IS NOT NULL
      AND shard_id = ANY((v_rev_coords).shard_ids)
      AND rev = i_rev;

    INSERT INTO dbaas.disk_placement_groups
    ( pg_id, cid, local_id, disk_placement_group_id, status )
    SELECT
        pg_id, cid, local_id, disk_placement_group_id, status
    FROM dbaas.disk_placement_groups_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.disks
    ( d_id, pg_id, fqdn, mount_point, disk_id, host_disk_id, status, cid )
    SELECT
        d_id, pg_id, fqdn, mount_point, disk_id, host_disk_id, status, cid
    FROM dbaas.disks_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.placement_groups
    ( pg_id, cid, subcid, shard_id, placement_group_id, status )
    SELECT
        pg_id, cid, subcid, shard_id, placement_group_id, status
    FROM dbaas.placement_groups_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    UPDATE dbaas.clusters
    SET name = (v_old_cluster_revs).name,
        description = (v_old_cluster_revs).description,
        network_id = (v_old_cluster_revs).network_id,
        folder_id = (v_old_cluster_revs).folder_id,
        status = (v_old_cluster_revs).status,
        host_group_ids = (v_old_cluster_revs).host_group_ids
    WHERE cid = i_cid;
END;
$$ LANGUAGE plpgsql;

--head/code/66_acquire_task.sql
CREATE OR REPLACE FUNCTION code.acquire_task(
    i_worker_id code.worker_id,
    i_task_id   code.task_id
) RETURNS code.worker_task AS $$
DECLARE
    v_task            dbaas.worker_queue;
    v_folder          dbaas.folders;
    v_new_status      dbaas.cluster_status;
    v_required_result boolean;
    v_rev             bigint;
BEGIN
    v_rev := code.lock_cluster_by_task(i_task_id);

    UPDATE dbaas.worker_queue
       SET start_ts = now(),
           worker_id = i_worker_id,
           acquire_rev = v_rev
     WHERE task_id = i_task_id
       AND start_ts IS NULL
       AND worker_id is NULL
       AND unmanaged = false
    RETURNING * INTO v_task;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to acquire task %, %', i_task_id, code.describe_not_acquired_task(i_worker_id, i_task_id)
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    IF v_task.required_task_id IS NOT NULL THEN
        SELECT result
          INTO v_required_result
          FROM dbaas.worker_queue
         WHERE task_id = v_task.required_task_id;

        IF (v_required_result IS NULL OR v_required_result = false) THEN
            RAISE EXCEPTION 'Unable to acquire task %, because required task % has result %s', v_task.task_id, v_task.required_task_id, v_required_result;
        END IF;
    END IF;

    SELECT to_status
      INTO v_new_status
      FROM code.cluster_status_acquire_transitions() t
      JOIN dbaas.clusters c ON (c.status = t.from_status)
     WHERE c.cid = v_task.cid
       AND t.action = code.task_type_action(v_task.task_type);

    IF found THEN
        UPDATE dbaas.clusters
           SET status = v_new_status
         WHERE cid = v_task.cid;
    END IF;

    PERFORM code.update_cluster_change(
        (v_task).cid,
        v_rev,
        jsonb_build_object(
            'acquire_task',
            jsonb_build_object(
                'task_id', i_task_id
            )
        )
    );

    PERFORM code.complete_cluster_change(
        (v_task).cid,
        v_rev
    );

    SELECT *
      INTO v_folder
      FROM dbaas.folders
     WHERE folders.folder_id = (v_task).folder_id;

    RETURN code.as_worker_task(
        v_task,
        v_folder);
END;
$$ LANGUAGE plpgsql;

--head/code/66_acquire_transition_exists.sql
CREATE OR REPLACE FUNCTION code.acquire_transition_exists(
    dbaas.worker_queue
) RETURNS boolean AS $$
SELECT EXISTS (
    SELECT 1
      FROM code.cluster_status_acquire_transitions() t
      JOIN dbaas.clusters c ON (c.status = t.from_status)
     WHERE c.cid = $1.cid
       AND t.action = code.task_type_action($1.task_type)
);
$$ LANGUAGE SQL STABLE;

--head/code/66_finish_task.sql
CREATE OR REPLACE FUNCTION code.finish_task(
    i_worker_id code.worker_id,
    i_task_id   code.task_id,
    i_result    boolean,
    i_changes   jsonb,
    i_comment   text,
    i_errors    jsonb DEFAULT NULL
) RETURNS void AS $$
DECLARE
    v_task dbaas.worker_queue;
    v_new_status dbaas.cluster_status;
    v_rev        bigint;
BEGIN
    v_rev := code.lock_cluster_by_task(i_task_id);

    UPDATE dbaas.worker_queue
       SET end_ts = now(),
           result = i_result,
           changes = i_changes,
           comment = i_comment,
           errors = i_errors,
           context = null,
           finish_rev = v_rev
     WHERE task_id = i_task_id
       AND worker_id IS NOT DISTINCT FROM i_worker_id
       AND unmanaged = false
    RETURNING * INTO v_task;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to finish task %, %', i_task_id, code.describe_not_acquired_task(i_worker_id, i_task_id)
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    IF v_task.config_id IS NOT NULL THEN
        UPDATE dbaas.maintenance_tasks
        SET status = (CASE WHEN i_result THEN 'COMPLETED'::dbaas.maintenance_task_status ELSE 'FAILED'::dbaas.maintenance_task_status END)
        WHERE cid = v_task.cid AND config_id=v_task.config_id;
    END IF;

    SELECT to_status
      INTO v_new_status
      FROM code.cluster_status_finish_transitions() t
      JOIN dbaas.clusters c ON (c.status = t.from_status)
     WHERE c.cid = v_task.cid
       AND t.action = code.task_type_action(v_task.task_type)
       AND t.result = i_result;

    IF found THEN
        UPDATE dbaas.clusters
           SET status = v_new_status
         WHERE cid = v_task.cid;
    END IF;


    PERFORM code.update_cluster_change(
        (v_task).cid,
        v_rev,
        jsonb_build_object(
            'finish_task',
            jsonb_build_object(
                'task_id', i_task_id
            )
        )
    );

    PERFORM code.complete_cluster_change(
        (v_task).cid,
        v_rev
    );

END;
$$ LANGUAGE plpgsql;

--head/code/66_finish_unmanaged_task.sql
CREATE OR REPLACE FUNCTION code.finish_unmanaged_task(
    i_task_id   code.task_id,
    i_result    boolean,
    i_changes   jsonb,
    i_comment   text,
    i_errors    jsonb DEFAULT NULL
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.worker_queue
       SET end_ts = now(),
           result = i_result,
           changes = i_changes,
           comment = i_comment,
           errors = i_errors,
           context = null
     WHERE task_id = i_task_id
       AND unmanaged = true;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to finish unmanaged task %', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

END;
$$ LANGUAGE plpgsql;

--head/code/66_increment_failed_acquire_count.sql
CREATE OR REPLACE FUNCTION code.increment_failed_acquire_count(
    i_task_id   code.task_id
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.worker_queue
       SET failed_acquire_count = failed_acquire_count + 1
     WHERE task_id = i_task_id
       AND unmanaged = false;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to increment failed acquire count for task %: task not found', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;
END;
$$ LANGUAGE plpgsql;

--head/code/66_lock_cluster_by_task.sql
CREATE OR REPLACE FUNCTION code.lock_cluster_by_task(
    i_task_id text
) RETURNS bigint AS $$
DECLARE
    v_cid         text;
    v_target_rev  bigint;
    v_actual_rev  bigint;
    v_acquire_rev bigint;
BEGIN
    SELECT c.cid, wq.target_rev, c.actual_rev, wq.acquire_rev
    INTO v_cid, v_target_rev, v_actual_rev, v_acquire_rev
    FROM dbaas.worker_queue wq
             JOIN dbaas.clusters c USING (cid)
    WHERE task_id = i_task_id;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to lock cluster by task %. No task with that task_id found', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    IF v_target_rev IS NOT NULL AND v_acquire_rev IS NULL THEN
        IF v_actual_rev >= v_target_rev THEN
            RAISE EXCEPTION 'In task % target rev need to be future revision, actual %, target %', i_task_id, v_actual_rev, v_target_rev
                USING TABLE = 'dbaas.worker_queue';
        END IF;
        PERFORM code.reset_cluster_to_rev(v_cid, v_target_rev);
        RETURN (code.forward_cluster_revision(v_cid)).actual_rev;
    END IF;

    RETURN (code.lock_cluster(v_cid)).rev;
END;
$$ LANGUAGE plpgsql;

--head/code/66_release_task.sql
CREATE OR REPLACE FUNCTION code.release_task(
    i_worker_id code.worker_id,
    i_task_id   code.task_id,
    i_context   jsonb
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.worker_queue
       SET changes     = NULL,
           comment     = NULL,
           start_ts    = NULL,
           end_ts      = NULL,
           result      = NULL,
           worker_id   = NULL,
           errors      = NULL,
           context     = i_context
     WHERE task_id = i_task_id
       AND unmanaged = false
       AND worker_id IS NOT DISTINCT FROM i_worker_id;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to release task %, %', i_task_id, code.describe_not_acquired_task(i_worker_id, i_task_id)
              USING TABLE = 'dbaas.worker_queue';
    END IF;
END;
$$ LANGUAGE plpgsql;

--head/code/66_terminate_hadoop_jobs.sql
CREATE OR REPLACE FUNCTION code.terminate_hadoop_jobs(
   i_cid                   text
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.hadoop_jobs
       SET status = 'ERROR',
           start_ts = COALESCE(start_ts, now()),
           end_ts = COALESCE(end_ts, now()),
           comment = 'The job was terminated by cluster operation.'
     WHERE cid = i_cid
       AND status NOT IN ('ERROR', 'DONE', 'CANCELLED');

    UPDATE dbaas.worker_queue
       SET start_ts = COALESCE(start_ts, now()),
           end_ts = COALESCE(end_ts, now()),
           errors = '[{"code": 1, "type": "Cancelled", "message": "The job was terminated", "exposable": true}]'::jsonb,
           result = false
     WHERE cid = i_cid 
       AND result is null
       AND unmanaged = true;

END;
$$ LANGUAGE plpgsql;

--head/code/66_update_hadoop_job_status.sql
CREATE OR REPLACE FUNCTION code.update_hadoop_job_status(
    i_job_id           text,
    i_status           dbaas.hadoop_jobs_status,
    i_application_info jsonb DEFAULT NULL
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.hadoop_jobs
       SET status = i_status,
           application_info = COALESCE(i_application_info, application_info),
           start_ts = CASE WHEN i_status >= 'RUNNING' THEN COALESCE(start_ts, now()) ELSE start_ts END,
           end_ts = CASE WHEN i_status IN ('ERROR', 'DONE', 'CANCELLED') THEN COALESCE(end_ts, now()) ELSE end_ts END
     WHERE job_id = i_job_id
       AND status NOT IN ('ERROR', 'DONE', 'CANCELLED');

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to update job %. Wrong job_id or it is in terminal state', i_job_id;
    END IF;
END;
$$ LANGUAGE plpgsql;

--head/code/66_update_task.sql
CREATE OR REPLACE FUNCTION code.update_task(
    i_worker_id code.worker_id,
    i_task_id   code.task_id,
    i_changes   jsonb,
    i_comment   text,
    i_context   jsonb DEFAULT NULL
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.worker_queue
       SET changes = i_changes,
           comment = i_comment,
           context = i_context
     WHERE task_id = i_task_id
       AND unmanaged = false
       AND worker_id IS NOT DISTINCT FROM i_worker_id;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to update task %, %', i_task_id, code.describe_not_acquired_task(i_worker_id, i_task_id)
              USING TABLE = 'dbaas.worker_queue';
    END IF;
END;
$$ LANGUAGE plpgsql;

--head/code/67_get_reject_rev.sql
CREATE OR REPLACE FUNCTION code.get_reject_rev(
    i_task_id   code.task_id,
    i_force     boolean       DEFAULT false
) RETURNS bigint AS $$
DECLARE
    v_task    dbaas.worker_queue;
    v_rev     bigint;
BEGIN
    SELECT *
      INTO v_task
      FROM dbaas.worker_queue
     WHERE task_id = i_task_id
       AND unmanaged = false
       FOR NO KEY UPDATE;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to get task, probably % is not a task_id', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    IF v_task.result IS NOT NULL THEN
        RAISE EXCEPTION 'This task % is in terminal state, it is %', i_task_id, code.task_status(v_task);
    END IF;

    IF NOT i_force AND coalesce(v_task.restart_count, 0) > 0 THEN
        RAISE EXCEPTION 'This task % has non-zero restart count: %s', i_task_id, v_task.restart_count;
    END IF;

    SELECT finish_rev q
      INTO v_rev
      FROM dbaas.worker_queue q
      JOIN dbaas.clusters c ON (c.cid = q.cid)
     WHERE q.cid = v_task.cid
       AND q.result = true
       AND q.unmanaged = false
       AND q.finish_rev IS NOT NULL
       AND code.visible(c)
  ORDER BY q.finish_rev DESC
     LIMIT 1;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find safe revision for rejecting task %s', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    RETURN v_rev;
END;
$$ LANGUAGE plpgsql;

--head/code/67_restart_task.sql
CREATE OR REPLACE FUNCTION code.restart_task(
    i_task_id code.task_id,
    i_force   boolean       DEFAULT false
) RETURNS void AS $$
DECLARE
    v_task_row   dbaas.worker_queue;
    v_cluster    dbaas.clusters;
    v_rev        bigint;
BEGIN
    SELECT *
      INTO v_task_row
      FROM dbaas.worker_queue
     WHERE task_id = i_task_id
       AND unmanaged = false
       FOR NO KEY UPDATE;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to restart task,'
                        ' probably % is not task_id', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    IF v_task_row.result IS NULL THEN
        RAISE EXCEPTION 'This task % is not in terminal state,'
                        ' it %', i_task_id, code.task_status(v_task_row);
    END IF;

    v_rev := (code.lock_cluster(v_task_row.cid)).rev;

    IF NOT i_force THEN
        SELECT *
          INTO v_cluster
          FROM dbaas.clusters
         WHERE cid = v_task_row.cid;

        IF NOT EXISTS (
            SELECT to_status
              FROM code.cluster_status_acquire_transitions() t
             WHERE from_status = v_cluster.status
               AND action = code.task_type_action(v_task_row.task_type))
        THEN
            RAISE EXCEPTION 'Invalid cluster status % for restarting %', v_cluster.status, v_task_row.task_type
                    USING TABLE = 'dbaas.worker_queue';
        END IF;
    END IF;

    UPDATE dbaas.worker_queue
       SET changes       = NULL,
           comment       = NULL,
           start_ts      = NULL,
           end_ts        = NULL,
           result        = NULL,
           worker_id     = NULL,
           errors        = NULL,
           restart_count = coalesce(restart_count, 0) + 1
     WHERE task_id = i_task_id;

    PERFORM code.update_cluster_change(
        (v_task_row).cid,
        v_rev,
        jsonb_build_object(
            'restart_task',
            jsonb_build_object(
                'task_id', i_task_id
            )
        )
    );

    PERFORM code.complete_cluster_change(
        (v_task_row).cid,
        v_rev
    );
END;
$$ LANGUAGE plpgsql;

--head/code/67_revert_cluster_to_rev.sql
CREATE OR REPLACE FUNCTION code.revert_cluster_to_rev(
    i_cid    text,
    i_rev    bigint,
    i_reason text
) RETURNS bigint AS $$
DECLARE
    v_locked_cluster   code.cluster_with_labels;
BEGIN
    v_locked_cluster := code.lock_cluster(
      i_cid          => i_cid,
      i_x_request_id => i_reason);

    PERFORM code.reset_cluster_to_rev(
        i_cid => i_cid,
        i_rev => i_rev
    );

    PERFORM code.update_cluster_change(
        i_cid,
        (v_locked_cluster).rev,
        jsonb_build_object(
            'revert_cluster_to_rev',
             jsonb_build_object(
                'i_rev', i_rev
            )
        )
    );

    PERFORM code.complete_cluster_change(i_cid, (v_locked_cluster).rev);

    RETURN (v_locked_cluster).rev;
END;
$$ LANGUAGE plpgsql;

--head/code/68_get_mysql_cluster_nodes.sql
CREATE OR REPLACE FUNCTION code.get_mysql_cluster_nodes(
    i_fqdn      text
) RETURNS jsonb AS $$
SELECT
    jsonb_strip_nulls(jsonb_build_object('ha', jsonb_agg(node_fqdn) FILTER (WHERE replication_source_fqdn IS NULL),
        'cascade_replicas', jsonb_agg(node_fqdn) FILTER (WHERE replication_source_fqdn = i_fqdn)))
FROM (
    SELECT hosts.fqdn AS node_fqdn, value->'data'->'mysql'->>'replication_source' replication_source_fqdn FROM dbaas.hosts
        LEFT JOIN dbaas.pillar USING (fqdn)
        WHERE hosts.subcid = (SELECT subcid FROM dbaas.hosts WHERE fqdn = i_fqdn)
        ORDER BY 1
) x;
$$ LANGUAGE SQL STABLE;

--head/code/68_get_pg_cluster_nodes.sql
CREATE OR REPLACE FUNCTION code.get_pg_cluster_nodes(
    i_fqdn      text
) RETURNS jsonb AS $$
SELECT
    jsonb_strip_nulls(jsonb_build_object('ha', jsonb_agg(node_fqdn) FILTER (WHERE replication_source_fqdn IS NULL),
        'cascade_replicas', jsonb_agg(node_fqdn) FILTER (WHERE replication_source_fqdn = i_fqdn)))
FROM (
    SELECT hosts.fqdn AS node_fqdn, value->'data'->'pgsync'->>'replication_source' replication_source_fqdn FROM dbaas.hosts
        LEFT JOIN dbaas.pillar USING (fqdn)
        WHERE hosts.subcid = (SELECT subcid FROM dbaas.hosts WHERE fqdn = i_fqdn)
        ORDER BY 1
) x;
$$ LANGUAGE SQL STABLE;

--head/code/68_get_rev_mysql_cluster_nodes.sql
CREATE OR REPLACE FUNCTION code.get_rev_mysql_cluster_nodes(
    i_fqdn      text,
    i_rev       bigint
) RETURNS jsonb AS $$
SELECT
    jsonb_strip_nulls(jsonb_build_object('ha', jsonb_agg(node_fqdn) FILTER (WHERE replication_source_fqdn IS NULL),
        'cascade_replicas', jsonb_agg(node_fqdn) FILTER (WHERE replication_source_fqdn = i_fqdn)))
FROM (
    SELECT hosts_revs.fqdn AS node_fqdn, value->'data'->'mysql'->>'replication_source' replication_source_fqdn FROM dbaas.hosts_revs
        LEFT JOIN dbaas.pillar_revs USING (fqdn)
        WHERE hosts_revs.subcid = (SELECT subcid FROM dbaas.hosts_revs WHERE fqdn = i_fqdn AND rev = i_rev)
          AND hosts_revs.rev = i_rev
          AND pillar_revs.rev = i_rev
        ORDER BY 1
) x;
$$ LANGUAGE SQL STABLE;

--head/code/68_get_rev_pg_cluster_nodes.sql
CREATE OR REPLACE FUNCTION code.get_rev_pg_cluster_nodes(
    i_fqdn      text,
    i_rev       bigint
) RETURNS jsonb AS $$
SELECT
    jsonb_strip_nulls(jsonb_build_object('ha', jsonb_agg(node_fqdn) FILTER (WHERE replication_source_fqdn IS NULL),
        'cascade_replicas', jsonb_agg(node_fqdn) FILTER (WHERE replication_source_fqdn = i_fqdn)))
FROM (
    SELECT hosts_revs.fqdn AS node_fqdn, value->'data'->'pgsync'->>'replication_source' replication_source_fqdn FROM dbaas.hosts_revs
        LEFT JOIN dbaas.pillar_revs USING (fqdn)
        WHERE hosts_revs.subcid = (SELECT subcid FROM dbaas.hosts_revs WHERE fqdn = i_fqdn AND rev = i_rev)
          AND hosts_revs.rev = i_rev
          AND pillar_revs.rev = i_rev
        ORDER BY 1
) x;
$$ LANGUAGE SQL STABLE;

--head/code/68_reject_task.sql
CREATE OR REPLACE FUNCTION code.reject_task(
    i_worker_id code.worker_id,
    i_task_id   code.task_id,
    i_changes   jsonb,
    i_comment   text,
    i_errors    jsonb           DEFAULT NULL,
    i_force     boolean         DEFAULT false
) RETURNS void AS $$
DECLARE
    v_rev       bigint;
    v_new_rev   bigint;
    v_cluster   dbaas.clusters;
    v_cloud     dbaas.clouds;
    v_new_cloud dbaas.clouds;
    v_task      dbaas.worker_queue;
BEGIN
    -- Lock cloud without getting revision
    SELECT c.*
      INTO v_cloud
      FROM dbaas.worker_queue q
           JOIN dbaas.folders f ON (f.folder_id = q.folder_id)
           JOIN dbaas.clouds c ON (c.cloud_id = f.cloud_id)
     WHERE q.task_id = i_task_id
       FOR NO KEY UPDATE;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to lock cloud for task %', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    -- Also lock (possibly) new cloud
    SELECT cl.*
      INTO v_new_cloud
      FROM dbaas.worker_queue q
           JOIN dbaas.clusters c ON (q.cid = c.cid)
           JOIN dbaas.folders f ON (f.folder_id = c.folder_id)
           JOIN dbaas.clouds cl ON (cl.cloud_id = f.cloud_id)
     WHERE q.task_id = i_task_id
       FOR NO KEY UPDATE;

    -- Lock cluster without getting revision
    SELECT c.*
      INTO v_cluster
      FROM dbaas.worker_queue q
           JOIN dbaas.clusters c ON (q.cid = c.cid)
     WHERE q.task_id = i_task_id
       FOR NO KEY UPDATE;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to lock cluster for task %', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    v_rev := code.get_reject_rev(i_task_id, i_force);

    v_new_rev := code.revert_cluster_to_rev(v_cluster.cid, v_rev, i_task_id || ' reject');

    PERFORM code.fix_cloud_usage(v_cloud.cloud_ext_id);

    IF (v_cloud).cloud_ext_id != (v_new_cloud).cloud_ext_id THEN
        PERFORM code.fix_cloud_usage(v_new_cloud.cloud_ext_id);
    END IF;

    UPDATE dbaas.worker_queue
       SET end_ts = now(),
           result = false,
           changes = i_changes,
           comment = i_comment,
           errors = i_errors,
           context = null,
           finish_rev = v_new_rev
     WHERE task_id = i_task_id
       AND worker_id IS NOT DISTINCT FROM i_worker_id
       AND unmanaged = false
     RETURNING * INTO v_task;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to reject task %, %', i_task_id, code.describe_not_acquired_task(i_worker_id, i_task_id)
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    IF v_task.config_id IS NOT NULL THEN
        UPDATE dbaas.maintenance_tasks
        SET status = 'REJECTED'
        WHERE cid = v_task.cid AND config_id=v_task.config_id;
    END IF;

    UPDATE dbaas.worker_queue
       SET start_ts = now(),
           end_ts = now(),
           worker_id = i_worker_id,
           result = false,
           comment = 'Failed due to reject of ' || i_task_id,
           errors = i_errors
     WHERE required_task_id = i_task_id
       AND unmanaged = false;
END;
$$ LANGUAGE plpgsql;

--head/code/69_get_custom_pillar_by_host.sql
CREATE OR REPLACE FUNCTION code.get_custom_pillar_by_host(
    i_fqdn      text
) RETURNS jsonb AS $$
DECLARE
    v_ctype dbaas.cluster_type;
BEGIN
    SELECT type INTO v_ctype
        FROM dbaas.clusters
        JOIN dbaas.subclusters USING (cid)
        JOIN dbaas.hosts USING (subcid)
        WHERE fqdn = i_fqdn;
    IF v_ctype = 'postgresql_cluster' THEN
        RETURN concat('{"data": {"cluster_nodes":', (SELECT code.get_pg_cluster_nodes(i_fqdn)), '}}')::jsonb;
    ELSIF v_ctype = 'mysql_cluster' THEN
        RETURN concat('{"data": {"cluster_nodes":', (SELECT code.get_mysql_cluster_nodes(i_fqdn)), '}}')::jsonb;
    ELSE
        RETURN '{}'::jsonb;
    END IF;
END
$$ LANGUAGE plpgsql STABLE;

--head/code/69_get_hosts_by_shard.sql
CREATE OR REPLACE FUNCTION code.get_hosts_by_shard(
    i_shard_id text,
    i_visibility code.visibility DEFAULT 'visible'
) RETURNS SETOF code.host AS $$
SELECT fmt.*
  FROM dbaas.shards s
  JOIN dbaas.subclusters sc USING (subcid)
  JOIN dbaas.clusters c USING (cid)
  JOIN dbaas.hosts h USING (shard_id)
  JOIN dbaas.geo g USING (geo_id)
  JOIN dbaas.disk_type d USING (disk_type_id)
  JOIN dbaas.flavors f ON (h.flavor = f.id),
       code.format_host(h, c, sc, g, d, f) fmt
 WHERE s.shard_id = i_shard_id
   AND code.match_visibility(c, i_visibility);
$$ LANGUAGE SQL STABLE;

--head/code/69_get_rev_custom_pillar_by_host.sql
CREATE OR REPLACE FUNCTION code.get_rev_custom_pillar_by_host(
    i_fqdn         text,
    i_cluster_type dbaas.cluster_type,
    i_rev          bigint
) RETURNS jsonb AS $$
DECLARE
BEGIN
    IF i_cluster_type = 'postgresql_cluster' THEN
        RETURN concat('{"data": {"cluster_nodes":', (SELECT code.get_rev_pg_cluster_nodes(i_fqdn, i_rev)), '}}')::jsonb;
    ELSIF i_cluster_type = 'mysql_cluster' THEN
        RETURN concat('{"data": {"cluster_nodes":', (SELECT code.get_rev_mysql_cluster_nodes(i_fqdn, i_rev)), '}}')::jsonb;
    ELSE
        RETURN '{}'::jsonb;
    END IF;
END
$$ LANGUAGE plpgsql STABLE;

--head/code/69_reject_failed_task.sql
CREATE OR REPLACE FUNCTION code.reject_failed_task(
    i_task_id   code.task_id,
    i_worker_id code.worker_id DEFAULT current_user,
    i_comment   text DEFAULT ''
) RETURNS void AS $$
BEGIN
    PERFORM code.restart_task(i_task_id);
    PERFORM code.acquire_task(i_worker_id, i_task_id);
    PERFORM code.reject_task(i_worker_id, i_task_id, '{}'::jsonb, i_comment, i_force => true);
END;
$$ LANGUAGE plpgsql;

--head/code/70_get_managed_config.sql
CREATE OR REPLACE FUNCTION code.dynamic_io_limit(
    i_space_limit     bigint,
    i_disk_type       dbaas.disk_type,
    i_static_io_limit bigint
) RETURNS bigint AS $$
SELECT COALESCE(
    LEAST(
        CEIL(i_space_limit / (i_disk_type).allocation_unit_size)::bigint * (i_disk_type).io_limit_per_allocation_unit,
        (i_disk_type).io_limit_max
        ),
    i_static_io_limit
    );
$$ LANGUAGE SQL IMMUTABLE;


CREATE OR REPLACE FUNCTION code.get_managed_config(
    i_fqdn      text,
    i_target_id text   DEFAULT NULL,
    i_rev       bigint DEFAULT NULL
) RETURNS jsonb AS $$
DECLARE
    v_rev            bigint;
    v_cluster        dbaas.clusters;
    v_config         jsonb[];
    v_custom_pillar  jsonb;
    v_derived_config jsonb;
    v_backup_schedule jsonb;
    v_host           dbaas.hosts;
    v_disk_type      dbaas.disk_type;
    v_flavor         dbaas.flavors;
    v_subcluster     dbaas.subclusters;
    v_shard          dbaas.shards;
    v_folder         dbaas.folders;
    v_cloud          dbaas.clouds;
    v_geo            dbaas.geo;
    component        dbaas.role_type;
    host             code.config_host;
    v_version        code.version;
    v_cluster_hosts  jsonb;
BEGIN
    IF i_rev IS NULL THEN
        SELECT c.actual_rev
          INTO v_rev
          FROM dbaas.clusters c
               JOIN dbaas.subclusters sc USING (cid)
               JOIN dbaas.hosts h USING (subcid)
         WHERE h.fqdn = i_fqdn
               AND code.visible(c)
               AND code.managed(c);

        IF NOT found THEN
            RAISE EXCEPTION 'Unable to find managed cluster by host %', i_fqdn
            USING ERRCODE = 'MDB01';
        END IF;
    ELSE
        v_rev := i_rev;
    END IF;

    SELECT c.cid,
           cr.name,
           c.type,
           c.env,
           c.created_at,
           c.public_key,
           cr.network_id,
           cr.folder_id,
           cr.description,
           cr.status,
           v_rev AS actual_rev,
           c.next_rev,
           cr.host_group_ids,
           c.deletion_protection
      INTO v_cluster
      FROM dbaas.clusters c
           JOIN dbaas.clusters_revs cr ON (c.cid = cr.cid)
           JOIN dbaas.subclusters_revs sc ON (c.cid = sc.cid)
           JOIN dbaas.hosts_revs h USING (subcid)
     WHERE cr.rev = v_rev
           AND h.rev = v_rev
           AND sc.rev = v_rev
           AND h.fqdn = i_fqdn
           AND code.visible(c)
           AND code.managed(c);

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find managed cluster by host %', i_fqdn
        USING ERRCODE = 'MDB01';
    END IF;

    v_config := ARRAY(SELECT value FROM code.get_rev_pillar_by_host(i_fqdn, v_rev, i_target_id) ORDER BY priority);

    v_custom_pillar := code.get_rev_custom_pillar_by_host(i_fqdn, (v_cluster).type, v_rev);

    SELECT subcid,
           shard_id,
           flavor,
           space_limit,
           fqdn,
           vtype_id,
           geo_id,
           disk_type_id,
           subnet_id,
           assign_public_ip,
           created_at
      INTO v_host
      FROM dbaas.hosts_revs
     WHERE rev = v_rev
       AND fqdn = i_fqdn;

    SELECT *
      INTO v_flavor
      FROM dbaas.flavors
     WHERE id = (v_host).flavor;

    SELECT *
      INTO v_disk_type
      FROM dbaas.disk_type
     WHERE disk_type_id = (v_host).disk_type_id;

    SELECT subcid,
           cid,
           name,
           roles,
           created_at
      INTO v_subcluster
      FROM dbaas.subclusters_revs
     WHERE rev = v_rev
       AND subcid = (v_host).subcid;

    SELECT subcid,
           shard_id,
           name,
           created_at
      INTO v_shard
      FROM dbaas.shards_revs
     WHERE rev = v_rev
       AND shard_id = (v_host).shard_id;

    SELECT *
      INTO v_folder
      FROM dbaas.folders
     WHERE folder_id = (v_cluster).folder_id;

    SELECT *
      INTO v_cloud
      FROM dbaas.clouds
     WHERE cloud_id = (v_folder).cloud_id;

    SELECT *
      INTO v_geo
      FROM dbaas.geo
     WHERE geo_id = (v_host).geo_id;

    SELECT coalesce(schedule, '{}'::jsonb)
      INTO v_backup_schedule
      FROM dbaas.backup_schedule
     WHERE cid = (v_cluster).cid;

    WITH cte AS (
        SELECT
            h.fqdn,
            fld.folder_ext_id folder_id,
            cld.cloud_ext_id cloud_id,
            f.name resource_preset_id,
            f.platform_id platform_id,
            f.cpu_limit::numeric cores,
            (f.cpu_guarantee * 100 / f.cpu_limit)::int core_fraction,
            f.gpu_limit gpu_limit,
            f.memory_limit memory,
            f.io_cores_limit io_cores_limit,
            cv.cid cluster_id,
            c.type cluster_type,
            dt.disk_type_ext_id disk_type_id,
            h.space_limit disk_size,
            h.assign_public_ip assign_public_ip,
            sc.roles roles,
            h.vtype_id compute_instance_id,
            coalesce(cardinality(cv.host_group_ids), 0) > 0 on_dedicated_host
        FROM
                dbaas.clusters c
            INNER JOIN
                dbaas.clusters_revs cv
                    ON cv.cid = c.cid AND cv.rev = v_rev
            INNER JOIN
                dbaas.subclusters_revs sc
                    ON c.cid = sc.cid AND sc.rev = v_rev
            INNER JOIN
                dbaas.hosts_revs h
                    ON h.subcid = sc.subcid AND h.rev = v_rev
            INNER JOIN
                dbaas.flavors f
                    ON f.id = h.flavor
            INNER JOIN
                dbaas.folders fld
                    ON fld.folder_id = cv.folder_id
            INNER JOIN
                dbaas.clouds cld
                    ON cld.cloud_id = fld.cloud_id
            INNER JOIN
                dbaas.disk_type dt
                    ON dt.disk_type_id = h.disk_type_id
            WHERE c.cid = (v_cluster).cid
    )
    SELECT jsonb_object_agg(r.fqdn, to_jsonb(r.*))
        INTO v_cluster_hosts
        FROM cte r;

    v_derived_config := jsonb_build_object(
        'data', jsonb_build_object(
            'backup', v_backup_schedule,
            'runlist', jsonb_build_array(),
            'dbaas', jsonb_build_object(
                'fqdn', i_fqdn,
                'cluster_id', (v_cluster).cid,
                'cluster_name', (v_cluster).name,
                'cluster_type', (v_cluster).type,
                'cluster', jsonb_build_object(
                    'subclusters', jsonb_build_object()
                ),
                'subcluster_id', (v_subcluster).subcid,
                'subcluster_name', (v_subcluster).name,
                'shard_id', (v_host).shard_id,
                'shard_name', (CASE WHEN (v_host).shard_id IS NOT NULL THEN (v_shard).name ELSE NULL END),
                'vtype', (v_flavor).vtype,
                'vtype_id', (v_host).vtype_id,
                'shard_hosts', jsonb_build_array(),
                'cluster_hosts', jsonb_build_array(),
                'folder', jsonb_build_object(
                    'folder_ext_id', (v_folder).folder_ext_id
                ),
                'cloud', jsonb_build_object(
                    'cloud_ext_id', (v_cloud).cloud_ext_id
                ),
                'flavor', jsonb_build_object(
                    'id', (v_flavor).id,
                    'cpu_guarantee', to_jsonb(1.0 * (v_flavor).cpu_guarantee::numeric),
                    'cpu_limit', to_jsonb(1.0 * (v_flavor).cpu_limit::numeric),
                    'cpu_fraction', ((v_flavor).cpu_guarantee * 100 / (v_flavor).cpu_limit)::int,
                    'gpu_limit', (v_flavor).gpu_limit,
                    'memory_guarantee', (v_flavor).memory_guarantee,
                    'memory_limit', (v_flavor).memory_limit,
                    'network_guarantee', (v_flavor).network_guarantee,
                    'network_limit', (v_flavor).network_limit,
                    'io_limit', code.dynamic_io_limit((v_host).space_limit, v_disk_type, (v_flavor).io_limit),
                    'io_cores_limit', (v_flavor).io_cores_limit,
                    'name', (v_flavor).name,
                    'description', (v_flavor).name,
                    'vtype', (v_flavor).vtype,
                    'type', (v_flavor).type,
                    'generation', (v_flavor).generation,
                    'platform_id', (v_flavor).platform_id
                ),
                'space_limit', (v_host).space_limit,
                'disk_type_id', (v_disk_type).disk_type_ext_id,
                'geo', (v_geo).name,
                'assign_public_ip', (v_host).assign_public_ip,
                'created_at', (v_host).created_at,
                'on_dedicated_host', coalesce(cardinality((v_cluster).host_group_ids), 0) > 0
            ),
            'versions', jsonb_build_object()
        ),
        'yandex', jsonb_build_object('environment', (v_cluster).env));

    IF (v_cluster).type::text = 'greenplum_cluster' THEN
        v_derived_config = jsonb_set(
                v_derived_config, '{data,dbaas,cluster_hosts_info}', v_cluster_hosts);
    END IF;

    FOREACH component IN ARRAY (v_subcluster).roles
    LOOP
        v_derived_config = jsonb_insert(v_derived_config, '{data,runlist,-1}',
                                        ('"components.' || component || '"')::jsonb, true);
    END LOOP;

    FOR v_version IN
        SELECT * FROM code.get_rev_version_by_host(i_fqdn, v_rev)
    LOOP
        v_derived_config = jsonb_set(
            v_derived_config, array_append('{data,versions}', (v_version).component),
            jsonb_build_object(
                'major_version', (v_version).major_version,
                'minor_version', (v_version).minor_version,
                'package_version', (v_version).package_version,
                'edition', (v_version).edition));
    END LOOP;

    FOR host IN
        SELECT
            sc.subcid,
            sc.name AS subcluster_name,
            sc.roles,
            s.name AS shard_name,
            s.shard_id,
            h.fqdn,
            g.name "geo"
        FROM
            dbaas.subclusters_revs sc
            JOIN dbaas.hosts_revs h ON (h.subcid = sc.subcid AND h.rev = sc.rev)
            LEFT JOIN dbaas.shards_revs s ON (s.shard_id = h.shard_id AND s.rev = h.rev)
            JOIN dbaas.geo g ON (h.geo_id = g.geo_id)
        WHERE
            sc.rev = v_rev
            AND sc.cid = (v_cluster).cid
        ORDER BY h.fqdn
    LOOP
        v_derived_config = jsonb_insert(v_derived_config, '{data,dbaas,cluster_hosts,-1}',
                                        ('"' || (host).fqdn || '"')::jsonb, true);

        IF ((v_host).shard_id IS NULL AND (host).shard_id IS NULL) OR (v_host).shard_id = (host).shard_id THEN
            v_derived_config = jsonb_insert(v_derived_config, '{data,dbaas,shard_hosts,-1}',
                                            ('"' || (host).fqdn || '"')::jsonb, true);
        END IF;

        IF v_derived_config->'data'->'dbaas'->'cluster'->'subclusters'->(host).subcid IS NULL THEN
            v_derived_config = jsonb_set(
                v_derived_config, array_append('{data,dbaas,cluster,subclusters}', (host).subcid),
                jsonb_build_object(
                    'name', (host).subcluster_name,
                    'roles', array_to_json((host).roles),
                    'shards', jsonb_build_object(),
                    'hosts', jsonb_build_object()));
        END IF;

        IF (host).shard_id IS NOT NULL THEN
            IF v_derived_config->'data'->'dbaas'->'cluster'->'subclusters'->(host).subcid->'shards'->(host).shard_id IS NULL THEN
                v_derived_config = jsonb_set(
                    v_derived_config,
                    array_append(
                        array_append(
                            array_append('{data,dbaas,cluster,subclusters}', (host).subcid),
                            'shards'),
                        (host).shard_id),
                    jsonb_build_object(
                        'name', (host).shard_name,
                        'hosts', jsonb_build_object()));
            END IF;

            v_derived_config = jsonb_set(
                v_derived_config,
                array_append(
                    array_append(
                        array_append(
                            array_append(
                                array_append('{data,dbaas,cluster,subclusters}', (host).subcid),
                                'shards'),
                            (host).shard_id),
                        'hosts'),
                    (host).fqdn),
                jsonb_build_object(
                    'geo', (host).geo));
        ELSE
            v_derived_config = jsonb_set(
                v_derived_config,
                array_append(
                    array_append(
                        array_append('{data,dbaas,cluster,subclusters}', (host).subcid),
                        'hosts'),
                    (host).fqdn),
                jsonb_build_object(
                    'geo', (host).geo));
        END IF;
    END LOOP;

    RETURN code.combine_dict(array_append(array_append(v_config, v_custom_pillar), v_derived_config));
END;
$$ LANGUAGE plpgsql STABLE;

--head/code/70_get_unmanaged_config.sql
CREATE OR REPLACE FUNCTION code.get_unmanaged_config(
    i_fqdn      text,
    i_target_id text   DEFAULT NULL,
    i_rev       bigint DEFAULT NULL
) RETURNS jsonb AS $$
DECLARE
    v_rev            bigint;
    v_config         jsonb[];
BEGIN
    IF i_rev IS NULL THEN
        SELECT c.actual_rev
          INTO v_rev
          FROM dbaas.clusters c
               JOIN dbaas.subclusters sc USING (cid)
               JOIN dbaas.hosts h USING (subcid)
         WHERE h.fqdn = i_fqdn
               AND code.visible(c)
               AND NOT code.managed(c);

        IF NOT found THEN
            RAISE EXCEPTION 'Unable to find unmanaged cluster by host %', i_fqdn
            USING ERRCODE = 'MDB01';
        END IF;
    ELSE
        v_rev := i_rev;
    END IF;

    SELECT array(
        SELECT value FROM code.get_rev_pillar_by_host(i_fqdn, v_rev, i_target_id) ORDER BY priority)
    INTO v_config;

    RETURN code.combine_dict(v_config);
END;
$$ LANGUAGE plpgsql STABLE;

--head/code/71_pillar_shortcuts.sql
CREATE OR REPLACE FUNCTION code.salt2json_path(
    i_path  text
) RETURNS text[] AS $$
    SELECT CAST(
        CASE
            WHEN i_path LIKE '{%' THEN i_path
            ELSE '{' || REPLACE(i_path, ':', ',') || '}'
        END
    AS text[])
$$ LANGUAGE sql IMMUTABLE;

CREATE OR REPLACE FUNCTION code.easy_get_pillar(
    i_cid   text,
    i_path  text DEFAULT NULL
) RETURNS jsonb AS $$
    SELECT
    CASE
        WHEN i_path IS NOT NULL THEN value #> code.salt2json_path(i_path)
        ELSE value
    END
      FROM dbaas.pillar
     WHERE cid = i_cid;
$$ LANGUAGE sql STABLE;

CREATE OR REPLACE FUNCTION code.easy_update_pillar(
    i_cid       text,
    i_path      text,
    i_val       jsonb,
    i_reason    text DEFAULT NULL,
    i_user_id   text DEFAULT 'admin.local'
) RETURNS void AS $$
DECLARE
    v_cluster record;
BEGIN
    v_cluster := code.lock_cluster(
        i_cid => i_cid,
        i_x_request_id => i_reason
    );
    PERFORM code.update_pillar(
        i_cid := i_cid,
        i_rev := v_cluster.rev,
        i_key := code.make_pillar_key(i_cid := i_cid),
        i_value := jsonb_set(
            v_cluster.pillar_value,
            code.salt2json_path(i_path),
            i_val
        )
    );
    PERFORM code.complete_cluster_change(i_cid, v_cluster.rev);
    PERFORM code.add_finished_operation_for_current_rev(
        i_operation_id => gen_random_uuid()::text,
        i_cid => i_cid,
        i_folder_id => v_cluster.folder_id,
        i_operation_type => (v_cluster.type::text || '_modify'),
        i_metadata => '{}'::jsonb,
        i_user_id => i_user_id,
        i_version => 2,
        i_hidden => true,
        i_rev => v_cluster.rev
    );
    RETURN;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code.easy_delete_pillar(
    i_cid       text,
    i_path      text,
    i_reason    text DEFAULT NULL,
    i_user_id   text DEFAULT 'admin.local'
) RETURNS void AS $$
DECLARE
    v_cluster record;
BEGIN
    v_cluster := code.lock_cluster(
        i_cid => i_cid,
        i_x_request_id => i_reason
    );
    PERFORM code.update_pillar(
        i_cid := i_cid,
        i_rev := v_cluster.rev,
        i_key := code.make_pillar_key(i_cid := i_cid),
        i_value := (v_cluster.pillar_value #- code.salt2json_path(i_path))
    );
    PERFORM code.complete_cluster_change(i_cid, v_cluster.rev);
    PERFORM code.add_finished_operation_for_current_rev(
        i_operation_id => gen_random_uuid()::text,
        i_cid => i_cid,
        i_folder_id => v_cluster.folder_id,
        i_operation_type => (v_cluster.type::text || '_modify'),
        i_metadata => '{}'::jsonb,
        i_user_id => i_user_id,
        i_version => 2,
        i_hidden => true,
        i_rev => v_cluster.rev
    );
    RETURN;
END;
$$ LANGUAGE plpgsql;

--head/code/72_set_maintenance_window_settings.sql
CREATE OR REPLACE FUNCTION code.set_maintenance_window_settings(
    i_cid   text,
    i_day   dbaas.maintenance_window_days,
    i_hour  int,
    i_rev   bigint
) RETURNS void AS $$
BEGIN
    DELETE FROM dbaas.maintenance_window_settings
        WHERE cid = i_cid;

    IF i_day IS NOT NULL AND i_hour IS NOT NULL THEN
        INSERT INTO dbaas.maintenance_window_settings (cid, day, hour)
            VALUES (i_cid, i_day, i_hour);
        PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                'set_maintenance_window_settings',
                jsonb_build_object(
                    'cid', i_cid,
                    'day', i_day,
                    'hour', i_hour
                )
            )
        );
    ELSIF i_day IS NULL AND i_hour IS NULL THEN
        PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                    'clear_maintenance_window_settings',
                    jsonb_build_object(
                            'cid', i_cid
                        )
                )
        );
    ELSE
        RAISE EXCEPTION 'unable to update maintenance settings if one of it is NULL';
    END IF;
END;
$$ LANGUAGE plpgsql;

--head/code/73_complete_future_cluster_change.sql
CREATE OR REPLACE FUNCTION code.complete_future_cluster_change(
    i_cid  text,
    i_actual_rev  bigint,
    i_next_rev bigint
) RETURNS void AS $$
BEGIN
    IF i_actual_rev = i_next_rev THEN
        RAISE EXCEPTION 'Unable to complete future cluster change: '
                        'actual and next rev are equals cid=%, rev=%', i_cid, i_actual_rev
            USING TABLE = 'dbaas.clusters';
    END IF;
    PERFORM code.complete_cluster_change(i_cid, i_next_rev);
    PERFORM code.reset_cluster_to_rev(i_cid, i_actual_rev);
END;
$$ LANGUAGE plpgsql;

--head/code/73_lock_future_cluster.sql
CREATE OR REPLACE FUNCTION code.lock_future_cluster(
    i_cid          text,
    i_x_request_id text DEFAULT NULL
) RETURNS SETOF code.cluster_with_labels AS $$
DECLARE
    v_cluster dbaas.clusters;
BEGIN
    UPDATE dbaas.clusters
    SET next_rev = next_rev + 1
    WHERE cid = i_cid
    RETURNING * INTO v_cluster;

    IF NOT found THEN
        RAISE EXCEPTION 'There is no cluster %', i_cid
            USING TABLE = 'dbaas.clusters';
    END IF;

    INSERT INTO dbaas.clusters_changes
    (cid, rev, x_request_id)
    VALUES
    (i_cid, (v_cluster).next_rev, i_x_request_id);

    RETURN QUERY
        SELECT cl.*
        FROM code.as_cluster_with_labels(v_cluster) cl;
END;
$$ LANGUAGE plpgsql;

--head/code/73_plan_maintenance_task.sql
CREATE OR REPLACE FUNCTION code.plan_maintenance_task(
    i_task_id               text,
    i_cid                   text,
    i_config_id             text,
    i_folder_id             bigint,
    i_operation_type        text,
    i_task_type             text,
    i_task_args             jsonb,
    i_version               integer,
    i_metadata              jsonb,
    i_user_id               text,
    i_rev                   bigint,
    i_target_rev            bigint,
    i_plan_ts               timestamp with time zone,
    i_info                  text,
    i_create_ts             timestamp with time zone DEFAULT now(),
    i_timeout               interval DEFAULT '1 hour',
    i_max_delay             timestamp with time zone DEFAULT now() + INTERVAL '21 0:00:00'
) RETURNS void AS $$
BEGIN
    INSERT INTO dbaas.maintenance_tasks
        (max_delay, cid, config_id, task_id, plan_ts, create_ts, info)
    VALUES
        (i_max_delay, i_cid, i_config_id, i_task_id, i_plan_ts, i_create_ts, i_info)
    ON CONFLICT (cid, config_id)
        DO UPDATE
        SET task_id = i_task_id,
            max_delay = i_max_delay,
            plan_ts = i_plan_ts,
            create_ts = i_create_ts,
            info = i_info,
            status = 'PLANNED'::dbaas.maintenance_task_status;

    INSERT INTO dbaas.worker_queue (
        task_id,
        cid,
        folder_id,
        task_type,
        task_args,
        created_by,
        operation_type,
        metadata,
        hidden,
        version,
        delayed_until,
        timeout,
        create_rev,
        unmanaged,
        target_rev,
        config_id
    )
    VALUES (
       i_task_id,
       i_cid,
       i_folder_id,
       i_task_type,
       i_task_args,
       i_user_id,
       i_operation_type,
       i_metadata,
       false,
       i_version,
       i_plan_ts,
       i_timeout,
       i_rev,
       false,
       i_target_rev,
       i_config_id
    );
END;
$$ LANGUAGE plpgsql;

--head/code/74_add_backup_schedule.sql
CREATE OR REPLACE FUNCTION code.add_backup_schedule(
    i_cid      text,
    i_rev      bigint,
    i_schedule jsonb
) RETURNS void AS $$
BEGIN
    INSERT INTO dbaas.backup_schedule (
        cid,
        schedule
    )
    VALUES (
        i_cid,
        i_schedule
    )
    ON CONFLICT (cid) DO UPDATE
    SET schedule = EXCLUDED.schedule;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_backup_schedule',
            jsonb_build_object(
                'cid', i_cid
            )
        )
    );
END;
$$ LANGUAGE plpgsql;

--head/code/74_plan_managed_backup.sql
CREATE OR REPLACE FUNCTION code.plan_managed_backup(
	i_backup_id      text,
	i_cid            text,
	i_subcid         text,
	i_shard_id       text,
	i_status         dbaas.backup_status,
	i_method         dbaas.backup_method,
	i_initiator      dbaas.backup_initiator,
	i_delayed_until  timestamptz,
	i_scheduled_date timestamp,
	i_parent_ids     text[],
	i_child_id       text

) RETURNS dbaas.backups AS $$
DECLARE
	v_backup       dbaas.backups;
BEGIN
	INSERT INTO dbaas.backups (backup_id, cid, subcid, shard_id, status, method, initiator, delayed_until, scheduled_date)
	VALUES (i_backup_id, i_cid, i_subcid, i_shard_id, i_status, i_method, i_initiator, i_delayed_until, i_scheduled_date) RETURNING * INTO v_backup;

	IF i_parent_ids <> '{}' THEN
		INSERT INTO dbaas.backups_dependencies
			(parent_id, child_id)
		SELECT parent_id, i_child_id FROM unnest(i_parent_ids) parent_id;
	END IF;

	return v_backup;
END;
$$ LANGUAGE plpgsql;

--head/code/74_reschedule_maintenance_task.sql
CREATE OR REPLACE FUNCTION code.reschedule_maintenance_task(
    i_cid                   text,
    i_config_id             text,
    i_plan_ts               timestamp with time zone
) RETURNS void AS $$
BEGIN
    WITH maintenance_task AS (
        UPDATE dbaas.maintenance_tasks
        SET plan_ts=i_plan_ts
        WHERE cid=i_cid AND config_id=i_config_id AND status='PLANNED'::dbaas.maintenance_task_status
        RETURNING task_id
    )
    UPDATE dbaas.worker_queue wq
    SET delayed_until=i_plan_ts
    FROM maintenance_task mt
    WHERE wq.task_id=mt.task_id;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Planned maintenance task not found % on cluster %', i_config_id, i_cid
            USING TABLE = 'dbaas.maintenance_tasks';
    END IF;
END;
$$ LANGUAGE plpgsql;

--head/code/74_set_backup_service_use.sql
CREATE OR REPLACE FUNCTION code.set_backup_service_use(
    i_cid       text,
    i_val       bool,
    i_reason    text DEFAULT NULL
) RETURNS void AS $$
DECLARE
    v_cluster record;
BEGIN
    v_cluster := code.lock_cluster(
        i_cid => i_cid,
        i_x_request_id => i_reason
    );
    UPDATE dbaas.backup_schedule
       SET schedule = jsonb_set(
        v_cluster.backup_schedule,
        '{use_backup_service}',
        CAST(i_val as TEXT)::jsonb
    )
    WHERE cid = i_cid;
    PERFORM code.update_cluster_change(
        i_cid,
        v_cluster.rev,
        jsonb_build_object(
            'update_backup_schedule',
            jsonb_build_object(
                'cid', i_cid
            )
        )
    );
    PERFORM code.complete_cluster_change(i_cid, v_cluster.rev);
    PERFORM code.add_finished_operation_for_current_rev(
        i_operation_id => gen_random_uuid()::text,
        i_cid => i_cid,
        i_folder_id => v_cluster.folder_id,
        i_operation_type => (v_cluster.type::text || '_modify'),
        i_metadata => '{}'::jsonb,
        i_user_id => 'backup_cli',
        i_version => 2,
        i_hidden => true,
        i_rev => v_cluster.rev
    );
    RETURN;
END;
$$ LANGUAGE plpgsql;

--head/code/75_tmp_greenplum_admin_functionality.sql
CREATE OR REPLACE FUNCTION code.random_string(
    i_len integer DEFAULT 17,
    i_prefix text DEFAULt 'mdb'
) RETURNS SETOF TEXT AS
$$
    SELECT (i_prefix || array_to_string(array(SELECT substr('abcdefghijklmnopqrstuvwxyz0123456789', trunc(random() * 36)::integer + 1, 1) FROM generate_series(1, i_len-3)), ''));
$$ LANGUAGE SQL;

CREATE OR REPLACE FUNCTION code.create_greenplum_test_cluster(
    i_name            text,
    i_fqdns_master    text[],         -- {"master.db.yandex.net", "standby.db.yandex.net"}
    i_fqdns_segments  text[],         -- {"seg1.db.yandex.net", "seg2.db.yandex.net"}
    i_env             dbaas.env_type  DEFAULT 'dev'::dbaas.env_type,
    i_network_id      text            DEFAULT '', -- omit this in porto
    i_folder          text            DEFAULT 'mdb-junk',
    i_geo             text            DEFAULT 'iva',
    i_flavor          text            DEFAULT 'db1.nano',
    i_disk_type       text            DEFAULT 'local-ssd',
    i_space_limit     bigint          DEFAULT 21474836480,
    i_request_id      text            DEFAULT 'greenplumhandmade',
    i_subnet_id       text            DEFAULT '', -- omit this in porto
    i_id_prefix      text            DEFAULT 'mdb'
)
RETURNS text
AS $$
DECLARE
    v_pillar              jsonb;
    v_folder              dbaas.folders;
    v_flavor              dbaas.flavors;
    v_cluster             code.cluster_with_labels;
    v_cid                 text = '';
    v_subcid              text = '';
    v_subcid_segs         text = '';
    v_prev_operation      code.operation;
BEGIN

    SELECT * INTO v_flavor FROM dbaas.flavors WHERE name = i_flavor;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'flavor % not found', i_flavor;
    END IF;
    
    SELECT * INTO v_folder FROM dbaas.folders WHERE folder_ext_id = i_folder;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'folder % not found', i_folder;
    END IF;

    v_cid = code.random_string(
        i_prefix => i_id_prefix
    );

    v_subcid = code.random_string(
        i_prefix => i_id_prefix
    );
    v_subcid_segs = code.random_string(
        i_prefix => i_id_prefix
    );
    
    v_cluster = code.create_cluster(
        i_cid          => v_cid,
        i_name         => i_name,
        i_type         => 'greenplum_cluster'::dbaas.cluster_type,
        i_env          => i_env,
        i_public_key   => '',
        i_network_id   => i_network_id,
        i_folder_id    => v_folder.folder_id,
        i_description  => 'greenplum test cluster',
        i_x_request_id => i_request_id
    );

    PERFORM code.add_subcluster(
        i_cid    => v_cid,
        i_subcid => v_subcid,
        i_name   => 'master_subcluster',
        i_roles  => '{greenplum_cluster.master_subcluster}'::dbaas.role_type[],
        i_rev    => v_cluster.rev
    );
    
    PERFORM code.add_subcluster(
        i_cid    => v_cid,
        i_subcid => v_subcid_segs,
        i_name   => 'segment_subcluster',
        i_roles  => '{greenplum_cluster.segment_subcluster}'::dbaas.role_type[],
        i_rev    => v_cluster.rev
    );

    
    v_pillar = ('{"data":{
    "deploy": {
        "version": 2
    },
    "token_service": {
        "address": "ts.private-api.cloud.yandex.net:4282"
    },
    "solomon_cloud": {
        "project": "yandexcloud",
        "service": "mdb_greenplum",
        "ca_path": "/opt/yandex/allCAs.pem",
        "push_url": "https://solomon.cloud.yandex-team.ru/api/v2/push",
        "sa_id": "saidtest",
        "sa_key_id": "sakeyidtest",
        "sa_private_key": "saprivatekey"
    },
    "gp_pkg_version": "6.14.0-11-yandex.51956.62d24f4a45",
    "gp_init": {
        "database_name": "adb",
        "segments_per_disk": 1,
        "mirror_mode": "group"
    },
    "gp_data_folders": [
        "/var/lib/greenplum/data1",
        "/var/lib/greenplum/data2"
    ],
    "gp_admin_prv_key": "-----BEGIN OPENSSH PRIVATE KEY-----\nb3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAACFwAAAAdzc2gtcn\nNhAAAAAwEAAQAAAgEAxyuXXAsptDD258v8MmfFEmbK3x3/6ZxbR4Ja3DNgkzZMqLigIiul\nTQ8Ezng9ANg6/Kn++u5XF8ZsxyH4bna6ZlPp8OrM59dAQJwLojnoRXX6x8TaQdLsTL4jJZ\nFtB3a8kHsHvuQW+Yr3DRardGrbJHDwosHHRUSPW5p9zVCSgrBX39EEx4Ksh3G/iBeBLlFd\nUsMh2rpef/lWenvwaW0vWf95XFLSzE4SoAuAsJMaSdWdjMY+pYD2Uw4ZgNLGbw2RfskKaW\nVLdjHwHM5XVD4UPaHz79HFKEqfOCch2KwBTgI13XHr7nKbK5X/bf/Y1umeNijScgSrVa7D\nvqWFsSawaawAsZzGt2P6umLcGDqxX2q2zgkiQFDBANGkzYunwwxaNWuExLrRSzmtH8EHgZ\n32lp0LeZ/iVO0x5IqLJCwO8Hq6jx2m0cDjYS0pfAOUnNzJ5XXDLHejtw4I7Jrz42IRkOl+\n03XPWNWM+tW7qcM5zYYY5KgsVXFqdQSvFpPtCrxXXdwphKVukDy6RmFkkSo5Z5I1DiWFUe\nUZ7G48MhctUBWrckB/uNYJjFTeiDYCn+l3/fir9TOYICyV9H8xqh43Q0bEn7LpBWwXaNGV\nF+d6SI4hNBfOlq9QUMfw+37Puqs0sFcVEiUN9ImSFO9552j65nZzfRNO7Uj2BUABxZ8mUO\n0AAAdQO8WOxzvFjscAAAAHc3NoLXJzYQAAAgEAxyuXXAsptDD258v8MmfFEmbK3x3/6Zxb\nR4Ja3DNgkzZMqLigIiulTQ8Ezng9ANg6/Kn++u5XF8ZsxyH4bna6ZlPp8OrM59dAQJwLoj\nnoRXX6x8TaQdLsTL4jJZFtB3a8kHsHvuQW+Yr3DRardGrbJHDwosHHRUSPW5p9zVCSgrBX\n39EEx4Ksh3G/iBeBLlFdUsMh2rpef/lWenvwaW0vWf95XFLSzE4SoAuAsJMaSdWdjMY+pY\nD2Uw4ZgNLGbw2RfskKaWVLdjHwHM5XVD4UPaHz79HFKEqfOCch2KwBTgI13XHr7nKbK5X/\nbf/Y1umeNijScgSrVa7DvqWFsSawaawAsZzGt2P6umLcGDqxX2q2zgkiQFDBANGkzYunww\nxaNWuExLrRSzmtH8EHgZ32lp0LeZ/iVO0x5IqLJCwO8Hq6jx2m0cDjYS0pfAOUnNzJ5XXD\nLHejtw4I7Jrz42IRkOl+03XPWNWM+tW7qcM5zYYY5KgsVXFqdQSvFpPtCrxXXdwphKVukD\ny6RmFkkSo5Z5I1DiWFUeUZ7G48MhctUBWrckB/uNYJjFTeiDYCn+l3/fir9TOYICyV9H8x\nqh43Q0bEn7LpBWwXaNGVF+d6SI4hNBfOlq9QUMfw+37Puqs0sFcVEiUN9ImSFO9552j65n\nZzfRNO7Uj2BUABxZ8mUO0AAAADAQABAAACACC0BqllZ9afh5suAl4gbdqEqGEUYvXv54kJ\nXXP0t7HUY6f8kMarlfveMHLaiWG/H4hnPWfkhMZxnWDhMhKpShgNRUd6tmSHEpTJSpu7mG\nj3Y1Mz/oZ6ZLSBL/I2O8nS9Elg+jec6izVZZVvmH2IIi2MoeaHnPnBtSxcZLW2uifdXsBw\naLF9wmiHA+ULvvllAMbbJY7ttSCcR1fbS/FzrSfA7CN9sgE7/JDs8peLv/BJtBHuZ1DzqP\n6gPQ3LDiwj9TT1O9FsgYSJ1JxWQT6i5t3r3ssNDat8/UHSIxuZuqkdcczHrO69QL9aZNOi\nA+/d8k2ATHXOUHfEN33xXc9lw+d7wtVUjomUW2guga5du6cEgoGO6To1nRgj0gPkYi0kYY\nOmlGc4nIdmuTW5ah0Ahql1HQ5EyOHuJhl8zvgUxuc/4s4h6PTEPMZCkUA9ptfxk9PCXBtw\nJBGtvdGWFJzNVWMVVVoZ67LeobjYMqbJpqwiZZg4JJG5rvrk1DyLfvqzireSsJ6fIe0ZdS\nRJexOUwnovvcsWvcxv/63aeo2zcn6M9B72Jd360Bp8EyOdmbsmnPRdv3I7rcKPmo2HiWS7\nAgssXv5EKW/ccVKdyjnT5DfDN1LqqNK9kaIh+C18P8Mw7Kfw7hcRIbrX3gJQaE7sx081ut\nbmkFuyxmTOMq4+2ah9AAABAQC8IuVZT71l5omMLNOHJmefkP3Rz0OlE9LLsqzsESaN1NmZ\nE79NpKEQi9EM3ApHHKZom/kML0x6tWoP8ebax0vSuTwx/g88e1vsdqVbuijByZYuDW2eUg\nlmIExf23R/IHzzb+NizwH4Z3Q7HpCmJMU36PgPf8d1wGs7XGHoT3mCImYLLclSmsQlDqrC\nOsPAl4Iha+6FdzYYvo9pKHCvz9+bleEvCM2CQnDtNQlOfWYs8dAb4dJs5SyyCaxB3rNzAs\nVd+6BH6D9/Ow6jg+KZOudrQVjL0RioUIXJMjXQqawLthbYSSozdfTZI6veOnd0B6K2ncWX\naDr85xef7yxJTAI2AAABAQDuuhYuldOCIOC+2ujow1q5qG4EAPGiSUAosDaHDPxmnMM5BA\n0PVm0B4YPdk3h3TLWTkobvDMDS0aYge2MY3TtNPesmPe2g0DwpOO+80T48fJgFsFx9eVVp\nMBoQmeqATt6GaORRqhlF1xFmg+ME17ggKZuLC6BQjjcX2zcuB+zWI+smeArmnh/FJqot9v\npnpLIyR9uVmI4X+8obMaoF/ISi7YQFAdS/1CBqRPJtDhWmGTtADb9S3rjNMby9O3IM06tc\nmFOk5Gtn8G77e9dJ0JqXfZw3mEdHa0gwwfjmH6GBbW0Vnyc1muS7G4JcJNMxcsnYB2sIfI\ntLM3HCVuteOECPAAABAQDVlM0qTG9vQboApQbkQlvB+12VZTlMZlgtlC1uIji0kbI8lpvP\n1h/rOdDIH3QqT7eBdH2BxlwmLMWQtXOTEE51i1ePnRHF4CC5oe2H2XrUpQG8G7brvKsD/l\nrJMumWIP0qGWWnkGbUhRWQ6nv3OreF0bloMGNVayf4ymtVurgROfa1ZziFKLF5l82e8s8r\nTSaPe4LQzUNdrzTmer4qKkCsObW/3Hm1DMgk/RSjK0kwvYn3Ww2HFcwEj152EyQNItVy1y\nOPp5VtfP9smHDVqZe1txLkbdlyl1Hrak6L6C8Ihi1zUqldq+tk6YUHgYiqPVdg1GWO2RXQ\nUdjfAL8MqpzDAAAAFWthc2hpbmF2QGthc2hpbmF2LW9zeAECAwQF\n-----END OPENSSH PRIVATE KEY-----\n",
    "gp_admin_pub_key": "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAACAQDHK5dcCym0MPbny/wyZ8USZsrfHf/pnFtHglrcM2CTNkyouKAiK6VNDwTOeD0A2Dr8qf767lcXxmzHIfhudrpmU+nw6szn10BAnAuiOehFdfrHxNpB0uxMviMlkW0HdryQewe+5Bb5ivcNFqt0atskcPCiwcdFRI9bmn3NUJKCsFff0QTHgqyHcb+IF4EuUV1SwyHaul5/+VZ6e/BpbS9Z/3lcUtLMThKgC4CwkxpJ1Z2Mxj6lgPZTDhmA0sZvDZF+yQppZUt2MfAczldUPhQ9ofPv0cUoSp84JyHYrAFOAjXdcevucpsrlf9t/9jW6Z42KNJyBKtVrsO+pYWxJrBprACxnMa3Y/q6YtwYOrFfarbOCSJAUMEA0aTNi6fDDFo1a4TEutFLOa0fwQeBnfaWnQt5n+JU7THkioskLA7werqPHabRwONhLSl8A5Sc3MnldcMsd6O3DgjsmvPjYhGQ6X7Tdc9Y1Yz61bupwznNhhjkqCxVcWp1BK8Wk+0KvFdd3CmEpW6QPLpGYWSRKjlnkjUOJYVR5RnsbjwyFy1QFatyQH+41gmMVN6INgKf6Xf9+Kv1M5ggLJX0fzGqHjdDRsSfsukFbBdo0ZUX53pIjiE0F86Wr1BQx/D7fs+6qzSwVxUSJQ30iZIU73nnaPrmdnN9E07tSPYFQAHFnyZQ7Q==", 
    "gp_master_directory": "/var/lib/greenplum/data1",
    "standby_install": true,
    "pxf_install": true,
    "pxf_pkg_version": "5.16.1-6-yandex.1054.2bb996ca",
    "greenplum":{
        "config": {
            "optimizer": true,
            "max_connections": {
                "segment": 750,
                "master": 250
            },
            "gp_workfile_limit_per_segment": 0,
            "gp_resource_manager": "queue"
        },
        "users": {
            "gpadmin": {
                "password": "gparray",
                "create": false
            },
            "user1": {
                "password": "user1",
                "login": true,
                "createrole": true,
                "createdb": true,
                "resource_group": "default_group",
                "resource_queue": "pg_default",
                "create": true
            }
        }
    }}}')::jsonb;

    PERFORM code.add_pillar(
        i_cid    => v_cid,
        i_rev    => v_cluster.rev,
        i_value  => v_pillar,
        i_key    => code.make_pillar_key(i_cid => v_cid)
    );

    v_prev_operation = code.add_operation(
        i_operation_id          => code.random_string(17, 'greenplum'),
        i_cid                   => v_cid,
        i_folder_id             => v_folder.folder_id,
        i_time_limit            => '1 hour',
        i_task_type             => 'greenplum_cluster_create',
        i_operation_type        => 'greenplum_cluster_create',
        i_metadata              => '{}'::jsonb,
        i_task_args             => '{}'::jsonb,
        i_user_id               => NULL,
        i_hidden                => false,
        i_version               => 2,
        i_delay_by              => NULL,
        i_required_operation_id => NULL,
        i_idempotence_data      => NULL,
        i_rev                   => v_cluster.rev
    );
    
    SELECT subcid INTO v_subcid FROM dbaas.subclusters WHERE cid = v_cid AND roles = '{greenplum_cluster.master_subcluster}'::dbaas.role_type[]; 
    SELECT subcid INTO v_subcid_segs FROM dbaas.subclusters WHERE cid = v_cid AND roles = '{greenplum_cluster.segment_subcluster}'::dbaas.role_type[];
    
    PERFORM code.update_cloud_usage(
        i_cloud_id   => v_folder.cloud_id,
        i_delta      => code.make_quota(
            i_cpu       => 0,
            i_gpu       => 0,
            i_memory    => 0,
            i_network   => 0,
            i_io        => 0,
            i_ssd_space => 0,
            i_hdd_space => 0,
            i_clusters  => 1
        ),
        i_x_request_id => i_request_id
    );

    FOR i IN 1 .. array_upper(i_fqdns_master, 1)
    LOOP

        PERFORM code.add_host(
            i_subcid           => v_subcid,
            i_shard_id         => NULL::text,
            i_space_limit      => i_space_limit,
            i_flavor_id        => v_flavor.id,
            i_geo              => i_geo,
            i_fqdn             => i_fqdns_master[i], --'greenplum-test02i.db.yandex.net',
            i_disk_type        => i_disk_type,
            i_subnet_id        => i_subnet_id,
            i_assign_public_ip => false,
            i_cid              => v_cid,
            i_rev              => v_cluster.rev
        );
        
        PERFORM code.update_cloud_usage(
            i_cloud_id   => v_folder.cloud_id,
            i_delta      => code.make_quota(
                i_cpu       => v_flavor.cpu_guarantee::real,
                i_gpu       => 0::bigint,
                i_memory    => v_flavor.memory_guarantee::bigint,
                i_network   => v_flavor.network_guarantee::bigint,
                i_io        => v_flavor.io_limit::bigint,
                i_ssd_space => i_space_limit,
                i_hdd_space => 0,
                i_clusters  => 0
            ),
            i_x_request_id => i_request_id
        );
        
        v_prev_operation = code.add_operation(
            i_operation_id          => code.random_string(17, 'greenplum'),
            i_cid                   => v_cid,
            i_folder_id             => v_folder.folder_id,
            i_time_limit            => '1 hour',
            i_task_type             => 'greenplum_cluster_create',
            i_operation_type        => 'greenplum_host_create',
            i_metadata              => '{}'::jsonb,
            i_task_args             => '{}'::jsonb,
            i_user_id               => NULL,
            i_hidden                => false,
            i_version               => 2,
            i_delay_by              => NULL,
            i_required_operation_id => v_prev_operation.operation_id,
            i_idempotence_data      => NULL,
            i_rev                   => v_cluster.rev
        );

    END LOOP;
    
    FOR i IN 1 .. array_upper(i_fqdns_segments, 1)
    LOOP

        PERFORM code.add_host(
            i_subcid           => v_subcid_segs,
            i_shard_id         => NULL::text,
            i_space_limit      => i_space_limit,
            i_flavor_id        => v_flavor.id,
            i_geo              => i_geo,
            i_fqdn             => i_fqdns_segments[i], --'greenplum-test06i.db.yandex.net',
            i_disk_type        => i_disk_type,
            i_subnet_id        => i_subnet_id,
            i_assign_public_ip => false,
            i_cid              => v_cid,
            i_rev              => v_cluster.rev
        );
        
        PERFORM code.update_cloud_usage(
            i_cloud_id   => v_folder.cloud_id,
            i_delta      => code.make_quota(
                i_cpu       => v_flavor.cpu_guarantee::real,
                i_gpu       => 0::bigint,
                i_memory    => v_flavor.memory_guarantee::bigint,
                i_network   => v_flavor.network_guarantee::bigint,
                i_io        => v_flavor.io_limit::bigint,
                i_ssd_space => i_space_limit,
                i_hdd_space => 0,
                i_clusters  => 0
            ),
            i_x_request_id => i_request_id
        );
        
        v_prev_operation = code.add_operation(
            i_operation_id          => code.random_string(17, 'greenplum'),
            i_cid                   => v_cid,
            i_folder_id             => v_folder.folder_id,
            i_time_limit            => '1 hour',
            i_task_type             => 'greenplum_cluster_create',
            i_operation_type        => 'greenplum_host_create',
            i_metadata              => '{}'::jsonb,
            i_task_args             => '{}'::jsonb,
            i_user_id               => NULL,
            i_hidden                => false,
            i_version               => 2,
            i_delay_by              => NULL,
            i_required_operation_id => v_prev_operation.operation_id,
            i_idempotence_data      => NULL,
            i_rev                   => v_cluster.rev
        );

    END LOOP;

    PERFORM code.complete_cluster_change(v_cid, v_cluster.rev);
   
    RETURN v_cid;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION code.insert_sample_cluster(
    i_folder          text            DEFAULT 'mdb-junk',
    i_flavor          text            DEFAULT 's1.compute.1',
    i_disk_type       text            DEFAULT 'local-ssd'
)
RETURNS void
AS $$
DECLARE
BEGIN

    PERFORM code.create_greenplum_test_cluster(
        i_name           => 'greenplum_test_cluster_13337'::text,
        i_fqdns_master   => '{"greenplum-test02i.db.yandex.net"}'::text[],
        i_fqdns_segments => '{"greenplum-test04i.db.yandex.net"}'::text[],
        i_folder         => i_folder,
        i_disk_type      => i_disk_type,
        i_flavor         => i_flavor
    ); --create sample cluster
    
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code.delete_greenplum_test_cluster(
    i_cid           text,
    i_request_id    text            DEFAULT 'greenplumhandmade'
)
RETURNS void
AS $$
DECLARE
    v_cluster     code.cluster_with_labels;
    v_del_op      code.operation;
    v_del_meta_op code.operation;
BEGIN
    v_cluster = code.lock_cluster(i_cid => i_cid, i_x_request_id => i_request_id);
    IF v_cluster IS NULL THEN
        RAISE EXCEPTION 'cluster % not found', i_cid;
    END IF;
    IF v_cluster.type::text != 'greenplum_cluster' THEN
        RAISE EXCEPTION 'cluster % is not greenplum', i_cid;
    END IF;
    
    v_del_op = code.add_operation(
        i_operation_id          => code.random_string(17, 'greenplum'),
        i_cid                   => v_cluster.cid,
        i_folder_id             => v_cluster.folder_id,
        i_time_limit            => '1 hour',
        i_task_type             => 'greenplum_cluster_delete',
        i_operation_type        => 'greenplum_cluster_delete',
        i_metadata              => '{}'::jsonb,
        i_task_args             => ('{}')::jsonb,
        i_user_id               => NULL,
        i_hidden                => false,
        i_version               => 2,
        i_delay_by              => NULL,
        i_required_operation_id => NULL,
        i_idempotence_data      => NULL,
        i_rev                   => v_cluster.rev
    );
    
    v_del_meta_op = code.add_operation(
        i_operation_id          => code.random_string(17, 'greenplum'),
        i_cid                   => v_cluster.cid,
        i_folder_id             => v_cluster.folder_id,
        i_time_limit            => '1 hour',
        i_task_type             => 'greenplum_cluster_delete_metadata',
        i_operation_type        => 'greenplum_cluster_delete',
        i_metadata              => '{}'::jsonb,
        i_task_args             => ('{}')::jsonb,
        i_user_id               => NULL,
        i_hidden                => false,
        i_version               => 2,
        i_delay_by              => '10 minutes'::interval,
        i_required_operation_id => v_del_op.operation_id,
        i_idempotence_data      => NULL,
        i_rev                   => v_cluster.rev
    );

    PERFORM code.add_operation(
        i_operation_id          => code.random_string(17, 'greenplum'),
        i_cid                   => v_cluster.cid,
        i_folder_id             => v_cluster.folder_id,
        i_time_limit            => '1 hour',
        i_task_type             => 'greenplum_cluster_purge',
        i_operation_type        => 'greenplum_cluster_delete',
        i_metadata              => '{}'::jsonb,
        i_task_args             => ('{}')::jsonb,
        i_user_id               => NULL,
        i_hidden                => false,
        i_version               => 2,
        i_delay_by              => '10 minutes'::interval,
        i_required_operation_id => v_del_meta_op.operation_id,
        i_idempotence_data      => NULL,
        i_rev                   => v_cluster.rev
    );
    PERFORM code.complete_cluster_change(v_cluster.cid, v_cluster.rev);
END;
$$ LANGUAGE plpgsql;


--head/code/76_get_rev_version_by_host.sql
CREATE OR REPLACE FUNCTION code.get_rev_version_by_host(
    i_fqdn      text,
    i_rev       bigint
) RETURNS SETOF code.version AS $$
SELECT component, major_version, minor_version, package_version, edition
  FROM (
    SELECT v.*, MAX(priority) OVER (PARTITION BY component) AS max_priority
      FROM dbaas.hosts_revs
      JOIN dbaas.subclusters_revs USING (subcid)
      JOIN dbaas.clusters USING (cid),
   LATERAL (
        SELECT component, major_version, minor_version, package_version, 'cid'::code.version_priority AS priority, edition
          FROM dbaas.versions_revs
         WHERE cid = subclusters_revs.cid
           AND rev = i_rev

        UNION ALL

        SELECT component, major_version, minor_version, package_version, 'subcid'::code.version_priority AS priority, edition
          FROM dbaas.versions_revs
         WHERE subcid = hosts_revs.subcid
           AND rev = i_rev

        UNION ALL

        SELECT component, major_version, minor_version, package_version, 'shard_id'::code.version_priority AS priority, edition
          FROM dbaas.versions_revs
         WHERE shard_id = hosts_revs.shard_id
           AND rev = i_rev) v
    WHERE fqdn = i_fqdn
      AND code.visible(clusters)
      AND hosts_revs.rev = i_rev
      AND subclusters_revs.rev = i_rev
  ) vp
WHERE priority = max_priority
$$ LANGUAGE SQL STABLE;

--head/code/76_set_default_versions.sql
-- fill entry in dbaas.versions with default_versions for given major version
CREATE OR REPLACE FUNCTION code.set_default_versions(
    i_cid           text,
    i_subcid        text,
    i_shard_id      text,
    i_ctype         dbaas.cluster_type,
    i_env           dbaas.env_type,
    i_major_version text,
    i_edition       text,
    i_rev           bigint
) RETURNS void AS $$
BEGIN

    INSERT INTO dbaas.versions (
                            cid,
                            subcid,
                            shard_id,
                            component,
                            major_version,
                            minor_version,
                            edition,
                            package_version)
    SELECT i_cid AS cid, i_subcid AS subcid, i_shard_id AS shard_id, component, major_version, minor_version, edition, package_version
        FROM dbaas.default_versions
        WHERE type=i_ctype AND major_version=i_major_version AND env=i_env AND edition=i_edition
    ON CONFLICT (cid, component) WHERE cid IS NOT NULL DO
        UPDATE SET component=EXCLUDED.component, major_version=EXCLUDED.major_version, minor_version=EXCLUDED.minor_version,
                   edition=EXCLUDED.edition, package_version=EXCLUDED.package_version;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'set_deafult_versions',
            jsonb_build_object(
                'cid', i_cid,
                'subcid', i_subcid,
                'shard_id', i_shard_id
            )
        )
    );

END;
$$ LANGUAGE plpgsql;

--temporary wrapper to support old api version
CREATE OR REPLACE FUNCTION code.set_default_versions(
    i_cid           text,
    i_subcid        text,
    i_shard_id      text,
    i_ctype         dbaas.cluster_type,
    i_env           dbaas.env_type,
    i_major_version text,
    i_rev           bigint
) RETURNS void AS $$
DECLARE
    v_major_version_1c text;
BEGIN
    v_major_version_1c := substring(i_major_version from '-1c$');
    IF v_major_version_1c IS NOT NULL THEN
        PERFORM code.set_default_versions(
            i_cid,
            i_subcid,
            i_shard_id,
            i_ctype,
            i_env,
            regexp_replace(i_major_version, '-1c$', '') ,
            '1c',
            i_rev
        );
    ELSE
        PERFORM code.set_default_versions(
            i_cid,
            i_subcid,
            i_shard_id,
            i_ctype,
            i_env,
            i_major_version,
            'default',
            i_rev
        );
    END IF;
END;
$$ LANGUAGE plpgsql;

--head/code/77_get_version_by_host.sql
CREATE OR REPLACE FUNCTION code.get_version_by_host(
    i_fqdn      text
) RETURNS SETOF code.version AS $$
SELECT component, major_version, minor_version, package_version, edition
  FROM (
    SELECT v.*, MAX(priority) OVER (PARTITION BY component) AS max_priority
      FROM dbaas.hosts
      JOIN dbaas.subclusters USING (subcid)
      JOIN dbaas.clusters USING (cid),
   LATERAL (
        SELECT component, major_version, minor_version, package_version, 'cid'::code.version_priority AS priority, edition
          FROM dbaas.versions
         WHERE cid = subclusters.cid

        UNION ALL

        SELECT component, major_version, minor_version, package_version, 'subcid'::code.version_priority AS priority, edition
          FROM dbaas.versions
         WHERE subcid = hosts.subcid

        UNION ALL

        SELECT component, major_version, minor_version, package_version, 'shard_id'::code.version_priority AS priority, edition
          FROM dbaas.versions
         WHERE shard_id = hosts.shard_id) v
     WHERE fqdn = i_fqdn
       AND code.visible(clusters)
  ) vp
WHERE priority = max_priority
$$ LANGUAGE SQL STABLE;

--head/code/78_create_disk_placement_group.sql
CREATE OR REPLACE FUNCTION code.create_disk_placement_group(
    i_cid                 text,
    i_local_id            bigint,
    i_rev                 bigint
) RETURNS bigint AS $$
DECLARE v_pg_id bigint;
BEGIN
    INSERT INTO dbaas.disk_placement_groups as pg (
        cid, local_id, status
    )
    VALUES (
        i_cid, i_local_id, 'DESCRIBED'::dbaas.disk_placement_group_status
    )
    RETURNING pg.pg_id INTO v_pg_id;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'create_placement_group',
             jsonb_build_object(
                'cid', i_cid,
                'local_id', i_local_id
            )
        )
    );

    RETURN v_pg_id;    
END;
$$ LANGUAGE plpgsql;

--head/code/79_create_disk.sql
CREATE OR REPLACE FUNCTION code.create_disk(
    i_cid                 text,
    i_local_id            bigint,
    i_fqdn                text,
    i_mount_point         text,
    i_rev                 bigint
) RETURNS bigint AS $$
DECLARE v_d_id bigint;
DECLARE v_pg_id bigint;
BEGIN
    SELECT pg_id INTO v_pg_id
        FROM dbaas.disk_placement_groups pg
        WHERE pg.cid = i_cid AND pg.local_id = i_local_id;

    INSERT INTO dbaas.disks as d (
        pg_id, fqdn, mount_point, status, cid
    )
    VALUES (
        v_pg_id, i_fqdn, i_mount_point, 'DESCRIBED'::dbaas.disk_status, i_cid
    )
    RETURNING d.d_id INTO v_d_id;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'create_disk',
             jsonb_build_object(
                'cid', i_cid,
                'local_id', i_local_id,
                'fqdn', i_fqdn,
                'mount_point', i_mount_point
            )
        )
    );

    RETURN v_d_id;    
END;
$$ LANGUAGE plpgsql;

--head/code/80_add_alert_group.sql
CREATE OR REPLACE FUNCTION code.add_alert_group(
    i_ag_id                 TEXT,
    i_cid                   TEXT,
    i_monitoring_folder_id  TEXT,
    i_managed               BOOLEAN,
    i_rev                   BIGINT

) RETURNS void AS $$
BEGIN
	INSERT INTO dbaas.alert_group (
		alert_group_id,
		cid,
		managed,
		monitoring_folder_id,
		status
    )
    VALUES (
		i_ag_id,
		i_cid,
		i_managed,
		i_monitoring_folder_id,
		'CREATING'
	);

	PERFORM code.update_cluster_change(
		i_cid,
		i_rev,
		jsonb_build_object(
			'add_alert_group',
			jsonb_build_object(
				'cid', i_cid,
				'alert_group', i_ag_id
			)
		)
	);
END;
$$ LANGUAGE plpgsql;

--head/code/80_add_alert_to_group.sql
CREATE OR REPLACE FUNCTION code.add_alert_to_group (
	i_cid                   TEXT,
	i_alert_group_id        TEXT,
	i_template_id           TEXT,
	i_notification_channels TEXT[],
	i_disabled              BOOLEAN,
	i_crit_threshold        NUMERIC,
	i_warn_threshold        NUMERIC,
	i_default_thresholds    BOOLEAN,
	i_rev                   BIGINT
) RETURNS VOID AS $$ 
BEGIN
	INSERT INTO dbaas.alert (
		alert_group_id,
		template_id,
		notification_channels,
		disabled,
		critical_threshold,
		warning_threshold,
		default_thresholds,
		status
	)
	VALUES (
		i_alert_group_id,
		i_template_id,
		i_notification_channels,
		i_disabled,
		i_crit_threshold,
		i_warn_threshold,
		i_default_thresholds,
		'CREATING'
	);

	PERFORM code.update_cluster_change(
		i_cid,
		i_rev,
		jsonb_build_object(
			'add_alert_to_group',
			jsonb_build_object(
				'alert_group', i_alert_group_id,
				'template_id', i_template_id
			)
		)
	);

END;
$$ LANGUAGE plpgsql;

--head/code/80_delete_alert_from_group.sql
CREATE OR REPLACE FUNCTION code.delete_alert_from_group(
	i_cid             TEXT,
	i_alert_group_id  TEXT,
	i_template_id     TEXT,
	i_rev             BIGINT
) RETURNS void AS $$
BEGIN

    UPDATE dbaas.alert SET status = 'DELETING' WHERE alert_group_id = i_alert_group_id AND template_id = i_template_id;

	PERFORM code.update_cluster_change(
		i_cid,
		i_rev,
		jsonb_build_object(
			'delete_alert_from_group',
			 jsonb_build_object(
				'cid', i_cid,
				'ag_id', i_alert_group_id,
				'template_id', i_template_id
			)
		)
	);
END;
$$ LANGUAGE plpgsql;

--head/code/80_delete_alert_group.sql
CREATE OR REPLACE FUNCTION code.delete_alert_group(
	i_alert_group_id               TEXT,
	i_cid                          TEXT,
	i_rev                          bigint,
	i_force_managed_group_deletion BOOLEAN DEFAULT FALSE
) RETURNS void AS $$
BEGIN

	IF NOT i_force_managed_group_deletion AND (SELECT managed from dbaas.alert_group WHERE alert_group_id = i_alert_group_id) THEN
		RAISE EXCEPTION 'Deletion of managed alert group % is prohibited', i_alert_group_id USING ERRCODE = '23514';
	END IF;
	UPDATE dbaas.alert_group SET status = 'DELETING' WHERE alert_group_id = i_alert_group_id;

	PERFORM code.update_cluster_change(
		i_cid,
		i_rev,
		jsonb_build_object(
			'delete_alert_group',
			 jsonb_build_object(
				'cid', i_cid,
				'ag_id', i_alert_group_id
			)
		)
	);
END;
$$ LANGUAGE plpgsql;

--head/code/80_update_alert.sql
CREATE OR REPLACE FUNCTION code.update_alert (
	i_cid                   TEXT,
	i_alert_group_id        TEXT,
	i_template_id           TEXT,
	i_notification_channels TEXT[],
	i_disabled              BOOLEAN,
	i_crit_threshold        NUMERIC,
	i_warn_threshold        NUMERIC,
	i_default_thresholds    BOOLEAN,
	i_rev                   BIGINT
) RETURNS VOID AS $$ 
BEGIN
	UPDATE
		dbaas.alert 
	SET
		critical_threshold = i_crit_threshold,
		warning_threshold = i_warn_threshold,
		notification_channels = i_notification_channels,
		disabled = i_disabled,
		default_thresholds = i_default_thresholds,
		status = 'UPDATING'
	WHERE
		alert_group_id = i_alert_group_id AND template_id = i_template_id;

	PERFORM code.update_cluster_change(
		i_cid,
		i_rev,
		jsonb_build_object(
			'update_alert',
			jsonb_build_object(
				'alert_group', i_alert_group_id,
				'template', i_template_id
			)
		)
	);

END;
$$ LANGUAGE plpgsql;

--head/code/80_update_disk.sql
CREATE OR REPLACE FUNCTION code.update_disk(
    i_cid text,
	i_fqdn text,
	i_mount_point text,
	i_host_disk_id text,
	i_disk_id text,
	i_local_id bigint,
	i_status dbaas.disk_status,
	i_rev bigint
	) RETURNS void AS $$
DECLARE v_pg_id bigint;
BEGIN
    SELECT pg_id INTO v_pg_id
    FROM dbaas.disk_placement_groups pg
    WHERE pg.cid = i_cid
      AND pg.local_id = i_local_id;

    UPDATE dbaas.disks
    SET disk_id = i_disk_id,
        host_disk_id = i_host_disk_id,
        status = i_status
    WHERE fqdn = i_fqdn
      AND mount_point = i_mount_point
      AND pg_id = v_pg_id;

    PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                    'update_disk',
                    jsonb_build_object(
                            'fqdn', i_fqdn,
                            'disk_id', i_disk_id
                        )
                )
        );

END;
$$ LANGUAGE plpgsql;

--head/code/80_update_disk_placement_group.sql
CREATE OR REPLACE FUNCTION code.update_disk_placement_group(
	i_cid text,
	i_disk_placement_group_id text,
	i_local_id bigint,
	i_rev bigint,
	i_status dbaas.disk_placement_group_status
	) RETURNS void AS
$$
BEGIN
    UPDATE dbaas.disk_placement_groups
    SET disk_placement_group_id = i_disk_placement_group_id,
        status                  = i_status
    WHERE cid = i_cid
      AND local_id = i_local_id;

    PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                    'update_placement_group',
                    jsonb_build_object(
                            'cid', i_cid,
                            'disk_placement_group_id', i_disk_placement_group_id
                        )
                )
        );

END;
$$ LANGUAGE plpgsql;

--head/code/81_create_placement_group.sql
CREATE OR REPLACE FUNCTION code.create_placement_group(
    i_cid                 text,
    i_rev                 bigint
) RETURNS bigint AS $$
DECLARE v_pg_id bigint;
BEGIN
    INSERT INTO dbaas.placement_groups as pg (
        cid, status
    )
    VALUES (
        i_cid, 'DESCRIBED'::dbaas.placement_group_status
    )
    RETURNING pg.pg_id INTO v_pg_id;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'create_placement_group',
             jsonb_build_object(
                'cid', i_cid
            )
        )
    );

    RETURN v_pg_id;
END;
$$ LANGUAGE plpgsql;

--head/code/82_update_placement_group.sql
CREATE OR REPLACE FUNCTION code.update_placement_group(
	i_cid text,
	i_placement_group_id text,
	i_rev bigint,
	i_status dbaas.placement_group_status
	) RETURNS void AS
$$
BEGIN
    UPDATE dbaas.placement_groups
    SET placement_group_id = i_placement_group_id,
        status             = i_status
    WHERE cid = i_cid;

    PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                    'update_placement_group',
                    jsonb_build_object(
                            'cid', i_cid,
                            'placement_group_id', i_placement_group_id
                        )
                )
        );

END;
$$ LANGUAGE plpgsql;

