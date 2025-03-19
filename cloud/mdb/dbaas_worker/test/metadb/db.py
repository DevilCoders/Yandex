import logging
from queue import Queue

from dataclasses import dataclass
import pytest

from test.mocks import _get_config
from cloud.mdb.dbaas_worker.internal.metadb import get_cursor
from cloud.mdb.dbaas_worker.internal.providers.metadb_alert import MetadbAlert
from .utils import get_recipe_dsn_from_env

log = logging.getLogger(__name__)

CLOUD_ID = 'test-cloud-id'

ALERT_GROUP_ID = '1'
METRIC_NAME = 'test_template_id'
CONDITION = 'LESS'


DEFAULT_CLOUD_QUOTA = {
    'clusters_quota': 2,
    'cpu_quota': 2 * 3 * 16,
    'gpu_quota': 0,
    'memory_quota': 2 * 3 * 68719476736,
    'ssd_space_quota': 2 * 3 * 858993459200,
    'hdd_space_quota': 2 * 3 * 858993459200,
}


@dataclass
class ConnectionCfg:
    port: str
    conn_string: str
    hosts: list[str]


def _create_cloud(cursor) -> int:
    create_cloud = '''
                SELECT
            cloud_id,
            cloud_ext_id,
            cpu_quota,
            gpu_quota,
            memory_quota,
            ssd_space_quota,
            hdd_space_quota,
            clusters_quota,
            memory_used,
            ssd_space_used,
            hdd_space_used,
            cpu_used,
            gpu_used,
            clusters_used
          FROM code.add_cloud(
            i_cloud_ext_id => %(cloud_ext_id)s,
            i_quota => code.make_quota(
                i_cpu       => %(cpu_quota)s::real,
                i_gpu       => %(gpu_quota)s::bigint,
                i_memory    => %(memory_quota)s::bigint,
                i_ssd_space => %(ssd_space_quota)s::bigint,
                i_hdd_space => %(hdd_space_quota)s::bigint,
                i_clusters  => %(clusters_quota)s::bigint
            ),
            i_x_request_id => %(x_request_id)s
        )'''
    cursor.execute(
        create_cloud,
        dict(
            cloud_ext_id=CLOUD_ID,
            x_request_id='test-req-id',
            **DEFAULT_CLOUD_QUOTA,
        ),
    )
    return cursor.fetchall()[0]['cloud_id']


FOLDER_ID = 'test-folder-id'


def _create_folder(cursor, in_cloud_id: int) -> int:
    folder_id = 1
    sql = '''
    INSERT INTO dbaas.folders (folder_id, folder_ext_id, cloud_id)
    VALUES (
            %(folder_id)s,
            %(folder_ext_id)s,
            %(cloud_id)s
    )'''
    cursor.execute(
        sql,
        dict(
            folder_id=folder_id,
            folder_ext_id=FOLDER_ID,
            cloud_id=in_cloud_id,
        ),
    )
    return folder_id


CLOUDS_CREATED = {}

CLUSTER_ID = 'test-cid'

CLUSTERS_CREATED = {}
TEMPLATE_VERSION = 'test-version'
TEMPLATE_ID = 'test-templ-id'


def _create_cluster(cursor, folder_id: int):
    sql = '''SELECT cid, folder_id, name,
           type, network_id, env,
           rev
      FROM code.create_cluster(
          i_cid                 => %(cid)s,
          i_name                => %(name)s,
          i_type                => %(type)s,
          i_env                 => %(env)s,
          i_public_key          => %(public_key)s,
          i_network_id          => %(network_id)s,
          i_folder_id           => %(folder_id)s,
          i_description         => %(description)s,
          i_x_request_id        => %(x_request_id)s,
          i_host_group_ids      => %(host_group_ids)s,
          i_deletion_protection => %(deletion_protection)s
    )
    '''
    cursor.execute(
        sql,
        dict(
            master=True,
            folder_id=folder_id,
            cid=CLUSTER_ID,
            name='test-cluster-name',
            type='postgresql_cluster',
            env='compute-prod',
            network_id='test-net-id',
            public_key='pub-ky',
            description='test cluster description',
            x_request_id='test-req-id',
            host_group_ids=[],
            deletion_protection=False,
        ),
    )
    sql = '''
    SELECT *
    FROM code.complete_cluster_change (
        i_cid := %(cid)s,
        i_rev := %(rev)s
    )'''
    cursor.execute(
        sql,
        dict(
            cid=CLUSTER_ID,
            rev=1,
        ),
    )


