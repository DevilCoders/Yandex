"""
Metadb setup/config/teardown
"""
import errno
import json
import yaml
import os
import re
import subprocess
import sys
import time
import uuid
import yatest.common
from argon2 import PasswordHasher, Type
from contextlib import closing
from pathlib import Path
from urllib.parse import urlparse

import psycopg2
import tenacity

from cloud.mdb.recipes.postgresql.lib import PersistenceConfig
from . import project

try:
    import docker
except ImportError:
    docker = None

REPO = 'metadb'

CLUSTER_ROLE_DATA = {
    ('postgresql_cluster', 'postgresql_cluster'): {
        'min_hosts': 1,
        'max_hosts': 32,
        'flavor_types': [('standard', 1), ('burstable', 1), ('standard', 2), ('burstable', 2), ('standard', 3)],
    },
    ('clickhouse_cluster', 'clickhouse_cluster'): {
        'min_hosts': 1,
        'max_hosts': 7,
        'flavor_types': [('standard', 1), ('burstable', 1), ('standard', 2), ('burstable', 2), ('aws-standard', 1)],
    },
    ('clickhouse_cluster', 'zk'): {
        'min_hosts': 3,
        'max_hosts': 5,
        'flavor_types': [('standard', 1), ('standard', 2), ('aws-standard', 1)],
    },
    ('mongodb_cluster', 'mongodb_cluster.mongod'): {
        'min_hosts': 1,
        'max_hosts': 7,
        'flavor_types': [('standard', 1), ('burstable', 1), ('standard', 2), ('burstable', 2)],
    },
    ('mongodb_cluster', 'mongodb_cluster.mongos'): {
        'min_hosts': 0,
        'max_hosts': 7,
        'flavor_types': [('standard', 1), ('standard', 2)],
    },
    ('mongodb_cluster', 'mongodb_cluster.mongocfg'): {
        'min_hosts': 0,
        'max_hosts': 7,
        'flavor_types': [('standard', 1), ('standard', 2)],
    },
    ('mongodb_cluster', 'mongodb_cluster.mongoinfra'): {
        'min_hosts': 0,
        'max_hosts': 7,
        'flavor_types': [('standard', 1), ('standard', 2)],
    },
    ('mysql_cluster', 'mysql_cluster'): {
        'min_hosts': 1,
        'max_hosts': 7,
        'flavor_types': [('standard', 1), ('burstable', 1), ('standard', 2), ('burstable', 2), ('standard', 3)],
    },
    ('sqlserver_cluster', 'sqlserver_cluster'): {
        'min_hosts': 1,
        'max_hosts': 7,
        'flavor_types': [('standard', 1), ('standard', 2)],
    },
    ('greenplum_cluster', 'greenplum_cluster.master_subcluster'): {
        'min_hosts': 1,
        'max_hosts': 2,
        'flavor_types': [('standard', 1), ('standard', 2), ('standard', 3), ('io', 2)],
    },
    ('greenplum_cluster', 'greenplum_cluster.segment_subcluster'): {
        'min_hosts': 2,
        'max_hosts': 8,
        'flavor_types': [('standard', 1), ('standard', 2), ('standard', 3), ('io', 2)],
    },
    ('redis_cluster', 'redis_cluster'): {
        'min_hosts': 1,
        'max_hosts': 7,
        'flavor_types': [('standard', 1), ('burstable', 1), ('standard', 2), ('burstable', 2)],
    },
    ('hadoop_cluster', 'hadoop_cluster.masternode'): {
        'min_hosts': 1,
        'max_hosts': 3,
        'flavor_types': [('standard', 1), ('burstable', 1), ('standard', 2), ('burstable', 2), ('gpu', 3)],
        'vtype': 'compute',
    },
    ('hadoop_cluster', 'hadoop_cluster.datanode'): {
        'min_hosts': 1,
        'max_hosts': 32,
        'flavor_types': [('standard', 1), ('burstable', 1), ('standard', 2), ('burstable', 2), ('gpu', 3)],
    },
    ('hadoop_cluster', 'hadoop_cluster.computenode'): {
        'min_hosts': 0,
        'max_hosts': 32,
        'flavor_types': [('standard', 1), ('burstable', 1), ('standard', 2), ('burstable', 2), ('gpu', 3)],
    },
    ('elasticsearch_cluster', 'elasticsearch_cluster.datanode'): {
        'min_hosts': 1,
        'max_hosts': 7,
        'flavor_types': [('standard', 1), ('burstable', 1), ('standard', 2), ('burstable', 2)],
    },
    ('elasticsearch_cluster', 'elasticsearch_cluster.masternode'): {
        'min_hosts': 3,
        'max_hosts': 5,
        'flavor_types': [('standard', 1), ('burstable', 1), ('standard', 2), ('burstable', 2)],
    },
    ('opensearch_cluster', 'opensearch_cluster.datanode'): {
        'min_hosts': 1,
        'max_hosts': 7,
        'flavor_types': [('standard', 1), ('burstable', 1), ('standard', 2), ('burstable', 2)],
    },
    ('opensearch_cluster', 'opensearch_cluster.masternode'): {
        'min_hosts': 3,
        'max_hosts': 5,
        'flavor_types': [('standard', 1), ('burstable', 1), ('standard', 2), ('burstable', 2)],
    },
    ('kafka_cluster', 'kafka_cluster'): {
        'min_hosts': 1,
        'max_hosts': 7,
        'flavor_types': [
            ('standard', 1),
            ('burstable', 1),
            ('standard', 2),
            ('burstable', 2),
            ('aws-standard', 1),
            ('aws-standard', 2),
        ],
    },
    ('kafka_cluster', 'zk'): {
        'min_hosts': 1,
        'max_hosts': 7,
        'flavor_types': [('standard', 1), ('burstable', 1), ('standard', 2), ('burstable', 2), ('aws-standard', 1)],
    },
    ('sqlserver_cluster', 'windows_witness'): {
        'min_hosts': 1,
        'max_hosts': 1,
        'flavor_types': [('standard', 1), ('standard', 2)],
    },
}


