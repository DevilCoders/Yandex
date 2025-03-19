"""
Metadb data generation helper
"""

import json
import os
import re
import string
from contextlib import contextmanager
from random import SystemRandom

import psycopg2
from psycopg2.extensions import adapt
from psycopg2.extras import DictCursor

from .crypto import argon2
from .docker import get_container, get_exposed_port
from .utils import env_stage

QUERY_BACKUPS = "SELECT * from dbaas.backups WHERE cid = %s ORDER BY created_at"


def mogrify(query, kwargs):
    """
    Mogrify query (without working connection instance)
    """
    return query % {key: adapt(value).getquoted().decode('utf-8') for key, value in kwargs.items()}


class SqlFile:
    """
    SQL-population helper
    """

    def __init__(self, base, name):
        self.base = base
        self.name = name
        self.buffer = list()  # type: list

    def add_query(self, query, **kwargs):
        """
        Add query to buffer
        """
        self.buffer.append(mogrify(query, kwargs))

    def dump(self):
        """
        Dump result to file
        """
        os.makedirs(self.base, exist_ok=True)
        path = os.path.join(self.base, '{name}.sql'.format(name=self.name))
        with open(path, 'w') as out:
            out.writelines(self.buffer)


def add_access_ids_data(sql, dynamic_config):
    """
    Add client access ids to initial sql
    """
    sql.add_query(
        """
        INSERT INTO dbaas.config_host_access_ids (
            access_id,
            access_secret,
            active,
            type
        ) VALUES (
            %(salt_access_id)s,
            %(hashed_salt_access_secret)s,
            true,
            'default'::dbaas.config_host_type
        ), (
            %(dbaas_worker_access_id)s,
            %(hashed_dbaas_worker_access_secret)s,
            true,
            'dbaas-worker'::dbaas.config_host_type
        );
        """,
        salt_access_id=dynamic_config['salt']['access_id'],
        hashed_salt_access_secret=argon2(dynamic_config['salt']['access_secret']),
        dbaas_worker_access_id=dynamic_config['mdb_api_for_worker']['access_id'],
        hashed_dbaas_worker_access_secret=argon2(dynamic_config['mdb_api_for_worker']['access_secret']),
    )


def add_clouds_data(sql, dynamic_config):
    """
    Add clouds to initial sql
    """
    for props in dynamic_config['clouds'].values():
        if 'cloud_id' in props:
            raise RuntimeError('Remove cloud_id. Resolve it dynamicly from cloud_ext_i')
        sql.add_query(
            """
            SELECT *
              FROM code.add_cloud(
                 i_cloud_ext_id => %(cloud_ext_id)s,
                 i_quota => code.make_quota(
                    i_cpu       => %(cpu_quota)s,
                    i_memory    => %(memory_quota)s,
                    i_ssd_space => %(ssd_space_quota)s,
                    i_hdd_space => %(hdd_space_quota)s,
                    i_clusters  => %(clusters_quota)s
                ),
                i_x_request_id => 'tests-init'
            );
            """,
            cloud_ext_id=props['cloud_ext_id'],
            cpu_quota=props.get('cpu_quota', 48),
            memory_quota=props.get('memory_quota', 206158430208),
            ssd_space_quota=props.get('ssd_space_quota', 3298534883328),
            hdd_space_quota=props.get('hdd_space_quota', 3298534883328),
            clusters_quota=props.get('clusters_quota', 10),
        )


def add_folders_data(sql, dynamic_config):
    """
    Add folders to initial sql
    """
    for props in dynamic_config['folders'].values():
        sql.add_query(
            """
            INSERT INTO dbaas.folders (
                folder_id,
                folder_ext_id,
                cloud_id
            )
            SELECT
                %(folder_id)s,
                %(folder_ext_id)s,
                cloud_id
            FROM dbaas.clouds
            WHERE cloud_ext_id = %(cloud_ext_id)s;
            """,
            folder_id=props['folder_id'],
            folder_ext_id=props['folder_ext_id'],
            cloud_ext_id=props['cloud_ext_id'],
        )


