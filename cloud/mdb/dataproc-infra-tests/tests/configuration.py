"""
Variables that influence testing behavior are defined here.
"""
# pylint: disable=too-many-lines

import json
import logging
import os
import pickle
import random
import string
from getpass import getuser

import argon2
import OpenSSL
import yaml
from deepdiff import DeepDiff
from nacl.encoding import URLSafeBase64Encoder as encoder
from nacl.public import PrivateKey

from tests.helpers import iam, vault
from tests.helpers.pillar import DATAPROC_PUBLIC_PORT, MDB_INT_API_PORT
from tests.helpers.utils import fix_rel_path, get_inner_dict, merge
from tests.local_config import CONF_OVERRIDE


def password_generate(length=20):
    letters = string.ascii_lowercase
    return ''.join(random.choice(letters) for i in range(length))


def build(state_file_path):
    """
    Build full config: join static, dynamic and secrets
    """
    config = _load_static()
    _apply_dynamic(config, state_file_path)
    _init_secrets(config)
    return config


def _load_static():
    """
    Validate that static config is idempotent
    """
    config = static()
    reloaded_config = static()
    if config != reloaded_config:
        dd = DeepDiff(config, reloaded_config, ignore_order=True, report_repetition=True, view='tree')
        raise Exception(f"Static config is not idempotent: {dd}")
    return config