def _find_container(context):
    api = docker.from_env()
    try:
        return api.containers.get(context.cached_metadb_container_name)
    except docker.errors.NotFound:
        return None


def _init_container(context):
    api = docker.from_env()
    if 'metadb_container' not in context:
        image, _ = api.images.build(path=REPO)
        container = api.containers.run(
            image=image.id,
            detach=True,
            auto_remove=True,
            ports={
                '5432/tcp': None,
            },
            name=context.cached_metadb_container_name,
            environment={
                'SKIPTESTS': 1,
                'NOSTOP': 1,
            },
        )
        context.metadb_container = api.containers.get(container.id)
    machine = urlparse(api.api.base_url).hostname
    if machine == 'localunixsocket':
        machine = 'localhost'
    context.metadb_dsn = '{base} host={host} port={port}'.format(
        base=context.conf.metadb.dsn_base,
        host=machine,
        port=context.metadb_container.attrs['NetworkSettings']['Ports']['5432/tcp'][0]['HostPort'],
    )


def _init_from_recipe(context):
    context.metadb_dsn = '{base} host={host} port={port}'.format(
        base=context.conf.metadb.dsn_base, **project.get_recipe_dsn_from_env()
    )


def _data_dir() -> Path:
    return project.func_tests_root() / 'data'


def _read_data_file(file_name):
    return json.loads((_data_dir() / file_name).read_text())


def _get_lastest_migration():
    migration_file_re = re.compile(r'V(?P<version>\d+)__(?P<description>.+)\.sql$')
    files = os.listdir(os.path.join(REPO, 'migrations'))
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


def _get_metadb_version(context):
    # last keyword wins
    dsn = '{dsn} user=postgres'.format(dsn=context.metadb_dsn)

    try:
        # pylint: disable=no-member
        with closing(psycopg2.connect(dsn)) as conn:
            cur = conn.cursor()
            cur.execute('SELECT max(version) from public.schema_version')
            version = cur.fetchone()[0]
            return version if version else 0
    except Exception:
        return 0


def _wait_for_migrations(context):
    deadline = time.time() + context.conf.metadb.migrations_timeout

    expected = _get_lastest_migration()

    while time.time() < deadline:
        version = _get_metadb_version(context)
        if version != expected:
            print(
                'MetaDB version {version}. Expected {expected}. '
                'Sleeping 1 sec'.format(version=version, expected=expected)
            )
            time.sleep(1)
        else:
            return

    raise RuntimeError('MetaDB migration timeout')