def set_default_pillars(sql, conf, state):
    """
    Set default pillars
    """
    sql.add_query("""
        DELETE FROM dbaas.default_pillar;
        """)

    sql.add_query(
        """
        INSERT INTO dbaas.default_pillar (
            id, value
        ) VALUES (
            1, %(value)s::jsonb
        );
        """,
        value=json.dumps({
            'data': {
                'dist': {
                    'bionic': {
                        'secure': True,
                    },
                    'mdb-bionic': {
                        'secure': True,
                    },
                    'pgdg': {
                        'absent': True,
                    },
                },
                'allow_salt_version_update': False,
                'salt_version': '3002.7+ds-1+yandex',
                's3': {
                    'endpoint_source': 'this.url.will.fail',
                    'endpoint': conf['dynamic']['s3']['endpoint'],
                    'use_https': conf['dynamic']['s3']['use_https'],
                    'access_secret_key': {
                        'data': conf['dynamic']['s3']['enc_access_secret_key'],
                        'encryption_version': 1,
                    },
                    'access_key_id': {
                        'data': conf['dynamic']['s3']['enc_access_key_id'],
                        'encryption_version': 1,
                    },
                },
                'monrun2': True,
                'selfdns_disable': True,
                'use_yasmagent': False,
                'use_mdbsecrets': True,
                'cauth_use': False,
                'database_slice': {
                    'enable': True,
                },
                'ship_logs': False,
                'billing': {
                    'ship_logs': False,
                },
                'mdb_metrics': {
                    'enabled': False,
                },
                'certs': {
                    'readonly': True,
                },
            },
        }),
    )
    # We use this hack here to avoid double \n-escaping
    sql.add_query(
        """
        UPDATE dbaas.default_pillar
           SET value = jsonb_set(
                    jsonb_set(
                        value,
                        ARRAY['cert.ca'],
                        to_jsonb(%(ca)s::text)
                    ),
                    ARRAY['internal.cert.ca'],
                    to_jsonb(%(ca)s::text)
                );
        """,
        ca=state['ssl'].get_cert('CA'),
    )


def set_disk_type_ids(sql, conf):
    """
    Set disk_type_ids
    """
    sql.add_query("""
        DELETE FROM dbaas.disk_type;
        """)

    for disk_type in conf['disk_type_config']:
        sql.add_query(
            """
            INSERT INTO dbaas.disk_type (
                disk_type_id,
                quota_type,
                disk_type_ext_id
            ) VALUES (
                %(disk_type_id)s,
                %(quota_type)s,
                %(disk_type_ext_id)s
            );
            """,
            **disk_type,
        )


def set_regions(sql, conf):
    """
    Set geos
    """
    sql.add_query("""
        DELETE FROM dbaas.regions;
        """)

    for region in conf['regions']:
        sql.add_query(
            """
            INSERT INTO dbaas.regions (
                region_id,
                name,
                cloud_provider
            ) VALUES (
                %(region_id)s,
                %(name)s,
                %(cloud_provider)s
            );
            """,
            **region,
        )


def set_geos(sql, conf):
    """
    Set geos
    """
    sql.add_query("""
        DELETE FROM dbaas.geo;
        """)

    for geo in conf['geos']:
        sql.add_query(
            """
            INSERT INTO dbaas.geo (
                geo_id,
                name,
                region_id
            ) VALUES (
                %(geo_id)s,
                %(name)s,
                %(region_id)s
            );
            """,
            **geo,
        )