def static():
    """
    Get static configuration (idempotent between subsequent calls if no config file is changed)
    """
    # Options for External Driver
    compute_driver = CONF_OVERRIDE.get('compute_driver')
    if os.getenv('DRIVER_FQDN'):
        compute_driver['fqdn'] = os.getenv('DRIVER_FQDN')
    fqdn = compute_driver.get('fqdn')

    if os.getenv('ZONE'):
        compute_driver['resources']['zone'] = os.getenv('ZONE')

    s3_config = CONF_OVERRIDE['s3']

    dataproc_test_data = CONF_OVERRIDE.get('test_dataproc', {})
    # Forwarded ssh public keys to data-plane
    ssh_public_keys = [dataproc_test_data.get('sshPublicKeys')]

    managed_test_data = CONF_OVERRIDE.get('test_managed', {})
    kubernetes_test_data = CONF_OVERRIDE.get('test_kubernetes', {})
    redis_test_data = CONF_OVERRIDE.get('test_redis', {})

    sqlserver_test_data = CONF_OVERRIDE.get('test_sqlserver', {})

    config = {
        # Common conf options.
        # See below for dynamic stuff (keys, certs, etc)
        'user': getuser(),
        'common': CONF_OVERRIDE.get('common', {}),
        'dynamic': {
            'clouds': {
                'test': {
                    'cloud_ext_id': dataproc_test_data['cloudId'],
                },
            },
            'folders': {
                'test': {
                    'cloud_ext_id': dataproc_test_data['cloudId'],
                    'folder_id': 1,
                    'folder_ext_id': dataproc_test_data['folderId'],
                },
            },
        },
        # Controls whether to perform cleanup after tests execution or not.
        'cleanup': True,
        # At slow matches override it local_configuration
        'retries_multiplier': 1,
        # Code checkout
        # Where does all the fun happens.
        # Assumption is that it can be safely rm-rf`ed later.
        'staging_dir': 'staging',
        'saved_context_path': os.path.abspath('staging/saved_context.pkl'),
        'repos': {},
        'compute_driver': {
            'fqdn': fqdn,
            'dataplane_folder_id': dataproc_test_data['folderId'],
            'network_id': compute_driver['network_id'],
            'resources': compute_driver.get('resources', {}),
            'compute': CONF_OVERRIDE.get('compute', {}),
            'ssh': CONF_OVERRIDE.get('ssh'),
            'dns': CONF_OVERRIDE.get('dns'),
            'ssh_keys': {
                'dataplane': {
                    'private': '/root/.ssh/id_dataplane',
                    'public': '/root/.ssh/id_dataplane.pub',
                },
            },
        },
        'ssh': {'pkey_path': '/root/.ssh/id_dataplane', 'pkey_password': ''},
        'deploy_api': {
            'fqdn': fqdn,
            'port': 8443,
            'url': f'https://{fqdn}:8443',
            'group': 'infratest',
            'token': 'testtoken',
        },
        's3': {
            'endpoint_url': s3_config['endpoint_url'],
            'region_name': s3_config['region_name'],
        },
        'put_to_context': {
            'defaultSubnetId': dataproc_test_data.get('subnetId'),
        },
        'test_cluster_configs': {
            'hadoop': {
                'standard': {
                    'serviceAccountId': CONF_OVERRIDE['put_to_context']['serviceAccountId1'],
                    'bucket': CONF_OVERRIDE['put_to_context']['outputBucket1'],
                    'zoneId': dataproc_test_data.get('zoneId'),
                    'labels': {
                        'owner': getuser(),
                    },
                    'configSpec': {
                        'versionId': '2.0',
                        'hadoop': {
                            'services': ['HDFS', 'YARN', 'SPARK', 'TEZ', 'HIVE', 'MAPREDUCE'],
                            'properties': {
                                'yarn:yarn.nm.liveness-monitor.expiry-interval-ms': 15000,
                                'yarn:yarn.log-aggregation-enable': 'false',
                            },
                            'sshPublicKeys': ssh_public_keys,
                        },
                        'subclustersSpec': [
                            {
                                'name': 'main',
                                'role': 'MASTERNODE',
                                'resources': dataproc_test_data.get(
                                    'resources',
                                    {
                                        'resourcePresetId': 's2.micro',
                                        'diskTypeId': 'network-hdd',
                                        'diskSize': 16106127360,
                                    },
                                ),
                                'subnetId': dataproc_test_data.get('subnetId'),
                            },
                            {
                                'name': 'data',
                                'role': 'DATANODE',
                                'resources': dataproc_test_data.get(
                                    'resources',
                                    {
                                        'resourcePresetId': 's2.micro',
                                        'diskTypeId': 'network-hdd',
                                        'diskSize': 16106127360,
                                    },
                                ),
                                'hostsCount': 1,
                                'subnetId': dataproc_test_data.get('subnetId'),
                            },
                        ],
                    },
                },
                'autoscaling': {
                    'serviceAccountId': CONF_OVERRIDE['put_to_context']['serviceAccountId2'],
                    'bucket': CONF_OVERRIDE['put_to_context']['outputBucket1'],
                    'zoneId': dataproc_test_data.get('zoneId'),
                    'securityGroupIds': [dataproc_test_data.get('securityGroupId')],
                    'labels': {
                        'owner': getuser(),
                    },
                    'configSpec': {
                        'versionId': '2.0',
                        'hadoop': {
                            'services': ['HDFS', 'YARN', 'SPARK', 'TEZ', 'HIVE', 'MAPREDUCE'],
                            'properties': {
                                'yarn:yarn.nm.liveness-monitor.expiry-interval-ms': 15000,
                                'yarn:yarn.log-aggregation-enable': 'false',
                            },
                            'sshPublicKeys': ssh_public_keys,
                        },
                        'subclustersSpec': [
                            {
                                'name': 'main',
                                'role': 'MASTERNODE',
                                'resources': dataproc_test_data.get(
                                    'resources',
                                    {
                                        'resourcePresetId': 's2.micro',
                                        'diskTypeId': 'network-hdd',
                                        'diskSize': 16106127360,
                                    },
                                ),
                                'subnetId': dataproc_test_data.get('subnetId'),
                            },
                            {
                                'name': 'data',
                                'role': 'DATANODE',
                                'resources': dataproc_test_data.get(
                                    'resources',
                                    {
                                        'resourcePresetId': 's2.micro',
                                        'diskTypeId': 'network-hdd',
                                        'diskSize': 16106127360,
                                    },
                                ),
                                'hostsCount': 1,
                                'subnetId': dataproc_test_data.get('subnetId'),
                            },
                            {
                                'name': 'compute-autoscaling',
                                'role': 'COMPUTENODE',
                                'resources': dataproc_test_data.get(
                                    'resources',
                                    {
                                        'resourcePresetId': 's2.micro',
                                        'diskTypeId': 'network-hdd',
                                        'diskSize': 16106127360,
                                    },
                                ),
                                'hostsCount': 1,
                                'autoscalingConfig': {
                                    'maxHostsCount': 2,
                                },
                                'subnetId': dataproc_test_data.get('subnetId'),
                            },
                        ],
                    },
                },
                'benchmark': {
                    'serviceAccountId': dataproc_test_data.get('serviceAccountId'),
                    'serviceAccountIdToChange': dataproc_test_data.get('serviceAccountIdToChange'),
                    'bucket': s3_config['bucket_name'],
                    'zoneId': dataproc_test_data.get('zoneId'),
                    'labels': {
                        'owner': getuser(),
                    },
                    'configSpec': {
                        'versionId': '1.1',
                        'hadoop': {
                            'services': ['HDFS', 'YARN', 'MAPREDUCE'],
                            'sshPublicKeys': ssh_public_keys,
                        },
                        'subclustersSpec': [
                            {
                                'name': 'main',
                                'role': 'MASTERNODE',
                                'resources': {
                                    'resourcePresetId': 's2.medium',
                                    'diskTypeId': 'network-hdd',
                                    'diskSize': 161061273600,
                                },
                                'subnetId': dataproc_test_data.get('subnetId'),
                            },
                            {
                                'name': 'data',
                                'role': 'DATANODE',
                                'resources': {
                                    'resourcePresetId': 's2.medium',
                                    'diskTypeId': 'network-hdd',
                                    'diskSize': 161061273600,
                                },
                                'hostsCount': 3,
                                'subnetId': dataproc_test_data.get('subnetId'),
                            },
                        ],
                    },
                },
            },
            'metastore': {
                'standard': {
                    "folder_id": kubernetes_test_data['folderId'],
                    "name": "test",
                    "description": "metastore infratest cluster",
                    "labels": {
                        "foo": "bar",
                    },
                    "min_servers_per_zone": 1,
                    "subnet_ids": kubernetes_test_data['user_subnet_ids'],
                },
            },
            'kafka': {
                'standart': {
                    'folder_id': managed_test_data['folderId'],
                    'network_id': managed_test_data['networkId'],
                    'subnet_id': managed_test_data['subnetIds'],
                    'name': 'TestCluster',
                    'environment': 2,
                    'configSpec': {
                        'brokersCount': 2,
                        'zoneId': [managed_test_data.get('zoneId')],
                        'kafka': {
                            'resources': {
                                'resourcePresetId': 's2.micro',
                                'diskTypeId': 'network-hdd',
                                'diskSize': 40 * 2**30,
                            },
                            'kafka_config_3_1': {
                                'log_flush_interval_ms': 60 * 1000,
                                'log_retention_hours': 48,
                                'message_max_bytes': 500000,
                                'replica_fetch_max_bytes': 2000000,
                                'offsets_retention_minutes': 10000,
                            },
                        },
                        'version': '3.1',
                    },
                    'topic_specs': [
                        {
                            'name': 'topic1',
                            'partitions': 3,
                            'replication_factor': 1,
                            'topic_config_3_1': {
                                'retention_bytes': 2 * 1024 * 1024 * 1024,
                                'retention_ms': 48 * 3600 * 1000,
                                'cleanup_policy': 1,
                                'compression_type': 4,
                                'flush_ms': 60 * 1000,
                            },
                        },
                        {
                            'name': 'topic2',
                            'partitions': 3,
                            'replication_factor': 1,
                        },
                    ],
                    'user_specs': [
                        {
                            'name': 'producer',
                            'password': 'producer-password',
                            'permissions': [
                                {
                                    'topic_name': 'topic1',
                                    'role': 1,
                                }
                            ],
                        },
                        {
                            'name': 'consumer',
                            'password': 'consumer-password',
                            'permissions': [
                                {
                                    'topic_name': 'topic1',
                                    'role': 2,
                                }
                            ],
                        },
                        {
                            'name': 'admin',
                            'password': 'admin-password',
                            'permissions': [
                                {
                                    'topic_name': '*',
                                    'role': 3,
                                }
                            ],
                        },
                    ],
                    'security_group_ids': [managed_test_data.get('securityGroupId')],
                },
                'kafka28': {
                    'folder_id': managed_test_data['folderId'],
                    'network_id': managed_test_data['networkId'],
                    'subnet_id': managed_test_data['subnetIds'],
                    'name': 'TestCluster',
                    'environment': 2,
                    'configSpec': {
                        'version': '2.8',
                        'brokersCount': 1,
                        'zoneId': ['ru-central1-a', 'ru-central1-b', 'ru-central1-c'],
                        'kafka': {
                            'resources': {
                                'resourcePresetId': 's2.micro',
                                'diskTypeId': 'network-hdd',
                                'diskSize': 40 * 2**30,
                            },
                            'kafka_config_2_8': {
                                'log_flush_interval_ms': 60 * 1000,
                            },
                        },
                    },
                    'topic_specs': [
                        {
                            'name': 'topic1',
                            'partitions': 6,
                            'replication_factor': 3,
                            'topic_config_2_8': {
                                'flush_ms': 60 * 1000,
                            },
                        },
                    ],
                    'user_specs': [
                        {
                            'name': 'admin',
                            'password': 'admin-password',
                            'permissions': [
                                {
                                    'topic_name': '*',
                                    'role': 3,
                                }
                            ],
                        },
                    ],
                },
                'standalone': {
                    'folder_id': managed_test_data['folderId'],
                    'network_id': managed_test_data['networkId'],
                    'subnet_id': managed_test_data['subnetIds'],
                    'name': 'TestCluster',
                    'environment': 2,
                    'configSpec': {
                        'brokersCount': 1,
                        'zoneId': [managed_test_data.get('zoneId')],
                        'kafka': {
                            'resources': {
                                'resourcePresetId': 's2.micro',
                                'diskTypeId': 'network-hdd',
                                'diskSize': 40 * 2**30,
                            },
                            'kafka_config_3_1': {
                                'log_flush_interval_ms': 60 * 1000,
                                'log_retention_hours': 48,
                            },
                        },
                        'version': '3.1',
                    },
                    'topic_specs': [
                        {
                            'name': 'topic1',
                            'partitions': 3,
                            'replication_factor': 1,
                        },
                        {
                            'name': 'topic3',
                            'partitions': 3,
                            'replication_factor': 1,
                        },
                    ],
                    'user_specs': [
                        {
                            'name': 'test',
                            'password': 'test-password',
                            'permissions': [
                                {
                                    'topic_name': 'topic1',
                                    'role': 1,
                                },
                                {
                                    'topic_name': 'topic1',
                                    'role': 2,
                                },
                            ],
                        },
                        {
                            'name': 'test-update-connector-user',
                            'password': 'test-update-connector-user-password',
                            'permissions': [
                                {
                                    'topic_name': 'topic3',
                                    'role': 1,
                                },
                                {
                                    'topic_name': 'topic3',
                                    'role': 2,
                                },
                            ],
                        },
                    ],
                    'security_group_ids': [managed_test_data.get('securityGroupId')],
                },
            },
            'greenplum': {
                'standard': {
                    "folder_id": managed_test_data['folderId'],
                    "environment": "PRESTABLE",
                    "name": "test",
                    "description": "dataproc infratest cluster",
                    "labels": {
                        "foo": "bar",
                    },
                    "network_id": managed_test_data['networkId'],
                    "config": {
                        "version": "6.19",
                        "zone_id": "ru-central1-c",
                        "subnet_id": "",
                        "assign_public_ip": False,
                    },
                    "master_config": {
                        "resources": {
                            "resourcePresetId": "s2.micro",
                            "diskTypeId": "network-ssd",
                            "diskSize": 17179869184,
                        },
                    },
                    "segment_config": {
                        "resources": {
                            "resourcePresetId": "s2.micro",
                            "diskTypeId": "network-ssd",
                            "diskSize": 17179869184,
                        },
                    },
                    "master_host_count": 2,
                    "segment_in_host": 1,
                    "segment_host_count": 2,
                    "user_name": "usr1",
                    "user_password": "Pa$$w0rd",
                },
            },
            'redis': {
                'standard': {
                    'environment': 'PRESTABLE',
                    'tlsEnabled': True,
                    'configSpec': {
                        'version': '6.2',
                        'redisConfig_6_2': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'hm2.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [
                        {
                            'zoneId': redis_test_data.get('zoneId'),
                        }
                    ],
                    'networkId': managed_test_data['networkId'],
                },
            },
            'sqlserver': {
                'standard': {
                    'folder_id': sqlserver_test_data['folderId'],
                    'network_id': sqlserver_test_data['networkId'],
                    'name': 'TestCluster',
                    'environment': 2,
                    'configSpec': {
                        'version': '2016sp2std',
                        'sqlserver_config_2016sp2std': {},
                        'resources': {
                            'resourcePresetId': 's2.small',
                            'diskTypeId': 'network-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'userSpecs': [
                        {
                            'name': 'test',
                            'password': 'test_password1!',
                            'permissions': [
                                {
                                    'database_name': 'testdb',
                                    'roles': ['DB_OWNER'],
                                }
                            ],
                            'server_roles': ['MDB_MONITOR'],
                        }
                    ],
                    "databaseSpecs": [
                        {
                            "name": "testdb",
                        }
                    ],
                    "hostSpecs": [
                        {
                            "zoneId": sqlserver_test_data.get('zoneId')[0],
                            "subnetId": sqlserver_test_data.get('subnetId')[0],
                        }
                    ],
                },
                'enterprise': {
                    'folder_id': sqlserver_test_data['folderId'],
                    'network_id': sqlserver_test_data['networkId'],
                    'name': 'TestCluster',
                    'environment': 2,
                    'configSpec': {
                        'version': '2016sp2ent',
                        'secondary_connections': 'SECONDARY_CONNECTIONS_READ_ONLY',
                        'sqlserver_config_2016sp2ent': {},
                        'resources': {
                            'resourcePresetId': 's2.small',
                            'diskTypeId': 'network-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'userSpecs': [
                        {
                            'name': 'test',
                            'password': 'test_password1!',
                            'permissions': [
                                {
                                    'database_name': 'testdb',
                                    'roles': ['DB_OWNER'],
                                }
                            ],
                            'server_roles': ['MDB_MONITOR'],
                        }
                    ],
                    'databaseSpecs': [
                        {
                            'name': 'testdb',
                        }
                    ],
                    'hostSpecs': [
                        {
                            'zoneId': sqlserver_test_data.get('zoneId')[0],
                            'subnetId': sqlserver_test_data.get('subnetId')[0],
                        },
                        {
                            'zoneId': sqlserver_test_data.get('zoneId')[1],
                            'subnetId': sqlserver_test_data.get('subnetId')[1],
                        },
                        {
                            'zoneId': sqlserver_test_data.get('zoneId')[2],
                            'subnetId': sqlserver_test_data.get('subnetId')[2],
                        },
                    ],
                },
            },
        },
        'test_subcluster_config': {
            'hadoop': {
                'computenode': {
                    'name': 'compute_subcluster',
                    'role': 'COMPUTENODE',
                    'resources': dataproc_test_data.get(
                        'resources',
                        {
                            'resourcePresetId': 's2.micro',
                            'diskTypeId': 'network-hdd',
                            'diskSize': 16106127360,
                        },
                    ),
                    'hostsCount': 2,
                    'subnetId': dataproc_test_data.get('subnetId'),
                },
                'datanode': {
                    'name': 'data_subcluster',
                    'role': 'DATANODE',
                    'resources': dataproc_test_data.get(
                        'resources',
                        {
                            'resourcePresetId': 's2.micro',
                            'diskTypeId': 'network-hdd',
                            'diskSize': 16106127360,
                        },
                    ),
                    'hostsCount': 2,
                    'subnetId': dataproc_test_data.get('subnetId'),
                },
            },
        },
        'template_params': {
            'folder_ext_id': dataproc_test_data['folderId'],
            'subnet_id': dataproc_test_data['subnetId'],
            'product_id': dataproc_test_data['productId'],
        },
        # A dict with all projects that are going to interact in this
        # testing environment.
        'projects': {
            # Basically this mimics docker-compose 'service'.
            # Matching keys will be used in docker-compose,
            # while others will be ignored in compose file, but may be
            # referenced in any other place.
            'base': {
                # The base needs to be present so templates,
                # if any, will be rendered.
                # It is brewed by docker directly,
                # and not used in compose environment.
            },
            'internal_api': {
                'port': compute_driver['port'],
                'url': f'https://{fqdn}:{compute_driver["port"]}',
            },
            'control_plane_logging': {
                'url': dataproc_test_data.get('logging_url'),
                'log_group_id': dataproc_test_data.get('control_plane_log_group_id'),
                'service_account_key': dataproc_test_data.get('control_plane_logging_sa_key'),
            },
            'mdb_internal_api': {
                'port': MDB_INT_API_PORT,
                'host': fqdn,
                'url': f'{fqdn}:{MDB_INT_API_PORT}',
                'grpc_timeout': 30,
                'vpc': {
                    'token': '',
                },
            },
            'metadb': {
                'dbname': 'dbaas_metadb',
                'host': fqdn,
                'user': 'dbaas_api',
                'port': 6432,
                'access_ids': {
                    'ext_pillar': 'd14b0eb6-013f-4a35-a871-3047e57a464f',
                    'worker': 'dfdc9997-0f2b-442e-bdd3-4628ed8a1319',
                    'manager': '63d9bb70-56ce-438a-984b-a36352c15d24',
                    'dataproc-ui-proxy': '2860bed0-5841-456f-81fd-aa816c1e7e77',
                },
            },
            'redis': {
                'host': fqdn,
            },
            'saltapi': {},
            'deploydb': {
                'dbname': 'deploydb',
                'host': fqdn,
                'user': 'deploy_api',
                'port': 6432,
            },
            'tvmtool': {
                'id': '',
                'secret': '',
                'token': '',
            },
            'worker': {
                'ssh_key': '',
                'certificator_token': '',
                'ssh': {
                    'public': '',
                    'private': '',
                },
            },
            'salt_master': {
                'keys': {
                    'public': '',
                    'private': '',
                },
            },
            'nginx': {
                'dhparam': '',
            },
            'mdb_deploy_api': {
                'token': '',
            },
            'mdb_secrets': {
                'oauth': '',
                'private': '',
                'public': '',
            },
            'certificator_mock': {},
        },
        'dataproc_agent_config_override': {
            'jobs': {
                's3logger': {
                    'endpoint': s3_config['endpoint_url'],
                    'region_name': s3_config['region_name'],
                    'bucket_name': s3_config['bucket_name'],
                },
            },
        },
    }
    if getuser() != 'robot-pgaas-ci':
        config['projects']['mdb_internal_api']['port'] = MDB_INT_API_PORT
        config['projects']['mdb_internal_api']['host'] = fqdn
        config['projects']['mdb_internal_api']['url'] = f'{fqdn}:{MDB_INT_API_PORT}'
    return merge(config, CONF_OVERRIDE)


def _apply_dynamic(config, state_file_path):
    """
    Load dynamic config from state file and override main config with dynamic values.
    If there's no dynamic config within state file then generate one and save it to state file.
    """
    try:
        with open(state_file_path, 'rb') as state_file:
            state = pickle.load(state_file)
    except FileNotFoundError:
        state = {}

    dynamic = state.get('config')

    if dynamic is None or not ('compute_driver' in dynamic and 'ident' in dynamic['compute_driver']):
        logging.info('Generating dynamic config values')
        dynamic = _init_dynamic(config)
        state['config'] = dynamic
        with open(state_file_path, 'wb') as state_file:
            pickle.dump(state, state_file)

    logging.info('Applying dynamic config values')
    merge(config, dynamic)


def _init_dynamic(config):
    """
    Build dynamic config
    """
    dynamic = {}

    set_generated_value(config, dynamic, 'projects.metadb.password', password_generate())
    set_generated_value(config, dynamic, 'projects.redis.password', password_generate())
    set_generated_value(config, dynamic, 'projects.deploydb.password', password_generate())

    ident = ''.join(random.choices(string.ascii_lowercase + string.digits, k=6))
    fqdn = f"dataproc-{ident}.infratest.db.yandex.net"
    set_generated_value(config, dynamic, 'compute_driver.ident', ident)
    set_generated_value(config, dynamic, 'compute_driver.fqdn', fqdn)
    set_generated_value(config, dynamic, 'deploy_api.fqdn', fqdn)
    set_generated_value(config, dynamic, 'deploy_api.url', fqdn + ':' + str(config['deploy_api']['port']))
    set_generated_value(config, dynamic, 'projects.metadb.host', fqdn)
    set_generated_value(config, dynamic, 'projects.redis.host', fqdn)
    set_generated_value(config, dynamic, 'projects.deploydb.host', fqdn)
    set_generated_value(config, dynamic, 'projects.mdb_internal_api.host', fqdn)
    intapi_port = config['projects']['mdb_internal_api']['port']
    set_generated_value(config, dynamic, 'projects.mdb_internal_api.url', fqdn + ':' + str(intapi_port))

    pillar_key = PrivateKey.generate()
    key = {
        'public': encoder.encode(pillar_key.public_key.encode()).decode('utf-8'),
        'private': encoder.encode(pillar_key.encode()).decode('utf-8'),
    }
    dynamic['pillar_key'] = key

    ph = argon2.PasswordHasher(memory_cost=512, parallelism=2)
    pwd = password_generate()
    set_generated_value(config, dynamic, 'projects.metadb.access_secret', pwd)
    set_generated_value(config, dynamic, 'projects.metadb.access_hash', ph.hash(pwd))

    key = OpenSSL.crypto.PKey()
    key.generate_key(OpenSSL.crypto.TYPE_RSA, 2048)
    key_pem = OpenSSL.crypto.dump_privatekey(OpenSSL.crypto.FILETYPE_PEM, key)
    pubkey_pem = OpenSSL.crypto.dump_publickey(OpenSSL.crypto.FILETYPE_PEM, key)
    set_generated_value(config, dynamic, 'projects.salt_master.tls.private', key_pem.decode())
    set_generated_value(config, dynamic, 'projects.salt_master.tls.public', pubkey_pem.decode())

    return dynamic


def set_generated_value(config, dynamic, path, value):
    """
    Fill dynamic config attribute if it is not filled within main config
    """
    path = path.split('.')
    key = path[-1]
    path = '.'.join(path[:-1])
    parent = get_inner_dict(config, path)
    if parent.get(key) is None:
        get_inner_dict(dynamic, path)[key] = value


def _init_secrets(config):
    """
    Populate config with secrets from ENV and Vault
    """
    common_config = config['common']
    if os.getenv('YAV_OAUTH'):
        common_config['yav_oauth'] = os.getenv('YAV_OAUTH')

    yav = vault.YandexVault(oauth=common_config.get('yav_oauth'), ya_command=fix_rel_path(common_config['ya_command']))

    common_config['cert'] = yav.get(common_config.get('cert_secret_id'))
    common_config['docker_registry'] = yav.get(common_config.get('docker_registry_secret_id'))
    common_config['cr'] = yav.get(common_config.get('cr_secret_id'))
    common_config['pguser_postgres'] = yav.get(common_config.get('pguser_postgres_secret_id'))['password']

    config['projects']['control_plane_logging']['service_account_key'] = json.loads(
        yav.get(common_config.get('cp_logging_sa_key_secret_id'))['key']
    )

    compute_config = config['compute_driver']['compute']
    sa_key = yav.get(compute_config['sa_secret_id'])
    config['compute_driver']['service_account'] = {
        'id': sa_key['service_account_id'],
        'key_id': sa_key['id'],
        'private_key': sa_key['private_key'],
    }
    compute_config['token'] = iam.get_iam_token(sa_key, verify=compute_config['ca_path'])
    dns_config = config['compute_driver']['dns']
    if os.getenv('DNS_TOKEN'):
        dns_config['token'] = os.getenv('DNS_TOKEN')
    elif dns_config.get('token'):
        pass
    elif dns_config.get('token_secret_id'):
        dns_config['token'] = yav.get(dns_config.get('token_secret_id'))['token']

    s3_config = get_inner_dict(config, 's3')
    if not s3_config.get('access_key'):
        sa_key = yav.get(compute_config.get('sa_secret_id'))
        s3_config['access_key'] = sa_key['access_key_id']
        s3_config['secret_key'] = sa_key['access_key_secret']

    backup_secret_id = yav.get(s3_config.get('backup_secret_id'))
    if 'id' in backup_secret_id and 'key' in backup_secret_id:
        s3_config['backup']['host'] = backup_secret_id['host']
        s3_config['backup']['endpoint'] = 'https://' + backup_secret_id['host']
        s3_config['backup']['access_key_id'] = backup_secret_id['id']
        s3_config['backup']['access_secret_key'] = backup_secret_id['key']

    solomon_cloud_secret = yav.get(config.get('solomon_cloud_secret'))
    solomon_keys = ['service_account_id', 'private_key', 'key_id']
    have_all_keys = True
    for key in solomon_keys:
        if key not in solomon_cloud_secret:
            have_all_keys = False
    if have_all_keys:
        config['solomon_cloud'] = {
            'sa_id': solomon_cloud_secret['service_account_id'],
            'sa_private_key': solomon_cloud_secret['private_key'],
            'sa_key_id': solomon_cloud_secret['key_id'],
            'push_url': '',
            'token': '',
            'ca_path': '',
        }
    else:
        config['solomon_cloud'] = {
            'sa_id': 'trash',
            'sa_private_key': 'trash',
            'sa_key_id': 'trash',
            'push_url': '',
            'token': '',
            'ca_path': '',
        }

    saltapi_config = config['projects']['saltapi']
    salt_pass = yav.get(common_config.get('saltapi_password_secret_id'))
    saltapi_config['password'] = salt_pass['password']
    saltapi_config['password_hash'] = salt_pass['password_hash']

    tvmtool_config = config['projects']['tvmtool']
    tvmtool_secret = yav.get(common_config.get('tvmtool_secret_id'))
    tvmtool_config['id'] = tvmtool_secret['id']
    tvmtool_config['secret'] = tvmtool_secret['secret']
    tvmtool_config['token'] = tvmtool_secret['token']

    nginx_config = config['projects']['nginx']
    dhparam_secret = yav.get(common_config.get('dhparam_secret_id'))
    nginx_config['dhparam'] = dhparam_secret['private']

    certificator_mock_config = config['projects']['certificator_mock']
    certificator_secret = yav.get(common_config.get('certificator_secret_id'))
    certificator_mock_config['private'] = certificator_secret['private']
    certificator_mock_config['cert'] = certificator_secret['cert']
    certificator_mock_config['crl'] = certificator_secret['crl']

    worker_config = config['projects']['worker']
    worker_dummmy_key = yav.get(common_config.get('worker_dummmy_key_id'))
    worker_config['ssh']['public'] = worker_dummmy_key['public']
    worker_config['ssh']['private'] = worker_dummmy_key['private']


def build_managed_secrets(state, conf, **_):
    common_config = conf['common']
    yav = vault.YandexVault(oauth=common_config.get('yav_oauth'), ya_command=common_config['ya_command'])

    mdb_internal_api_config = conf['projects']['mdb_internal_api']
    compute_token = yav.get(common_config.get('compute_token_secret_id'))
    mdb_internal_api_config['vpc']['token'] = compute_token['token']

    mdb_secrets_config = conf['projects']['mdb_secrets']
    mdb_secrets_config['oauth'] = ''
    mdb_secrets_config['private'] = ''
    mdb_secrets_config['public'] = ''


def generate_yc_config(state, conf, **_):
    compute_config = conf['compute_driver']['compute']
    common_config = conf['common']
    yav = vault.YandexVault(oauth=common_config['yav_oauth'], ya_command=common_config['ya_command'])
    sa_key = yav.get(compute_config.get('sa_secret_id'))
    dataproc_user_sa_key = yav.get(compute_config.get('dataproc_user_sa_secret_id'))
    config = {
        'current': 'default',
        'profiles': {
            'default': {
                'endpoint': f"{conf['compute_driver']['fqdn']}:{DATAPROC_PUBLIC_PORT}",
                'folder-id': conf['compute_driver']['dataplane_folder_id'],
                'service-account-key': {
                    'id': sa_key['id'],
                    'service_account_id': sa_key['service_account_id'],
                    'key_algorithm': 'RSA_2048',
                    'public_key': sa_key['public_key'],
                    'private_key': sa_key['private_key'],
                },
            },
            'dataproc-user-sa': {
                'endpoint': f"{conf['compute_driver']['fqdn']}:{DATAPROC_PUBLIC_PORT}",
                'folder-id': conf['compute_driver']['dataplane_folder_id'],
                'service-account-key': json.loads(dataproc_user_sa_key['key']),
            },
        },
    }
    with open('staging/yc_config.yaml', 'w') as file:
        yaml.dump(config, file)


def set_managed_flag(state, conf, **_):
    state.setdefault('persistent', {})['managed'] = True


def put_infratest_ca(state, conf, **_):
    certificator_mock_config = conf['projects']['certificator_mock']
    with open('./staging/infratest.pem', 'w') as f:
        f.write(certificator_mock_config['cert'])