def _generate_flavors():
    """
    Generate flavors table contents
    """

    flavors = []

    flavor_types = _read_data_file('flavor_type.json')

    counter = 0
    base = {
        'cpu_guarantee': 1,
        'cpu_limit': 1,
        'io_limit': 20971520,
        'memory_guarantee': 4294967296,
        'memory_limit': 4294967296,
        'network_guarantee': 16777216,
        'network_limit': 16777216,
        'gpu_limit': 0,
        'io_cores_limit': 0,
    }
    for flavor_type in flavor_types:
        for i in range(5):
            for vtype in ['porto', 'compute', 'aws']:
                if flavor_type['type'] == 'aws-standard' or vtype == 'aws':
                    if flavor_type['type'] != 'aws-standard' or vtype != 'aws':
                        continue

                flavor = {k: v * (2**i) for k, v in base.items()}
                cpu_guarantee = flavor['cpu_guarantee']
                memory_guarantee = flavor['memory_guarantee']
                memory_limit = flavor['memory_limit']
                gpu_limit = flavor['gpu_limit']
                io_cores_limit = flavor['io_cores_limit']
                if flavor_type['type'] == 'memory-optimized':
                    memory_guarantee *= 2
                    memory_limit *= 2
                elif flavor_type['type'] == 'cpu-optimized':
                    memory_guarantee //= 2
                    memory_limit //= 2
                elif flavor_type['type'] == 'burstable':
                    cpu_guarantee *= 0.05
                    memory_guarantee //= 2
                    memory_limit //= 2
                elif flavor_type['type'] == 'gpu':
                    gpu_limit = 2**i
                elif flavor_type['type'] == 'io':
                    io_cores_limit = 2
                flavor.update(
                    {
                        'vtype': vtype,
                        'platform_id': 'mdb-v1',
                        'type': flavor_type['type'],
                        'generation': flavor_type['generation'],
                        'cpu_guarantee': cpu_guarantee,
                        'memory_guarantee': memory_guarantee,
                        'memory_limit': memory_limit,
                        'id': str(uuid.UUID(int=counter)),
                        'name': '{type}{gen}.{vtype}.{num}'.format(
                            type=flavor_type['type'][0], gen=flavor_type['generation'], vtype=vtype, num=i + 1
                        ),
                        'gpu_limit': gpu_limit,
                        'io_cores_limit': io_cores_limit,
                    }
                )
                flavors.append(flavor)
                counter += 1

    decomissioned_flavor = flavors[0].copy()
    decomissioned_flavor.update(
        {
            'id': str(uuid.UUID(int=counter)),
            'name': 's1.porto.legacy',
        }
    )
    flavors.append(decomissioned_flavor)
    decomissioned_flavor = flavors[0].copy()
    decomissioned_flavor.update(
        {
            'id': str(uuid.UUID(int=counter + 1)),
            'name': 's2.porto.0.legacy',  # 0 needed to force preset be first in list in resource_preset_by_cpu
            'generation': 2,
        }
    )
    flavors.append(decomissioned_flavor)

    with open(project.generated_data_root() / 'flavors.json', 'w') as out_file:
        json.dump(flavors, out_file)


def _copy_default_versions():
    """
    Put values of dbaas:default_resources to generated_data_root
    """

    try:
        # pylint: disable=import-error, import-outside-toplevel
        import yatest.common

        path = Path(yatest.common.source_path('cloud/mdb/salt/pillar/metadb_default_versions.sls'))
    except ImportError:
        path = Path(__file__).parent.parent.parent.parent / 'salt/pillar/metadb_default_versions.sls'
    with open(path, 'r') as config_file:
        data = yaml.load(config_file)['data']['dbaas_metadb']['default_versions']
    with open(project.generated_data_root() / 'default_versions.json', 'w') as out_file:
        json.dump(data, out_file)


def _copy_default_alert():
    """
    Put values of dbaas:default_alerts to generated_data_root
    """

    path = Path(
        yatest.common.source_path('cloud/mdb/dbaas-internal-api-image/tests/testdata/metadb_default_alert.yaml')
    )

    with open(path, 'r') as config_file:
        data = yaml.load(config_file)['data']['dbaas_metadb']['default_alert']
    with open(project.generated_data_root() / 'default_alert.json', 'w') as out_file:
        json.dump(data, out_file)