def set_flavors(sql, conf):
    """
    Set flavors
    """
    sql.add_query("""
        DELETE FROM dbaas.flavors;
        """)

    sql.add_query("""
        INSERT INTO dbaas.flavor_type (id, type, generation) VALUES (1, 'standard', 1) ON CONFLICT DO NOTHING ;
        """)

    for flavor in conf['flavors']:
        sql.add_query(
            """
            INSERT INTO dbaas.flavors (
                id,
                cpu_guarantee,
                cpu_limit,
                memory_guarantee,
                memory_limit,
                network_guarantee,
                network_limit,
                io_limit,
                name,
                visible,
                vtype,
                platform_id
            ) VALUES (
                %(id)s,
                %(cpu_guarantee)s,
                %(cpu_limit)s,
                %(memory_guarantee)s,
                %(memory_limit)s,
                %(network_guarantee)s,
                %(network_limit)s,
                %(io_limit)s,
                %(name)s,
                %(visible)s,
                %(vtype)s,
                %(platform_id)s
            );
            """,
            **flavor,
        )


def set_default_versions(sql, conf):
    """
    Fill dbaas.default_versions
    """
    sql.add_query("""
        DELETE FROM dbaas.default_versions;
        """)

    if len(conf['default_versions_config']) == 0:
        return

    query_prefix = """INSERT INTO dbaas.default_versions (
    type,
    component,
    major_version,
    minor_version,
    edition,
    package_version,
    env,
    is_deprecated,
    is_default,
    updatable_to,
    name
) VALUES """

    i = 0
    query_parts = []
    common_resource_combination = {}
    for resource_combination in conf['default_versions_config']:
        query_parts.append("""(
                    %(type_{i})s,
                    %(component_{i})s,
                    %(major_version_{i})s,
                    %(minor_version_{i})s,
                    %(edition_{i})s,
                    %(package_version_{i})s,
                    %(env_{i})s,
                    %(is_deprecated_{i})s,
                    %(is_default_{i})s,
                    %(updatable_to_{i})s,
                    %(name_{i})s
                    )
                """.format(i=i))
        common_resource_combination.update(
            {'{key}_{i}'.format(key=key, i=i): value
             for key, value in resource_combination.items()})
        i += 1
    query = query_prefix + ', '.join(query_parts) + ';'
    sql.add_query(query, **common_resource_combination)


def set_valid_resources(sql, conf):
    """
    Fill dbaas.valid_resources table with existing flavors/geos
    """
    sql.add_query("""
        DELETE FROM dbaas.valid_resources;
        """)

    for index, resource_combination in enumerate(conf['resources_config']):
        combination = resource_combination.copy()
        combination['id'] = index + 1
        sql.add_query(
            """
            INSERT INTO dbaas.valid_resources (
                id,
                cluster_type,
                role,
                flavor,
                disk_type_id,
                geo_id,
                disk_size_range,
                disk_sizes,
                min_hosts,
                max_hosts
            ) VALUES (
                %(id)s,
                %(cluster_type)s,
                %(role)s,
                %(flavor)s,
                %(disk_type_id)s,
                %(geo_id)s,
                %(disk_size_range)s,
                %(disk_sizes)s,
                %(min_hosts)s,
                %(max_hosts)s
            );
            """,
            **combination,
        )


def set_cluster_type_pillars(sql, conf):
    """
    Set cluster_type-specific pillars
    """
    sql.add_query("""
        DELETE FROM dbaas.cluster_type_pillar;
        """)
    for cluster_type, value in conf['dynamic']['cluster_type_pillars'].items():
        sql.add_query(
            """
            INSERT INTO dbaas.cluster_type_pillar (
                type, value
            ) VALUES (
                %(type)s,
                %(value)s
            );
            """,
            type=cluster_type,
            value=json.dumps(value),
        )


def set_default_feature_flags(sql, conf):
    """
    Set feature flags
    """
    for flag in conf['feature_flags']:
        sql.add_query(
            """
            INSERT INTO dbaas.default_feature_flags (
                flag_name
            ) VALUES (
                %(flag_name)s
            );
            """,
            flag_name=flag,
        )


