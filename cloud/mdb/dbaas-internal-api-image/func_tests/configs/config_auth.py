"""
Config with mocked auth provider (use for functional tests)
"""
import os

from tests.providers import (
    ComputeQuotaServiceMock,
    ConfigAuthProvider,
    ConfigResourceManager,
    ConfigYCComputeProvider,
    ConfigYCIdentityProvider,
    ConfigYCNetworkProvider,
    DataprocManagerMock,
    DummyConfigAuthProvider,
    DummyLoggingService,
    DummyLoggingReadService,
    DummyURIValidator,
    DummyYCComputeBillingProvider,
    DummyVersionValidator,
    FileS3Api,
    IamClientMock,
    MDBHealthProviderFromFile,
    MockedCryptoProvider,
    SequenceClusterSecretsProvider,
    SequenceHostnameGenerator,
    SequenceIdGenerator,
)

# pylint: disable=invalid-name

TMP_ROOT = os.environ['TMP_ROOT']
LOGS_ROOT = os.environ['LOGS_ROOT']

AUTH_PROVIDER = ConfigAuthProvider
CRYPTO_PROVIDER = MockedCryptoProvider
MDBH_PROVIDER = MDBHealthProviderFromFile
IDENTITY_PROVIDER = ConfigYCIdentityProvider
NETWORK_PROVIDER = ConfigYCNetworkProvider
ID_GENERATOR = SequenceIdGenerator
HOSTNAME_GENERATOR = SequenceHostnameGenerator
S3_PROVIDER = FileS3Api
CONFIG_AUTH_PROVIDER = DummyConfigAuthProvider
CLUSTER_SECRETS_PROVIDER = SequenceClusterSecretsProvider
COMPUTE_BILLING_PROVIDER = DummyYCComputeBillingProvider
RESOURCE_MANAGER = ConfigResourceManager
COMPUTE_QUOTA_PROVIDER = ComputeQuotaServiceMock
COMPUTE_PROVIDER = ConfigYCComputeProvider
LOGGING_SERVICE = DummyLoggingService
LOGGING_READ_SERVICE = DummyLoggingReadService

MOCKED_AUTH = {
    'ro-token': {
        'user_id': 'read-only-user',
        'user_type': 'user_account',
        'folders': {
            'folder1': {
                'read_only': True,
            },
        },
        'clouds': ['cloud1'],
        'service_accounts': ['sa1', 'sa2'],
    },
    'rw-token': {
        'user_id': 'user',
        'user_type': 'user_account',
        'folders': {
            'folder1': {
                'read_only': False,
                'force_delete': False,
            },
            'folder2': {
                'read_only': False,
                'force_delete': False,
            },
            'folder4': {
                'read_only': False,
                'force_delete': False,
            },
        },
        'clouds': ['cloud1', 'cloud2'],
        'service_accounts': [
            'sa1',
            'sa2',
            'service_account_1',
            'service_account_2',
            'service_account_with_permission',
            'service_account_without_permission',
            'service_account_editor',
        ],
    },
    'rw-service-token': {
        'user_id': 'service-user',
        'user_type': 'service',
        'folders': {
            'read_only': False,
            'force_delete': False,
        },
        'clouds': [
            'cloud1',
        ],
    },
    'logging-service-account-token': {
        'user_id': 'service-user',
        'user_type': 'service',
        'folders': {
            'folder1': {
                'read_only': False,
                'force_delete': False,
                'logging.records.write': True,
            },
            'folder2': {
                'read_only': False,
                'force_delete': False,
                'logging.records.write': False,
            },
        },
        'clouds': [
            'cloud1',
        ],
    },
    'resource-reaper-service-token': {
        'user_id': 'resource-reaper',
        'user_type': 'service',
        'folders': {
            'folder1': {
                'read_only': False,
                'force_delete': True,
            }
        },
        'clouds': ['cloud1', 'cloud2'],
    },
    'rw-e2e-token': {
        'user_id': 'e2e-user',
        'user_type': 'user_account',
        'folders': {
            'folder3': {
                'read_only': False,
                'force_delete': False,
            },
        },
        'clouds': ['cloud3'],
    },
}