def _generate_valid_resources():
    """
    Generate valid resources table contents
    """
    resources = []
    with open(project.generated_data_root() / 'flavors.json') as ffd:
        flavors = json.load(ffd)

    with open(_data_dir() / 'disk_types.json') as dtfd:
        disk_types = json.load(dtfd)

    def _get_disk_type_id(ext_id: str) -> int:
        for dt in disk_types:
            if dt['disk_type_ext_id'] == ext_id:
                return dt['disk_type_id']
        raise RuntimeError(f'Disk type with {ext_id} not found in {disk_types}')

    local_ssd_id = _get_disk_type_id('local-ssd')
    network_ssd_id = _get_disk_type_id('network-ssd')
    local_nvme_id = _get_disk_type_id('local-nvme')
    network_ssd_nonreplicated_id = _get_disk_type_id('network-ssd-nonreplicated')

    geos = _read_data_file('geos.json')

    for pair, opts in CLUSTER_ROLE_DATA.items():
        cluster_type, _ = pair
        for flavor in flavors:
            min_hosts = opts['min_hosts']
            max_hosts = opts['max_hosts']

            if (flavor['type'], flavor['generation']) not in opts['flavor_types']:
                continue
            if flavor['type'] == 'burstable' and cluster_type != 'hadoop_cluster':
                min_hosts = 1
                max_hosts = 1
            for geo in geos:
                # generate AWS flavors only in AWS region
                if geo['region_id'] == 2 and flavor['vtype'] != 'aws':
                    continue

                if flavor['vtype'] == 'aws':
                    resources.append(
                        {
                            'cluster_type': pair[0],
                            'role': pair[1],
                            'disk_size_range': None,
                            'disk_sizes': [2 ** (30 + gb_power) for gb_power in range(5, 12)],
                            'default_disk_size': 2**36,
                            'disk_type_id': local_ssd_id,
                            'min_hosts': min_hosts,
                            'max_hosts': max_hosts,
                            'flavor': flavor['id'],
                            'geo_id': geo['geo_id'],
                            'id': len(resources),
                            'feature_flag': None,
                        }
                    )

                # We follow compute/porto differences here
                if flavor['vtype'] == 'compute':
                    resources.append(
                        {
                            'cluster_type': pair[0],
                            'role': pair[1],
                            'disk_size_range': {
                                'int8range': [10737418240, 2199023255552],
                            },
                            'disk_sizes': None,
                            'default_disk_size': None,
                            'disk_type_id': network_ssd_id,
                            'min_hosts': min_hosts,
                            'max_hosts': max_hosts,
                            'flavor': flavor['id'],
                            'geo_id': geo['geo_id'],
                            'id': len(resources),
                            'feature_flag': None,
                        }
                    )
                    resources.append(
                        {
                            'cluster_type': pair[0],
                            'role': pair[1],
                            'disk_size_range': {
                                'int8range': [4194304, 4199023255552],
                            },
                            'disk_sizes': None,
                            'default_disk_size': None,
                            'disk_type_id': network_ssd_id,
                            'min_hosts': min_hosts,
                            'max_hosts': max_hosts,
                            'flavor': flavor['id'],
                            'geo_id': geo['geo_id'],
                            'id': len(resources),
                            'feature_flag': 'MDB_MONGODB_EXTENDEDS',
                        }
                    )
                    resources.append(
                        {
                            'cluster_type': pair[0],
                            'role': pair[1],
                            'disk_size_range': {
                                'int8range': [10737418240, 2199023255552],
                            },
                            'disk_sizes': None,
                            'default_disk_size': None,
                            'disk_type_id': network_ssd_nonreplicated_id,
                            'min_hosts': min_hosts,
                            'max_hosts': max_hosts,
                            'flavor': flavor['id'],
                            'geo_id': geo['geo_id'],
                            'id': len(resources),
                            'feature_flag': 'MDB_ALLOW_NETWORK_SSD_NONREPLICATED',
                        }
                    )
                    if flavor['type'] != 'burstable' and pair[0] != 'hadoop_cluster':
                        min_hosts_tmp = 2 if pair[1] == 'clickhouse_cluster' else 3
                        resources.append(
                            {
                                'cluster_type': pair[0],
                                'role': pair[1],
                                'disk_size_range': None,
                                'disk_sizes': [107374182400, 214748364800],
                                'default_disk_size': None,
                                'disk_type_id': local_nvme_id,
                                'min_hosts': min_hosts_tmp,
                                'max_hosts': max(max_hosts, min_hosts_tmp),
                                'flavor': flavor['id'],
                                'geo_id': geo['geo_id'],
                                'id': len(resources),
                                'feature_flag': None,
                            }
                        )

                def is_redis_compute_standard():
                    return pair[0] == 'redis_cluster' and flavor['vtype'] == 'compute' and flavor['type'] == 'standard'

                def is_greenplum_compute_standard():
                    return (
                        pair[0] == 'greenplum_cluster'
                        and flavor['vtype'] == 'compute'
                        and flavor['type'] in ('standard', 'io')
                    )

                min_hosts_tmp = 3 if is_redis_compute_standard() else min_hosts
                if flavor['vtype'] == 'porto' or is_redis_compute_standard() or is_greenplum_compute_standard():
                    min_range = 10737418240
                    if pair[0] == 'redis_cluster' and (
                        flavor['name'] == 's1.porto.3' or flavor['name'] == 's1.compute.3'
                    ):
                        min_range = 42949672960
                    resources.append(
                        {
                            'cluster_type': pair[0],
                            'role': pair[1],
                            'disk_size_range': {
                                'int8range': [min_range, 2199023255552],
                            },
                            'disk_sizes': None,
                            'default_disk_size': None,
                            'disk_type_id': local_ssd_id,
                            'min_hosts': min_hosts_tmp,
                            'max_hosts': max_hosts,
                            'flavor': flavor['id'],
                            'geo_id': geo['geo_id'],
                            'id': len(resources),
                            'feature_flag': None,
                        }
                    )
                    resources.append(
                        {
                            'cluster_type': pair[0],
                            'role': pair[1],
                            'disk_size_range': {
                                'int8range': [4194304, 4199023255552],
                            },
                            'disk_sizes': None,
                            'default_disk_size': None,
                            'disk_type_id': local_ssd_id,
                            'min_hosts': min_hosts_tmp,
                            'max_hosts': max_hosts,
                            'flavor': flavor['id'],
                            'geo_id': geo['geo_id'],
                            'id': len(resources),
                            'feature_flag': 'MDB_MONGODB_EXTENDEDS',
                        }
                    )
    with open(project.generated_data_root() / 'valid_resources.json', 'w') as out_file:
        json.dump(resources, out_file)