@env_stage('create', fail=True)
def gen_initial_data(*, conf, state):
    """
    Add initial clouds, folders, config clients, cluster_type defaults
    """
    metadb_dir = os.path.join(
        conf['staging_dir'],
        'code',
        'metadb',
    )
    base_dir = os.path.join(
        metadb_dir,
        'data',
    )
    sql = SqlFile(base_dir, '01_Initial')
    add_access_ids_data(sql, conf['dynamic'])
    add_clouds_data(sql, conf['dynamic'])
    add_folders_data(sql, conf['dynamic'])
    set_default_pillars(sql, conf, state)
    set_cluster_type_pillars(sql, conf)
    set_regions(sql, conf)
    set_geos(sql, conf)
    set_disk_type_ids(sql, conf)
    set_flavors(sql, conf)
    set_valid_resources(sql, conf)
    set_default_versions(sql, conf)
    set_default_feature_flags(sql, conf)
    sql.dump()


def _connect(context):
    metadb_opts = context.conf['projects']['metadb']['db']

    host, port = get_exposed_port(
        get_container(context, 'metadb01'), context.conf['projects']['metadb']['metadb01']['expose']['pgbouncer'])

    dsn = ('host={host} port={port} dbname={dbname} '
           'user={user} password={password} sslmode=require').format(
               host=host,
               port=port,
               dbname=metadb_opts['dbname'],
               user=metadb_opts['user'],
               password=metadb_opts['password'])

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


def get_first_backup(context, cid):
    """
    Get cluster first backup
    """
    with _query_master(context, QUERY_BACKUPS, cid) as cur:
        return cur.fetchone()


def get_cluster_backups(context, cid):
    """
    Get cluster backups ordered by creation time
    """
    with _query_master(context, QUERY_BACKUPS, cid) as cur:
        return cur.fetchall()


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

    revs_query = """
    UPDATE dbaas.pillar_revs
       SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(%s::text))
     WHERE cid IS NOT NULL
    """
    with _query_master(context, revs_query, pooler):
        pass


def get_subclusters_shards_by_cid(context, cid):
    """
    Get cluster subclusters and shards info
    """
    query = """
    SELECT
        su.subcid as subcluster_id,
        su.name as subcluster_name,
        sh.shard_id as shard_id,
        sh.name as shard_name
    FROM
        dbaas.subclusters su
        LEFT JOIN dbaas.shards sh USING (subcid)
    WHERE
        su.cid = %s
    """
    with _query_master(context, query, cid) as cur:
        return cur.fetchall()


def set_backup_service_usage(context, cid, enabled):
    """
    Set backup service usage
    """
    query = """
    SELECT * FROM code.set_backup_service_use(
        i_cid => %s,
        i_val => %s
    )
    """
    with _query_master(context, query, cid, enabled):
        pass


def disable_ch_zk_tls(context, cluster_name):
    """Disable TLS in ClickHouse cluster pillar."""
    query = """
    UPDATE
        dbaas.pillar
    SET
        value = jsonb_set(value, '{data,unmanaged,enable_zk_tls}', 'false', true)
    WHERE
        cid = (SELECT cid FROM dbaas.clusters WHERE name = %s)
    """
    with _query_master(context, query, cluster_name):
        pass

    revs_query = """
    UPDATE
        dbaas.pillar_revs
    SET
        value = jsonb_set(value, '{data,unmanaged,enable_zk_tls}', 'false', true)
    WHERE
        cid = (SELECT cid FROM dbaas.clusters WHERE name = %s)
    """
    with _query_master(context, revs_query, cluster_name):
        pass


def enable_ch_zk_tls(context, cluster_name):
    """Enable TLS in ClickHouse cluster pillar."""
    query = """
    UPDATE
        dbaas.pillar
    SET
        value = jsonb_set(value, '{data,unmanaged,enable_zk_tls}', 'true', true)
    WHERE
        cid = (SELECT cid FROM dbaas.clusters WHERE name = %s)
    """
    with _query_master(context, query, cluster_name):
        pass

    revs_query = """
    UPDATE
        dbaas.pillar_revs
    SET
        value = jsonb_set(value, '{data,unmanaged,enable_zk_tls}', 'true', true)
    WHERE
        cid = (SELECT cid FROM dbaas.clusters WHERE name = %s)
    """
    with _query_master(context, revs_query, cluster_name):
        pass