E2E = {'folder_id': 'folder3', 'cluster_name': 'dbaas_e2e_func_tests'}

IDENTITY = {
    'override_folder': None,
    'override_cloud': None,
    'create_missing': True,
    'allow_move_between_clouds': True,
}

CONFIG_YC_IDENTITY = {
    'map': {
        'folder1': 'cloud1',
        'folder2': 'cloud2',
        'folder3': 'cloud3',
        'folder4': 'cloud1',
    },
    'status_path': f'{TMP_ROOT}/cloud-status.json',
    'feature_flags_path': f'{TMP_ROOT}/cloud-feature-flags.json',
}

NETWORK = {
    'network1': {
        'vla': {'network1-vla': 'folder1'},
        'sas': {'network1-sas': 'folder1'},
        'myt': {'network1-myt': 'folder1'},
        'iva': {'network1-iva': 'folder1'},
        # No instances in man are possible with this
        'man': {},
    },
    'network2': {
        'vla': {'network2-vla': 'folder1'},
        'sas': {'network2-sas': 'folder1'},
        'myt': {'network2-myt': 'folder1'},
        # No instances in man are possible with this
        'man': {},
        # Instances in iva require explicit subnet
        'iva': {'network2-iva': 'folder1', 'network2-iva2': 'folder2'},
    },
    'unresolved_networks': 'IN-PORTO-NO-NETWORK-API',
    'security_groups': {
        'sg_id1': {'id': 'sg_id1', 'network_id': 'network1', 'folder_id': 'folder1'},
        'sg_id2': {'id': 'sg_id2', 'network_id': 'network1', 'folder_id': 'folder1'},
        'sg_id3': {'id': 'sg_id3', 'network_id': 'network1', 'folder_id': 'folder1'},
        'sg_id4': {'id': 'sg_id4', 'network_id': 'network1', 'folder_id': 'folder1'},
        'sg_net2_id1': {'id': 'sg_net2_id1', 'network_id': 'network2', 'folder_id': 'folder2'},
        # security group with blocked internal traffic
        'sg_id5': {'id': 'sg_id5', 'network_id': 'network1', 'folder_id': 'folder1', 'rules': [{}]},
        # security group with allowed ssh/22 traffic
        'sg_id6': {
            'id': 'sg_id6',
            'network_id': 'network1',
            'folder_id': 'folder1',
            'rules': [
                {'protocol_name': 'TCP', 'ports_to': 22, 'description': 'openssh/22'},
            ],
        },
        # 0-65536 rule from UI
        'sg_id7': {
            'id': 'sg_id7',
            'network_id': 'network1',
            'folder_id': 'folder1',
            'rules': [
                {
                    'protocol_name': 'TCP',
                    'ports_from': 0,
                    'ports_to': 65535,
                    'predefined_target': 'self_security_group',
                },
            ],
        },
        # security group with allowed internal traffic without port range
        'sg_id8': {
            'id': 'sg_id7',
            'network_id': 'network1',
            'folder_id': 'folder1',
            'rules': [
                {
                    'protocol_name': 'TCP',
                    'ports_from': None,
                    'ports_to': None,
                    'predefined_target': 'self_security_group',
                },
            ],
        },
    },
    'subnets': {
        'network1-myt-with-nat': {
            'id': 'network1-myt-with-nat',
            'name': 'network1-myt-with-nat',
            'networkId': 'network1',
            'zoneId': 'myt',
            'v4CidrBlock': ['192.168.0.0/16'],
            'egressNatEnable': True,
        },
        'network1-myt-without-nat': {
            'id': 'network1-myt-without-nat',
            'name': 'network1-myt-without-nat',
            'networkId': 'network1',
            'zoneId': 'myt',
            'v4CidrBlock': ['192.168.0.0/16'],
            'egressNatEnable': False,
        },
    },
}

