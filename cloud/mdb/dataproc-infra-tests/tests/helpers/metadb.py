"""
Metadb data generation helper
"""

import os
import re
from contextlib import contextmanager

import psycopg2
from psycopg2.extensions import adapt
from psycopg2.extras import DictCursor


def mogrify(query, kwargs):
    """
    Mogrify query (without working connection instance)
    """
    return query % {key: adapt(value).getquoted().decode('utf-8') for key, value in kwargs.items()}


def _connect(context):
    metadb_opts = context.conf['projects']['metadb']

    host = context.conf['compute_driver']['fqdn']
    port = 6432

    dsn = ('host={host} port={port} dbname={dbname} ' 'user={user} password={password} sslmode=require').format(
        host=host, port=port, dbname=metadb_opts['dbname'], user=metadb_opts['user'], password=metadb_opts['password']
    )

    return psycopg2.connect(dsn, cursor_factory=DictCursor)


@contextmanager
def _query_master(context, query, *args):
    """
    Run query on master yields cursor
    """
    with _connect(context) as conn:
        cur = conn.cursor()
        cur.execute(query, args)
        yield cur


def get_version(context):
    """
    Get actual version of metadb
    """
    with _query_master(context, 'SELECT max(version) AS v from public.schema_version') as cur:
        return cur.fetchone()['v']


def get_latest_version(context):
    """
    Get latest migration version
    """
    migration_file_re = re.compile(r'V(?P<version>\d+)__(?P<description>.+)\.sql$')
    migrations_dir = os.path.join(
        context.conf['staging_dir'],
        'code',
        'metadb',
        'migrations',
    )
    files = os.listdir(migrations_dir)
    max_version = 0
    for migration_file in files:
        match = migration_file_re.match(migration_file)
        if match is None:
            pass
        version = int(match.group('version'))
        if version > max_version:
            max_version = version

    if max_version > 0:
        return max_version
    raise RuntimeError('Malformed metadb migrations dir')


def get_cluster_tasks(context, folder_id, cid, task_type):
    """
    Get cluster tasks
    """
    query = """
    SELECT *
        FROM code.get_operations(
            i_folder_id      => %s,
            i_cid            => %s,
            i_type           => %s,
            i_limit          => NULL,
            i_include_hidden => true
        )"""
    with _query_master(context, query, folder_id, cid, task_type) as cur:
        return cur.fetchall()


def get_task(context, folder_id, task_id):
    """
    Get cluster task by its id
    """
    query = """
    SELECT *
      FROM code.get_operation_by_id(
        i_folder_id     => %s,
        i_operation_id  => %s
    )
    """
    with _query_master(context, query, folder_id, task_id) as cur:
        return cur.fetchone()


def set_delayed_until_to_now(context, task_id):
    """
    Set delayed_until for task by id
    """
    query = """
    UPDATE dbaas.worker_queue
       SET delayed_until = now()
     WHERE task_id = %s"""
    with _query_master(context, query, task_id):
        pass


def set_pg_connection_pooler(context, pooler):
    """
    Set connection pooler for all clusters
    """
    query = """
    UPDATE dbaas.pillar
       SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(%s::text))
     WHERE cid IS NOT NULL
    """
    with _query_master(context, query, pooler):
        pass


def get_all_hosts(context):
    """
    Get all hosts
    """
    query = """
        SELECT
        fqdn, vtype_id
        FROM
        dbaas.hosts"""
    with _query_master(context, query) as cur:
        return cur.fetchall()


def get_cluster_hosts_with_role(context, role):
    """
    Get cluster hosts with specific role
    """
    query = """
        SELECT fqdn
        FROM dbaas.hosts AS h
        LEFT JOIN dbaas.subclusters AS s USING(subcid)
        LEFT JOIN dbaas.clusters AS c USING(cid)
        WHERE c.name=%s AND c.status='RUNNING' AND s.roles::text[] = ARRAY[%s]"""
    with _query_master(context, query, context.cluster['name'], role) as cur:
        return cur.fetchall()


def get_masternode(context):
    """
    Get cluster masternode fqdn and instance_id
    """
    query = """
        SELECT fqdn, vtype_id
        FROM dbaas.hosts AS h
        LEFT JOIN dbaas.subclusters AS s USING(subcid)
        LEFT JOIN dbaas.clusters AS c USING(cid)
        WHERE c.name=%s AND c.status='RUNNING' AND s.roles::text[] = ARRAY['hadoop_cluster.masternode']
    """
    with _query_master(context, query, context.cluster['name']) as cur:
        return cur.fetchone()


def truncate_clusters_cascade(context):
    """
    Truncate dbaas.clusters and all relations for cleanup environment
    """
    query = """
        TRUNCATE
        dbaas.clusters
        CASCADE"""
    with _query_master(context, query):
        pass


def truncate_worker_queue_cascade(context):
    """
    Truncate dbaas.worker_queue and all relations for cleanup environment
    """
    query = """
        TRUNCATE
        dbaas.worker_queue
        CASCADE"""
    with _query_master(context, query):
        pass


def running_job(context):
    """
    Returns most recently submitted running job
    """
    query = """
        SELECT job_id, cid
        FROM dbaas.hadoop_jobs
        WHERE end_ts IS NULL
        ORDER BY create_ts DESC
        LIMIT 1"""
    with _query_master(context, query) as cur:
        return cur.fetchone()


def get_job(context, job_id):
    """
    Returns jobs by id
    """
    query = """
        SELECT *
        FROM dbaas.hadoop_jobs
        WHERE job_id=%s"""
    with _query_master(context, query, job_id) as cur:
        return cur.fetchone()


def restart_operation(context):
    """
    Returns jobs by id
    """
    query = """
        SELECT code.restart_task(%s)
        """
    with _query_master(context, query, context.operation_id) as cur:
        return cur.fetchone()


def set_feature_flag(context, feature_flag):
    query = """
        INSERT INTO dbaas.cloud_feature_flags (cloud_id, flag_name)
        VALUES (1, %s)
        ON CONFLICT DO NOTHING
        """
    with _query_master(context, query, feature_flag):
        pass


def remove_feature_flag(context, feature_flag):
    query = """
        DELETE FROM dbaas.cloud_feature_flags
        WHERE flag_name=%s"""
    with _query_master(context, query, feature_flag):
        pass


def get_instance_group_id(context, subcluster_name):
    query = """
        SELECT instance_group_id
        FROM dbaas.instance_groups AS ig
        INNER JOIN dbaas.subclusters AS s
        USING (subcid)
        WHERE name=%s"""
    with _query_master(context, query, subcluster_name) as cur:
        return cur.fetchone()[0]


def update_default_cluster_pillar(context, cluster_type, path, value):
    query = """
        update dbaas.cluster_type_pillar
        set value = jsonb_set(value, '{{{path}}}', '{value}'::jsonb)
        where type = '{cluster_type}_cluster';
        """.format(
        path=','.join(path), value=value, cluster_type=cluster_type
    )
    with _query_master(context, query):
        pass