def _generate_config_host_access_ids():
    """
    Generate config host access ids table contents
    """
    hasher = PasswordHasher(type=Type.I)
    ids = [
        {
            'access_id': '00000000-0000-0000-0000-000000000000',
            'access_secret': hasher.hash('dummy'),
            'active': True,
            'type': 'default',
        },
        {
            'access_id': '00000000-0000-0000-0000-000000000001',
            'access_secret': hasher.hash('dummy'),
            'active': True,
            'type': 'dbaas-worker',
        },
    ]
    with open(project.generated_data_root() / 'config_host_access_ids.json', 'w') as out_file:
        json.dump(ids, out_file)


def generate_data():
    """
    Generate table contents (only flavors and valid_resources for now)
    """

    if not project.generated_data_root().exists():
        os.makedirs(project.generated_data_root())
    _generate_flavors()
    _generate_valid_resources()
    _copy_default_versions()
    _copy_default_alert()
    _generate_config_host_access_ids()


def _populate_cmd():
    try:
        #  pylint: disable=import-outside-toplevel
        from cloud.mdb.dbaas_metadb.tests.helpers.locate import locate_populate_table

        return [locate_populate_table()]
    except ImportError:
        return [
            os.path.join(os.path.split(sys.executable)[0], 'python'),
            str(project.project_root() / REPO / 'bin' / 'populate_table.py'),
        ]