def enable_ch_zk_acl(context, cluster_name):
    """Enable ACL without TLS in ClickHouse cluster pillar."""
    query = """
    UPDATE
        dbaas.pillar
    SET value = jsonb_set(
        jsonb_set(value, '{data,unmanaged,enable_zk_tls}', to_jsonb(CAST('true' as BOOLEAN))),
        '{data,unmanaged,tmp_disable_zk_tls_mdb_12035}', to_jsonb(CAST('true' as BOOLEAN)))
    WHERE
        cid = (SELECT cid FROM dbaas.clusters WHERE name = %s)
    """
    with _query_master(context, query, cluster_name):
        pass

    revs_query = """
    UPDATE
        dbaas.pillar_revs
    SET value = jsonb_set(
        jsonb_set(value, '{data,unmanaged,enable_zk_tls}', to_jsonb(CAST('true' as BOOLEAN))),
        '{data,unmanaged,tmp_disable_zk_tls_mdb_12035}', to_jsonb(CAST('true' as BOOLEAN)))
    WHERE
        cid = (SELECT cid FROM dbaas.clusters WHERE name = %s)
    """
    with _query_master(context, revs_query, cluster_name):
        pass


def generate_id():
    """Generate random ID for task."""
    id_prefix = 'mdb'
    symbols = string.ascii_lowercase.replace('wxyz', '') + string.digits
    return id_prefix + ''.join(SystemRandom().choice(symbols) for _ in range(17))


def create_maintenance_task(context, cluster_type, cluster_name, task_args):
    """Make maintenance task for cluster."""
    task_id = generate_id()
    # begin_transaction(context)
    # cid, rev = lock_cluster(context, cluster_name)
    query = """
    WITH
    lc AS (
        SELECT
            cid,
            rev
        FROM code.lock_cluster(i_cid => (SELECT cid FROM dbaas.clusters WHERE name = %s))
    ),
    cluster AS (
        SELECT
            cid,
            folder_id,
            code.rev(c) rev,
            CASE
               WHEN type = 'postgresql_cluster' THEN value -> 'data' -> 'pgsync' -> 'zk_hosts'
               WHEN type = 'mysql_cluster' THEN value -> 'data' -> 'mysql' -> 'zk_hosts'
               ELSE CAST('null' AS jsonb)
            END zk_hosts
        FROM dbaas.clusters c
        JOIN dbaas.pillar USING (cid)
        WHERE c.cid = (SELECT cid FROM lc)
    ),
    ao AS (
        SELECT code.add_operation(
            i_operation_id    => %s,
            i_cid             => c.cid,
            i_folder_id       => c.folder_id,
            i_operation_type  => %s,
            i_task_type       => %s,
            i_task_args       => jsonb_set(%s, '{zk_hosts}', c.zk_hosts),
            i_metadata        => '{}',
            i_user_id         => '',
            i_version         => 2,
            i_hidden          => false,
            i_time_limit      => NULL,
            i_rev             => (SELECT rev FROM lc),
            i_delay_by        => interval '1 second'
        )
        FROM cluster c
    )
    SELECT code.complete_cluster_change(
        (SELECT cid FROM lc),
        (SELECT rev FROM lc)
    ), (SELECT * FROM ao);
    """
    opertaion_type = cluster_type + '_cluster_modify'
    task_type = cluster_type + '_cluster_maintenance'
    with _query_master(context, query, cluster_name, task_id, opertaion_type, task_type, task_args):
        pass
    # complete_cluster_change(context, cid, rev)
    # commit_transaction(context)
    return task_id


def set_cluster_pillar_value(context, cid, pillar_path, pillar_value):
    """Updates cluster pillar value by path"""
    set_pillar = """SELECT code.easy_update_pillar(%s, %s, %s)"""
    with _query_master(context, set_pillar, cid, pillar_path, pillar_value):
        pass