COMPUTE_GRPC = {
    'url': 'http://',
    'host_groups': {
        'hg_id1': {
            'id': 'hg_id1',
            'name': 'host group 1',
            'folder_id': 'folder1',
            'zone_id': 'myt',
            'host_type_id': "ht1",
        },
        'hg_id2': {
            'id': 'hg_id2',
            'name': 'host group 2',
            'folder_id': 'folder1',
            'zone_id': 'vla',
            'host_type_id': "ht1",
        },
        'hg_id3': {
            'id': 'hg_id3',
            'name': 'host group 3',
            'folder_id': 'folder2',
            'zone_id': 'sas',
            'host_type_id': "ht1",
        },
    },
    'host_types': {
        'ht1': {'id': 'ht1', 'cores': 16, 'memory': 4294967296, 'disks': 4, 'disk_size': 10737418240},
    },
}

METADB = {
    'hosts': [os.getenv('METADB_HOST')],
    'user': 'dbaas_api',
    'password': 'dbaas_api',
    'dbname': 'dbaas_metadb',
    'port': os.getenv('METADB_PORT'),
    'maxconn': 10,
    'minconn': 5,
    'sslmode': 'allow',
    'connect_timeout': 1,
}

SEQUENCE_PATH = f'{TMP_ROOT}'

MDBHEALTH = {
    'path': f'{TMP_ROOT}/mdb-health.json',
}

LOGCONFIG = {
    'version': 1,
    'disable_existing_loggers': True,
    'formatters': {
        'raw': {
            'format': '%(asctime)s [%(levelname)s] %(name)s:\t%(message)s',
        },
        'tskv': {
            '()': 'dbaas_internal_api.core.logs.TSKVFormatter',
            'tskv_format': 'dbaas-internal-api',
        },
    },
    'handlers': {
        'file': {
            'class': 'logging.FileHandler',
            'level': 'DEBUG',
            'formatter': 'raw',
            'filename': f'{LOGS_ROOT}/internal-api.log',
        },
        'tskv': {
            'class': 'logging.FileHandler',
            'level': 'DEBUG',
            'formatter': 'tskv',
            'filename': f'{LOGS_ROOT}/internal-api.tskv',
        },
    },
    'loggers': {
        'dbaas-internal-api-request': {
            'handlers': ['file', 'tskv'],
            'level': 'DEBUG',
        },
        'dbaas-internal-api-background': {
            'handlers': ['file', 'tskv'],
            'level': 'DEBUG',
        },
        'clickhouse_driver': {
            'handlers': ['file', 'tskv'],
            'level': 'WARNING',
        },
    },
}

ENVCONFIG = {
    'mongodb_cluster': {
        'dev': {
            'zk_hosts': ['localhost'],
        },
        'qa': {
            'zk_hosts': ['localhost'],
        },
    },
    'postgresql_cluster': {
        'dev': {
            'zk': {
                'test': ['localhost'],
            },
        },
        'qa': {
            'zk': {
                'test': ['localhost'],
            },
        },
        'prod': {
            'zk': {
                'test': ['localhost'],
            },
        },
    },
    'mysql_cluster': {
        'dev': {
            'zk': {
                'test': ['localhost'],
            },
        },
        'qa': {
            'zk': {
                'test': ['localhost'],
            },
        },
        'prod': {
            'zk': {
                'test': ['localhost'],
            },
        },
    },
    'redis_cluster': {
        'dev': {
            'zk_hosts': ['localhost'],
        },
        'qa': {
            'zk_hosts': ['localhost'],
        },
    },
}

CLOSE_PATH = f'{TMP_ROOT}/close'
READ_ONLY_FLAG = f'{TMP_ROOT}/read-only'

DEFAULT_CLOUD_QUOTA = {
    'clusters_quota': 2,
    'cpu_quota': 2 * 3 * 4,
    'gpu_quota': 0,
    'memory_quota': 2 * 3 * 4 * 4294967296,
    'ssd_space_quota': 2 * 3 * 107374182400,
    'hdd_space_quota': 2 * 3 * 107374182400,
}

CTYPE_CONFIG = {
    'clickhouse_cluster': {
        'zk': {
            'flavor': 's1.porto.1',
            'volume_size': 10737418240,
            'disk_type_id': 'local-ssd',
            'node_count': 3,
        },
        'shard_count_limit': 3,
    },
    'mongodb_cluster': {
        'mongocfg': {
            'flavor': 's1.porto.1',
            'volume_size': 10737418240,
            'disk_type_id': 'local-ssd',
            'node_count': 3,
        },
        'shard_count_limit': 3,
    },
}