@tenacity.retry(
    retry=tenacity.retry_if_exception_type(subprocess.CalledProcessError),
    wait=tenacity.wait_random_exponential(multiplier=5, max=5),
    stop=tenacity.stop_after_attempt(6),
)
def fill_data(context):
    data = [
        {
            'table': 'dbaas.flavor_type',
            'key': 'id',
            'file': 'flavor_type.json',
        },
        {
            'table': 'dbaas.flavors',
            'key': 'name',
            'file': 'flavors.json',
            'dir': project.generated_data_root(),
        },
        {
            'table': 'dbaas.regions',
            'key': 'name',
            'file': 'regions.json',
        },
        {
            'table': 'dbaas.geo',
            'key': 'name',
            'file': 'geos.json',
        },
        {
            'table': 'dbaas.disk_type',
            'key': 'disk_type_ext_id',
            'file': 'disk_types.json',
        },
        {
            'table': 'dbaas.default_pillar',
            'key': 'id',
            'file': 'default_pillar.json',
        },
        {
            'table': 'dbaas.cluster_type_pillar',
            'key': 'type',
            'file': 'cluster_type_pillars.json',
        },
        {
            'table': 'dbaas.valid_resources',
            'key': 'id',
            'file': 'valid_resources.json',
            'dir': project.generated_data_root(),
        },
        {
            'table': 'dbaas.default_versions',
            'key': 'type,component,name,env',
            'file': 'default_versions.json',
            'dir': project.generated_data_root(),
        },
        {
            'table': 'dbaas.default_alert',
            'key': 'template_id',
            'file': 'default_alert.json',
            'dir': project.generated_data_root(),
        },
        {
            'table': 'dbaas.config_host_access_ids',
            'key': 'access_id,access_secret,active,type',
            'file': 'config_host_access_ids.json',
            'dir': project.generated_data_root(),
        },
    ]

    cmd = _populate_cmd()

    for options in data:
        path = options.get('dir', _data_dir()) / options['file']

        subprocess.check_call(
            cmd
            + [
                '-d',
                context.metadb_dsn,
                '-f',
                path,
                '-t',
                options['table'],
                '-k',
                options['key'],
            ]
        )


def prepare(context):
    """
    Metadb prepare has 4 phases:
    1) Checkout correct metadb version
    2) Build and start container
    3) Wait for migrations to complete
    4) Fill container with data
    """
    context.cached_metadb_container_name = os.getenv('CACHED_METADB_CONTAINER_NAME')
    container = _find_container(context) if context.cached_metadb_container_name else None

    if container:
        context.metadb_container = container
        _init_container(context)
    else:
        if project.is_running_with_recipe():
            _init_from_recipe(context)
        else:
            _init_container(context)
            _wait_for_migrations(context)
        persistence_config = PersistenceConfig.read()
        if persistence_config is None or 'metadb' not in persistence_config.persist_data_for_clusters:
            generate_data()
            fill_data(context)
    context.metadb_conn = psycopg2.connect(context.metadb_dsn)


def cleanup(context):
    """
    Clean data from metadb
    """
    # pylint: disable=no-member
    with context.metadb_conn as txn:
        cur = txn.cursor()
        for table in [
            'default_feature_flags',
            'idempotence',
            'worker_queue',
            'target_pillar',
            'pillar_revs',
            'hosts_revs',
            'shards_revs',
            'subclusters_revs',
            'clusters_revs',
            'clusters_changes',
            'pillar',
            'hosts',
            'shards',
            'subclusters',
            'hadoop_jobs',
            'clusters',
            'folders',
            'clouds',
            'search_queue',
        ]:
            cur.execute(
                """
                DELETE FROM dbaas.{table}
                """.format(
                    table=table
                )
            )


def teardown(context):
    """
    Shutdown metadb container
    """
    if hasattr(context, 'metadb_container') and not context.cached_metadb_container_name:

        @tenacity.retry(
            retry=tenacity.retry_if_exception_type(docker.errors.DockerException),
            wait=tenacity.wait_random_exponential(multiplier=5, max=5),
            stop=tenacity.stop_after_attempt(3),
        )
        def _stop_meta():
            context.metadb_container.stop(timeout=1)
            context.metadb_container = None

        _stop_meta()

    for name in ['flavors', 'valid_resources']:
        try:
            os.remove(_data_dir() / '{name}.json'.format(name=name))
        except OSError as exc:
            if exc.errno != errno.ENOENT:
                raise