def _prepare_db(cursor):
    cloud_id = _create_cloud(cursor)
    folder_id = _create_folder(cursor, cloud_id)
    _create_cluster(cursor, folder_id)
    cursor.execute('COMMIT', {})


@pytest.fixture(scope='module')
def empty_metadb():
    # prepare db
    dsn = get_recipe_dsn_from_env('METADB')
    conn_string = 'user=dbaas_worker port={port} dbname=dbaas_metadb sslmode=verify-full'.format(port=dsn['port'])

    conn_cfg = ConnectionCfg(port=dsn['port'], conn_string=conn_string, hosts=[dsn['host']])
    with get_cursor(conn_cfg.conn_string, conn_cfg.hosts, log) as transaction:
        _prepare_db(transaction)
        # do the testing
        yield conn_cfg

    # tear down


@pytest.fixture()
def metadb_with_alert_group(empty_metadb: ConnectionCfg):  # noqa: F811
    with get_cursor(empty_metadb.conn_string, empty_metadb.hosts, log) as cursor:
        sql = '''SELECT * FROM code.lock_cluster(%(cid)s, %(req)s)'''
        cursor.execute(
            sql,
            dict(
                req='test-req-id',
                cid=CLUSTER_ID,
            ),
        )
        cluster = cursor.fetchall()[0]
        sql = '''
SELECT * FROM code.add_alert_group(
    i_ag_id := %(ag_id)s,
    i_cid := %(cid)s,
    i_monitoring_folder_id := %(folder)s,
    i_managed := true,
    i_rev := %(rev)s
)
        '''
        cursor.execute(
            sql,
            dict(
                ag_id=ALERT_GROUP_ID,
                cid=CLUSTER_ID,
                folder='solomon-project-id',
                rev=cluster['rev'],
            ),
        )
        sql = '''
        INSERT INTO dbaas.default_alert
        (
        template_id,
        template_version,
        critical_threshold,
        warning_threshold,
        cluster_type)
        VALUES (
            %(template_id)s,
            %(template_version)s,
            %(critical_threshold)s,
            %(warning_threshold)s,
            %(cluster_type)s
        )
        '''
        cursor.execute(
            sql,
            dict(
                template_version=TEMPLATE_VERSION,
                template_id=TEMPLATE_ID,
                critical_threshold=90,
                warning_threshold=20,
                condition=CONDITION,
                cluster_type=cluster['type'],
                selectors='test-selectors',
            ),
        )
        sql = '''
        SELECT *
        FROM code.complete_cluster_change (
            i_cid := %(cid)s,
            i_rev := %(rev)s
        )'''
        cursor.execute(
            sql,
            dict(
                cid=CLUSTER_ID,
                rev=cluster['rev'],
            ),
        )
        cursor.execute('COMMIT')
        yield empty_metadb


@pytest.fixture()
def metadb_alerts_provider(empty_metadb: ConnectionCfg) -> MetadbAlert:  # noqa: F811
    config = _get_config()
    config.main.metadb_dsn = empty_metadb.conn_string
    config.main.metadb_hosts = empty_metadb.hosts
    queue = Queue(maxsize=10000)
    task = {
        'cid': 'cid-test',
        'task_id': 'test_id',
        'task_type': 'test-task',
        'feature_flags': [],
        'folder_id': 'test_folder',
        'context': {},
        'timeout': 3600,
        'changes': [],
    }
    return MetadbAlert(config, task, queue)