MINIMAL_DISK_UNIT = 4194304

DECOMMISSIONING_ZONES = [
    'ugr',
    'eu-central-1a',
    'eu-central-1b',
    'eu-central-1c',
    'us-east-2a',
    'us-east-2b',
    'us-east-2c',
]
DECOMMISSIONING_FLAVORS = ['s1.porto.legacy', 's2.porto.0.legacy']

RESOURCE_MANAGER_CONFIG = {
    'service_account_1': ['dataproc.agent'],
    'service_account_3': ['dataproc.agent'],
    'service_account_with_permission': ['dataproc.agent'],
    'service_account_without_permission': [],
    'service_account_editor': ['editor', 'dataproc.agent', 'vpc.user', 'dns.editor'],
}

DATAPROC_MANAGER_CONFIG = {
    'class': DataprocManagerMock,
    'url': 'test-dataproc-manager-private-url',
    'health_path': f'{TMP_ROOT}/dataproc_manager_cluster_health.json',
}

DATAPROC_MANAGER_PUBLIC_CONFIG = {
    'manager_url': 'test-dataproc-manager-public-url',
}

EXTERNAL_URI_VALIDATION = {
    'regexp': r'https://(?:[a-zA-Z0-9-]+\.)?storage\.yandexcloud\.net/\S+',
    'validator': DummyURIValidator,
}

VERSION_VALIDATOR = {
    'validator': DummyVersionValidator,
    'validator_config': {
        'packages_uri': 'http://mdb-bionic-secure.dist.yandex.ru/mdb-bionic-secure/stable/all/Packages.gz',
    },
}

RESTFUL_JSON = {'indent': 4, 'sort_keys': True}

S3_PROVIDER_RESPONSE_FILE = f'{TMP_ROOT}/s3response.json'

# test versions
VERSIONS = {
    'clickhouse_cluster': [
        {
            'version': '21.1.9.41',
            'name': '21.1',
            'deprecated': True,
        },
        {
            'version': '21.2.10.48',
            'name': '21.2',
            'downgradable': False,
        },
        {
            'version': '21.3.8.76',
            'name': '21.3 LTS',
            'default': True,
        },
        {
            'version': '21.4.5.46',
            'name': '21.4',
        },
        {
            'version': '21.7.2.7',
            'name': '21.7',
        },
        {
            'version': '21.8.15.7',
            'name': '21.8 LTS',
        },
        {
            'version': '21.11.5.33',
            'name': '21.11',
        },
        {
            'version': '22.1.4.30',
            'name': '22.1',
        },
        {
            'version': '22.3.2.2',
            'name': '22.3 LTS',
        },
        {
            'version': '22.5.1.2079',
            'name': '22.5',
        },
    ],
    'redis_cluster': [
        {
            'version': '5.0',
            'default': True,
            'deprecated': True,
            'allow_deprecated_feature_flag': 'MDB_REDIS_ALLOW_DEPRECATED_5',
        },
        {
            'version': '6.0',
            'deprecated': True,
            'allow_deprecated_feature_flag': 'MDB_REDIS_ALLOW_DEPRECATED_6',
        },
        {
            'version': '6.2',
            'feature_flag': 'MDB_REDIS_62',
        },
        {
            'version': '7.0',
            'feature_flag': 'MDB_REDIS_70',
        },
    ],
    'mongodb_cluster': {
        '306': '30623',
        '400': '40023',
        '402': '40212',
        '404': '40404',
        '500': '50002',
    },
    # old compatibility versions
    'hadoop_cluster': [
        {
            'version': '2.0',
        },
        {
            'version': '1.4',
        },
        {
            'version': '1.3',
        },
    ],
}

PUBLIC_IMAGES_FOLDER = 'aoen0gulfkoaicqsvt1i'

HADOOP_DEFAULT_VERSION_PREFIX = '2.0'
HADOOP_IMAGES = {
    '2.0.0': {
        'name': '2.0.0',
        'version': '2.0.0',
        'description': 'image with monitoring',
        'imageMinSize': 16106127360,
        'supported_pillar': '1.1.0',
        'services': {
            'hdfs': {
                'version': '2.8.5',
                'default': True,
            },
            'yarn': {
                'version': '2.8.5',
                'default': True,
                'deps': ['hdfs'],
            },
            'mapreduce': {
                'version': '2.8.5',
                'default': True,
                'deps': ['yarn'],
            },
            'tez': {
                'version': '0.9.1',
                'default': True,
                'deps': ['yarn'],
            },
            'zookeeper': {
                'version': '3.4.6',
                'default': True,
            },
            'hbase': {
                'version': '1.3.3',
                'default': True,
                'deps': ['zookeeper', 'hdfs', 'yarn'],
            },
            'hive': {
                'version': '2.3.4',
                'default': True,
                'deps': ['yarn'],
            },
            'sqoop': {
                'version': '1.4.6',
                'default': True,
            },
            'flume': {
                'version': '1.8.0',
                'default': True,
            },
            'spark': {
                'version': '2.2.1',
                'default': True,
                'deps': ['yarn', 'hdfs'],
            },
            'zeppelin': {
                'version': '0.7.3',
                'default': True,
            },
            'oozie': {
                'version': '4.3.1',
                'default': False,
                'deps': ['zookeeper'],
            },
            'livy': {
                'version': '0.7.0',
                'default': False,
                'deps': ['spark'],
            },
        },
        'roles_services': {
            'hadoop_cluster.masternode': [
                'hdfs',
                'yarn',
                'mapreduce',
                'zookeeper',
                'hbase',
                'hive',
                'sqoop',
                'spark',
                'zeppelin',
                'oozie',
                'livy',
            ],
            'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'spark', 'flume'],
            'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez', 'flume'],
        },
    },
    '1.4.0': {
        'default': True,
        'name': '1.4.0',
        'version': '1.4.0',
        'description': 'image with monitoring',
        'imageMinSize': 16106127360,
        'supported_pillar': '1.1.0',
        'services': {
            'hdfs': {
                'version': '2.8.5',
                'default': True,
            },
            'yarn': {
                'version': '2.8.5',
                'default': True,
                'deps': ['hdfs'],
            },
            'mapreduce': {
                'version': '2.8.5',
                'default': True,
                'deps': ['yarn'],
            },
            'tez': {
                'version': '0.9.1',
                'default': True,
                'deps': ['yarn'],
            },
            'zookeeper': {
                'version': '3.4.6',
                'default': True,
            },
            'hbase': {
                'version': '1.3.3',
                'default': True,
                'deps': ['zookeeper', 'hdfs', 'yarn'],
            },
            'hive': {
                'version': '2.3.4',
                'default': True,
                'deps': ['yarn'],
            },
            'sqoop': {
                'version': '1.4.6',
                'default': True,
            },
            'flume': {
                'version': '1.8.0',
                'default': True,
            },
            'spark': {
                'version': '2.2.1',
                'default': True,
                'deps': ['yarn', 'hdfs'],
            },
            'zeppelin': {
                'version': '0.7.3',
                'default': True,
            },
            'oozie': {
                'version': '4.3.1',
                'default': False,
                'deps': ['zookeeper'],
            },
            'livy': {
                'version': '0.7.0',
                'default': False,
                'deps': ['spark'],
            },
        },
        'roles_services': {
            'hadoop_cluster.masternode': [
                'hdfs',
                'yarn',
                'mapreduce',
                'zookeeper',
                'hbase',
                'hive',
                'sqoop',
                'spark',
                'zeppelin',
                'oozie',
                'livy',
            ],
            'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'spark', 'flume'],
            'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez', 'flume'],
        },
    },
    '1.3.0': {
        'name': '1.3.0',
        'version': '1.3.0',
        'deprecated': True,
        'allow_deprecated_feature_flag': 'MDB_DATAPROC_ALLOW_DEPRECATED_VERSIONS',
        'description': 'image with apache livy',
        'imageMinSize': 16106127360,
        'supported_pillar': '1.1.0',
        'services': {
            'hdfs': {
                'version': '2.8.5',
                'default': True,
            },
            'yarn': {
                'version': '2.8.5',
                'default': True,
                'deps': ['hdfs'],
            },
            'mapreduce': {
                'version': '2.8.5',
                'default': True,
                'deps': ['yarn'],
            },
            'tez': {
                'version': '0.9.1',
                'default': True,
                'deps': ['yarn'],
            },
            'zookeeper': {
                'version': '3.4.6',
                'default': True,
            },
            'hbase': {
                'version': '1.3.3',
                'default': True,
                'deps': ['zookeeper', 'hdfs', 'yarn'],
            },
            'hive': {
                'version': '2.3.4',
                'default': True,
                'deps': ['yarn'],
            },
            'sqoop': {
                'version': '1.4.6',
                'default': True,
            },
            'flume': {
                'version': '1.8.0',
                'default': True,
            },
            'spark': {
                'version': '2.2.1',
                'default': True,
                'deps': ['yarn', 'hdfs'],
            },
            'zeppelin': {
                'version': '0.7.3',
                'default': True,
            },
            'oozie': {
                'version': '4.3.1',
                'default': False,
                'deps': ['zookeeper'],
            },
            'livy': {
                'version': '0.7.0',
                'default': False,
                'deps': ['spark'],
            },
        },
        'roles_services': {
            'hadoop_cluster.masternode': [
                'hdfs',
                'yarn',
                'mapreduce',
                'zookeeper',
                'hbase',
                'hive',
                'sqoop',
                'spark',
                'zeppelin',
                'oozie',
                'livy',
            ],
            'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'spark', 'flume'],
            'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez', 'flume'],
        },
    },
    '99.0.0': {
        'name': '99.0.0',
        'version': '99.0.0',
        'feature_flag': 'MDB_DATAPROC_IMAGE_99',
        'description': 'image with monitoring',
        'imageMinSize': 16106127360,
        'supported_pillar': '1.1.0',
        'services': {
            'hdfs': {
                'version': '2.8.5',
                'default': True,
            },
            'yarn': {
                'version': '2.8.5',
                'default': True,
                'deps': ['hdfs'],
            },
            'mapreduce': {
                'version': '2.8.5',
                'default': True,
                'deps': ['yarn'],
            },
            'tez': {
                'version': '0.9.1',
                'default': True,
                'deps': ['yarn'],
            },
            'zookeeper': {
                'version': '3.4.6',
                'default': True,
            },
            'hbase': {
                'version': '1.3.3',
                'default': True,
                'deps': ['zookeeper', 'hdfs', 'yarn'],
            },
            'hive': {
                'version': '2.3.4',
                'default': True,
                'deps': ['yarn'],
            },
            'sqoop': {
                'version': '1.4.6',
                'default': True,
            },
            'flume': {
                'version': '1.8.0',
                'default': True,
            },
            'spark': {
                'version': '2.2.1',
                'default': True,
                'deps': ['yarn', 'hdfs'],
            },
            'zeppelin': {
                'version': '0.7.3',
                'default': True,
            },
            'oozie': {
                'version': '4.3.1',
                'default': False,
                'deps': ['zookeeper'],
            },
            'livy': {
                'version': '0.7.0',
                'default': False,
                'deps': ['spark'],
            },
        },
        'roles_services': {
            'hadoop_cluster.masternode': [
                'hdfs',
                'yarn',
                'mapreduce',
                'zookeeper',
                'hbase',
                'hive',
                'sqoop',
                'spark',
                'zeppelin',
                'oozie',
                'livy',
            ],
            'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'spark', 'flume'],
            'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez', 'flume'],
        },
    },
}

MINIMAL_ZK_RESOURCES = [
    {
        "ch_subcluster_cpu": 16,
        "zk_host_cpu": 2,
    },
    {
        "ch_subcluster_cpu": 40,
        "zk_host_cpu": 4,
    },
    {
        "ch_subcluster_cpu": 160,
        "zk_host_cpu": 8,
    },
]

IAM_TOKEN_CONFIG = {
    'class': IamClientMock,
    'url': 'test-dataproc-manager-private-url',
}
