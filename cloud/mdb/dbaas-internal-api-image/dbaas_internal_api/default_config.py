"""
DBaaS Internal API Config
"""

from typing import Dict, List

from .apis.config_auth import MetadbConfigAuthProvider
from .core.auth import AccessServiceAuthProvider
from .core.crypto import NaCLCryptoProvider
from .core.id_generators import RandomHostnameGenerator, YCIDGenerator
from .health.health import MDBHealthProviderHTTP
from .utils.iam_jwt import YCIamJwtProvider
from .utils.iam_token.client import IAMClient
from .utils.cluster_secrets import RSAClusterSecretsProvider
from .utils.compute import YCComputeProvider
from .utils.compute_billing import YCComputeBillingProvider
from .utils.compute_quota import ComputeQuota
from .utils.dataproc_manager.client import DataprocManagerClient
from .utils.dataproc_joblog.client import DataprocS3JobLogClient
from .utils.identity import GRPCIdentityProvider
from .utils.logging_service import LoggingService
from .utils.logging_read_service import LoggingReadService
from .utils.network import MetaNetworkProvider
from .utils.resource_manager import ResourceManagerGRPC
from .utils.s3 import BotoS3Api
from .utils.validation import URIValidator
from .utils.version import VersionValidator
from .utils.types import DTYPE_LOCAL_SSD, DTYPE_NETWORK_SSD, VTYPE_COMPUTE

# pylint: disable=too-many-lines

APISPEC_SPEC = dict(
    title='MDB API',
    info={
        'description': 'Yandex Managed Databases API specification',
    },
    version='1.0',
    openapi_version='3.0.2',
)

INTERNAL_SCHEMA_FIELDS_EXPOSE = False

# pylint: disable=invalid-name
AUTH_PROVIDER = AccessServiceAuthProvider

COMPUTE_QUOTA_PROVIDER = ComputeQuota

# pylint: disable=invalid-name
CRYPTO_PROVIDER = NaCLCryptoProvider

# pylint: disable=invalid-name
MDBH_PROVIDER = MDBHealthProviderHTTP

# pylint: disable=invalid-name
ID_GENERATOR = YCIDGenerator

YC_ID_PREFIX = 'mdb'

# pylint: disable=invalid-name
HOSTNAME_GENERATOR = RandomHostnameGenerator

IDENTITY_PROVIDER = GRPCIdentityProvider

S3_PROVIDER = BotoS3Api

CONFIG_AUTH_PROVIDER = MetadbConfigAuthProvider

CLUSTER_SECRETS_PROVIDER = RSAClusterSecretsProvider

RSA_CLUSTER_SECRETS = {
    'exponent': 65537,
    'key_size': 2048,
}

DISPENSER_USED = False

CRYPTO = {
    'api_secret_key': 'SECRET',
    'client_public_key': 'PUBLIC',
}

YC_ACCESS_SERVICE = {
    'endpoint': 'my-cool-yc-as-endpoint',
    'ca_path': '/my/cool/ca/path',
    'timeout': 1,
}

# This option should be used only on "special" environment
# for overriding auth checks for admin/support users
IDENTITY = {
    'override_folder': None,
    'override_cloud': None,
    'create_missing': False,
    'allow_move_between_clouds': False,
}

YC_IDENTITY = {
    'base_url': 'https://my.cool/identity/url',
    'connect_timeout': 1,
    'read_timeout': 2,
    'token': 'my secret SA token',
    'ca_path': '/my/ca/path',
    'cache_ttl': 60,
    'cache_size': 16384,
}

SENTRY = {
    'dsn': None,
    'environment': 'test',
}

TRACING = {
    'service_name': 'python-internal-api',
    'disabled': False,
    'local_agent': {
        'reporting_host': 'localhost',
        'reporting_port': '6831',
    },
    'queue_size': 10000,
    'sampler': {
        'type': 'const',
        'param': 1,
    },
    'logging': False,
}

DEFAULT_CLOUD_QUOTA = {
    'clusters_quota': 2,
    'cpu_quota': 2 * 3 * 16,
    'gpu_quota': 0,
    'memory_quota': 2 * 3 * 68719476736,
    'ssd_space_quota': 2 * 3 * 858993459200,
    'hdd_space_quota': 2 * 3 * 858993459200,
}

QUOTA_COEFFICIENTS = {
    "io_per_mem_gb": 5 * 1048576,
    "network_per_mem_gb": 4 * 1048576,
}

EXPOSE_ALL_TASK_ERRORS = False

CLOSE_PATH = '/tmp/.dbaas-internal-api-close'  # noqa
READ_ONLY_FLAG = '/tmp/.dbaas-internal-api-read-only'  # noqa

METADB = {
    'hosts': [
        'pgaas_metadb1_1.pgaas_test_net',
        'pgaas_metadb2_1.pgaas_test_net',
        'pgaas_metadb3_1.pgaas_test_net',
    ],
    'user': 'dbaas_api',
    'password': 'dbaas_api_password',
    'dbname': 'dbaas_metadb',
    'port': 6432,
    'maxconn': 10,
    'minconn': 5,
    'sslmode': 'verify-full',
    'connect_timeout': 1,
}

MDBHEALTH = {
    'url': 'https://health.db_test_net',
    'connect_timeout': 1,
    'read_timeout': 2,
    'ca_certs': 'my_ca_path',
}

LOGCONFIG_REQUESTS_LOGGER = 'dbaas-internal-api-request'
LOGCONFIG_BACKGROUND_LOGGER = 'dbaas-internal-api-background'

LOGCONFIG = {
    'version': 1,
    'disable_existing_loggers': True,
    'formatters': {
        'tskv': {
            '()': 'dbaas_internal_api.core.logs.TSKVFormatter',
            'tskv_format': 'dbaas-internal-api',
        },
    },
    'handlers': {
        'tskv': {
            'class': 'logging.handlers.RotatingFileHandler',
            'level': 'DEBUG',
            'formatter': 'tskv',
            'maxBytes': 10485760,
            'filename': '/var/log/dbaas-internal-api/api.log',
            'backupCount': 10,
        },
    },
    'loggers': {
        LOGCONFIG_REQUESTS_LOGGER: {
            'handlers': ['tskv'],
            'level': 'DEBUG',
        },
        LOGCONFIG_BACKGROUND_LOGGER: {
            'handlers': ['tskv'],
            'level': 'DEBUG',
        },
        'clickhouse_driver': {
            'handlers': ['tskv'],
            'level': 'WARNING',
        },
    },
}

ENVCONFIG = {
    'postgresql_cluster': {
        'dev': {
            'zk': {
                'test': [
                    'zkeeper-test01e.db.yandex.net:2181',
                    'zkeeper-test01h.db.yandex.net:2181',
                    'zkeeper-test01f.db.yandex.net:2181',
                ],
            },
        },
    },
    'clickhouse_cluster': {
        'dev': {},
    },
    'mongodb_cluster': {
        'dev': {
            'zk_hosts': [
                'zkeeper-test01e.db.yandex.net:2181',
                'zkeeper-test01h.db.yandex.net:2181',
                'zkeeper-test01f.db.yandex.net:2181',
            ],
        },
    },
    'redis_cluster': {
        'dev': {},
    },
    'mysql_cluster': {
        'dev': {
            'zk': {
                'test': [
                    'zkeeper-test01e.db.yandex.net:2181',
                    'zkeeper-test01h.db.yandex.net:2181',
                    'zkeeper-test01f.db.yandex.net:2181',
                ],
            },
        },
    },
}

ENV_MAPPING = {
    'qa': 'prestable',
    'prod': 'production',
}

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
    ],
    'redis_cluster': [
        {
            'version': '5.0',
            'default': True,
        },
    ],
    # old compatibility versions
    'hadoop_cluster': [
        {
            'version': '2.0',
        },
        {
            'version': '1.4',
        },
    ],
}

# This template used in moment of initial cluster creation
DEFAULT_PILLAR_TEMPLATE = {
    'postgresql_cluster': {
        'data': {
            'pgbouncer': {
                'custom_user_pool': True,
                'custom_user_params': [],
            },
            'config': {
                'pgusers': {
                    'repl': {
                        'create': True,
                        'bouncer': False,
                        'allow_db': '*',
                        'superuser': False,
                        'allow_port': '*',
                        'replication': True,
                        'settings': {},
                    },
                    'admin': {
                        'create': True,
                        'bouncer': False,
                        'allow_db': '*',
                        'superuser': True,
                        'allow_port': '*',
                        'connect_dbs': ['*'],
                        'replication': False,
                        'settings': {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod",
                        },
                    },
                    'postgres': {
                        'create': True,
                        'bouncer': False,
                        'allow_db': '*',
                        'superuser': True,
                        'allow_port': '*',
                        'connect_dbs': ['*'],
                        'replication': True,
                        'settings': {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "lock_timeout": 0,
                            "log_statement": "mod",
                            "synchronous_commit": "local",
                            "temp_file_limit": -1,
                        },
                    },
                    'monitor': {
                        'create': True,
                        'bouncer': True,
                        'allow_db': '*',
                        'superuser': False,
                        'allow_port': '*',
                        'connect_dbs': ['*'],
                        'replication': False,
                        'settings': {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod",
                        },
                    },
                    'mdb_admin': {
                        'create': True,
                        'bouncer': False,
                        'superuser': False,
                        'replication': False,
                        'login': False,
                        'conn_limit': 0,
                    },
                    'mdb_replication': {
                        'create': True,
                        'bouncer': False,
                        'superuser': False,
                        'replication': False,
                        'login': False,
                        'conn_limit': 0,
                    },
                    'mdb_monitor': {
                        'create': True,
                        'bouncer': False,
                        'superuser': False,
                        'replication': False,
                        'login': False,
                        'conn_limit': 0,
                    },
                },
                'pgbouncer': {
                    'override_pool_mode': {},
                },
            },
            'pgsync': {},
            'unmanaged_dbs': [],
            'use_wale': False,
            'use_walg': True,
        },
    },
    'clickhouse_cluster': {},
    'mongodb_cluster': {
        'data': {
            'mongodb': {
                'cluster_name': None,
                'config': {},
                'databases': {},
                'shards': {},
                'cluster_auth': 'keyfile',
                'users': {
                    'admin': {
                        'services': ['mongod', 'mongos', 'mongocfg'],
                        'dbs': {
                            'admin': ['root', 'dbOwner', 'mdbInternalAdmin'],
                            'local': ['dbOwner'],
                            'config': ['dbOwner'],
                        },
                        'internal': True,
                        'password': None,
                    },
                    'monitor': {
                        'services': ['mongod', 'mongos', 'mongocfg'],
                        'dbs': {
                            'admin': ['clusterMonitor', 'mdbInternalMonitor'],
                            'mdb_internal': ['readWrite'],
                        },
                        'internal': True,
                        'password': None,
                    },
                },
            },
        },
    },
    'redis_cluster': {
        'data': {
            'redis': {
                'config': {},
            },
        },
    },
    'mysql_cluster': {
        'data': {
            'mysql': {
                'config': {},
                'users': {
                    'admin': {
                        'dbs': {
                            '*': ['ALL PRIVILEGES'],
                        },
                        'password': None,
                        'hosts': '__cluster__',
                        'connection_limits': {
                            'MAX_USER_CONNECTIONS': 30,
                        },
                    },
                    'monitor': {
                        'dbs': {
                            '*': ['REPLICATION CLIENT'],
                            'mysql': ['SELECT'],
                        },
                        'password': None,
                        'hosts': '__cluster__',
                        'connection_limits': {
                            'MAX_USER_CONNECTIONS': 10,
                        },
                    },
                    'repl': {
                        'dbs': {
                            '*': ['REPLICATION SLAVE'],
                        },
                        'password': None,
                        'hosts': '__cluster__',
                        'connection_limits': {
                            'MAX_USER_CONNECTIONS': 10,
                        },
                    },
                },
                'databases': [],
            },
        },
    },
}

DEFAULT_SUBCLUSTER_PILLAR_TEMPLATE = {
    'mongod_subcluster': {
        'data': {
            'mongodb': {
                'use_mongod': True,
            },
        },
    },
    'mongos_subcluster': {
        'data': {
            'mongodb': {
                'use_mongos': True,
                'use_mongod': False,
            },
        },
    },
    'mongocfg_subcluster': {
        'data': {
            'mongodb': {
                'use_mongocfg': True,
                'use_mongod': False,
            },
        },
    },
    'mongoinfra_subcluster': {
        'data': {
            'mongodb': {
                'use_mongos': True,
                'use_mongod': False,
                'use_mongocfg': True,
            },
        },
    },
}

# This template is used for new user creation
DEFAULT_USER_PILLAR_TEMPLATE = {
    'postgresql_cluster': {
        'create': True,
        'grants': [],
        'bouncer': True,
        'allow_db': '*',
        'superuser': False,
        'allow_port': 6432,
        'conn_limit': 10,
        'connect_dbs': [],
        'replication': False,
    },
    'mongodb_cluster': {
        'services': ['mongod', 'mongos'],
    },
    'mysql_cluster': {},
}

CREATE_AUTH_USER = {
    'postgresql_cluster': False,
}

VTYPES = {
    'porto': 'db.yandex.net',
    'compute': 'df.cloud.yandex.net',
}

ZONE_RENAME_MAP = {
    'ru-central1-a': 'rc1a',
    'ru-central1-b': 'rc1b',
    'ru-central1-c': 'rc1c',
}

DEFAULT_DISK_TYPE_IDS = {
    'compute': DTYPE_NETWORK_SSD,
    'porto': DTYPE_LOCAL_SSD,
}

PASSWORD_LENGTH = 128

CONSOLE_ADDRESS = 'https://console-preprod.cloud.yandex.ru'

GENERATION_NAMES = {
    1: 'Haswell',
    2: 'Broadwell',
    3: 'Cascade Lake',
}

CONSOLE_DEFAULT_RESOURCES = {
    'postgresql_cluster': {
        'postgresql_cluster': {
            'generation': 2,
            'resource_preset_id': 's2.nano',
            'disk_type_id': DTYPE_LOCAL_SSD,
            'disk_size': 10737418240,
        },
    },
    'clickhouse_cluster': {
        'clickhouse_cluster': {
            'generation': 2,
            'resource_preset_id': 's2.nano',
            'disk_type_id': DTYPE_LOCAL_SSD,
            'disk_size': 10737418240,
        },
        'zk': {
            'generation': 2,
            'resource_preset_id': 's2.nano',
            'disk_type_id': DTYPE_LOCAL_SSD,
            'disk_size': 10737418240,
        },
    },
    'mongodb_cluster': {
        'mongodb_cluster.mongod': {
            'generation': 2,
            'resource_preset_id': 's2.nano',
            'disk_type_id': DTYPE_LOCAL_SSD,
            'disk_size': 10737418240,
        },
        'mongodb_cluster.mongos': {
            'generation': 2,
            'resource_preset_id': 's2.nano',
            'disk_type_id': DTYPE_LOCAL_SSD,
            'disk_size': 10737418240,
        },
        'mongodb_cluster.mongocfg': {
            'generation': 2,
            'resource_preset_id': 's2.nano',
            'disk_type_id': DTYPE_LOCAL_SSD,
            'disk_size': 10737418240,
        },
        'mongodb_cluster.mongoinfra': {
            'generation': 2,
            'resource_preset_id': 's2.nano',
            'disk_type_id': DTYPE_LOCAL_SSD,
            'disk_size': 10737418240,
        },
    },
    'mysql_cluster': {
        'mysql_cluster': {
            'generation': 2,
            'resource_preset_id': 's2.nano',
            'disk_type_id': DTYPE_LOCAL_SSD,
            'disk_size': 10737418240,
        },
    },
    'redis_cluster': {
        'redis_cluster': {
            'generation': 2,
            'resource_preset_id': 'm2.nano',
            'disk_type_id': DTYPE_LOCAL_SSD,
            'disk_size': 17179869184,
        },
    },
    'hadoop_cluster': {
        'hadoop_cluster.masternode': {
            'generation': 2,
            'resource_preset_id': 's2.nano',
            'disk_type_id': DTYPE_LOCAL_SSD,
            'disk_size': 10737418240,
        },
        'hadoop_cluster.datanode': {
            'generation': 2,
            'resource_preset_id': 's2.nano',
            'disk_type_id': DTYPE_LOCAL_SSD,
            'disk_size': 10737418240,
        },
        'hadoop_cluster.computenode': {
            'generation': 2,
            'resource_preset_id': 's2.nano',
            'disk_type_id': DTYPE_LOCAL_SSD,
            'disk_size': 10737418240,
        },
    },
}

CHARTS = {
    'postgresql_cluster': [
        (
            'YASM',
            'YaSM (Golovan) charts',
            'https://yasm.yandex-team.ru/template/panel/dbaas_postgres_metrics/cid={cid};dbname={dbname}',
        ),
        (
            'Solomon',
            'Solomon charts',
            'https://solomon.yandex-team.ru/?cluster=mdb_{cid}'
            '&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-postgres',
        ),
        (
            'Console',
            'Console charts',
            '{console}/folders/{folderId}' '/mdb/cluster/{cluster_type}/{cid}?section=monitoring',
        ),
    ],
    'clickhouse_cluster': [
        (
            'YASM',
            'YaSM (Golovan) charts',
            'https://yasm.yandex-team.ru/template/panel/dbaas_clickhouse_metrics/cid={cid}',
        ),
        (
            'Solomon',
            'Solomon charts',
            'https://solomon.yandex-team.ru/?cluster=mdb_{cid}'
            '&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-clickhouse',
        ),
        (
            'Console',
            'Console charts',
            '{console}/folders/{folderId}/mdb/cluster/{cluster_type}/{cid}?section=monitoring',
        ),
    ],
    'mongodb_cluster': [
        (
            'YASM',
            'YaSM (Golovan) charts',
            'https://yasm.yandex-team.ru/template/panel/{yasm_dashboard}/cid={cid}',
        ),
        (
            'Solomon',
            'Solomon charts',
            'https://solomon.yandex-team.ru/?cluster=mdb_{cid}'
            '&project=internal-mdb&service=mdb&dashboard={solomon_dashboard}',
        ),
        (
            'Console',
            'Console charts',
            '{console}/folders/{folderId}/mdb/cluster/{cluster_type}/{cid}?section=monitoring',
        ),
    ],
    'redis_cluster': [
        (
            'Solomon',
            'Solomon charts',
            'https://solomon.yandex-team.ru/?cluster=mdb_{cid}'
            '&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-redis',
        ),
        (
            'Console',
            'Console charts',
            '{console}/folders/{folderId}/mdb/cluster/{cluster_type}/{cid}?section=monitoring',
        ),
    ],
    'mysql_cluster': [
        (
            'YASM',
            'YaSM (Golovan) charts',
            'https://yasm.yandex-team.ru/template/panel/dbaas_mysql_metrics/cid={cid}',
        ),
        (
            'Solomon',
            'Solomon charts',
            'https://solomon.yandex-team.ru/?cluster=mdb_{cid}'
            '&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-mysql',
        ),
        (
            'Console',
            'Console charts',
            '{console}/folders/{folderId}/mdb/cluster/{cluster_type}/{cid}?section=monitoring',
        ),
    ],
}

CTYPE_CONFIG = {
    'postgresql_cluster': {},
    'clickhouse_cluster': {
        'zk': {
            'flavor': 's2.nano',
            'volume_size': 10737418240,
            'disk_type_id': DTYPE_LOCAL_SSD,
            'node_count': 3,
        },
        'shard_count_limit': 50,
    },
    'mongodb_cluster': {
        'mongocfg': {
            'flavor': 's2.nano',
            'volume_size': 10737418240,
            'disk_type_id': DTYPE_LOCAL_SSD,
            'node_count': 3,
        },
    },
    'mysql_cluster': {},
}

APISPEC_SWAGGER_URL = None
APISPEC_SWAGGER_UI_URL = None

BUCKET_PREFIX = 'yandexcloud-dbaas-'

# See flask_restful/__init__.py#L301
# Neither setting `message` to None nor error to proper text seemed to help.
ERROR_404_HELP = False

S3 = {
    'endpoint_url': 'https://s3.mds.yandex.net',
    # 'aws_access_key_id': 'access key id for BACKUPS_BUCKET',
    # 'aws_secret_access_key': 'secret access key for BACKUPS_BUCKET',
}

BOTO_CONFIG = {
    's3': {
        'addressing_style': 'auto',
        'region_name': 'us-east-1',
    },
}

# Validation of URI references to external resources.
EXTERNAL_URI_VALIDATION = {
    'regexp': r'https://(?:[a-zA-Z0-9-]+\.)?storage\.yandexcloud\.net/\S+',
    'validator': URIValidator,
    'validator_config': {
        'ca_path': '/my/ca/path',
    },
}

VERSION_VALIDATOR = {
    'validator': VersionValidator,
    'validator_config': {
        'packages_uri': 'http://mdb-bionic-secure.dist.yandex.ru/mdb-bionic-secure/stable/all/Packages.gz',
    },
}

NETWORK_PROVIDER = MetaNetworkProvider

NETWORK = {
    'url': '',
    'connect_timeout': 1,
    'read_timeout': 2,
    'token': 'my secret SA token',
    'ca_path': '/my/ca/path',
}

NETWORK_VTYPES = [
    VTYPE_COMPUTE,
]

COMPUTE_PROVIDER = YCComputeProvider

COMPUTE_GRPC = {
    'url': '',
    'token': 'my secret SA token',
    'ca_path': '/my/ca/path',
}

COMPUTE_BILLING_PROVIDER = YCComputeBillingProvider

MINIMAL_DISK_UNIT = 1

ZONE_ID_TO_PRIORITY = {
    'myt': 10,
    'iva': 10,
    'sas': 5,
    'vla': 5,
    'man': 0,
}

HADOOP_DEFAULT_RESOURCES = {
    'resource_preset_id': 's2.micro',
    'disk_size': 21474836480,
    'disk_type_id': DTYPE_NETWORK_SSD,
}

PUBLIC_IMAGES_FOLDER = ''

HADOOP_DEFAULT_VERSION_PREFIX = '2.0'
HADOOP_IMAGES = {
    '2.0.0': {
        'name': '2.0.0',
        'version': '2.0.0',
        'description': 'stable image for hadoop instances',
        'imageId': 'computeImageId2',
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
            },
            'spark': {
                'version': '2.2.1',
                'default': True,
                'deps': ['yarn'],
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
        },
        'roles_services': {
            'hadoop_cluster.masternode': [
                'hdfs',
                'yarn',
                'mapreduce',
                'zookeeper',
                'hbase',
                'hive',
                'spark',
                'zeppelin',
                'oozie',
            ],
            'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'spark', 'flume'],
            'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez', 'flume'],
        },
    },
    '1.4.0': {
        'name': '1.4.0',
        'version': '1.4.0',
        'description': 'image for hadoop instances',
        'imageId': 'computeImageId',
        'imageMinSize': 10737418240,
        'supported_pillar': '1.0.0',
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
            ],
            'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'flume'],
            'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez', 'flume'],
        },
    },
    '1.3.0': {
        'version': '1.3.0',
        'name': 'deprecated',
        'deprecated': True,
    },
}


HADOOP_PILLARS: Dict[str, Dict] = {
    '1.1.0': {
        'data': {
            'unmanaged': {
                'services': [
                    'hdfs',
                    'yarn',
                    'mapreduce',
                    'tez',
                    'zookeeper',
                    'hbase',
                    'hive',
                    'sqoop',
                    'flume',
                    'spark',
                    'zeppelin',
                ],
                'topology': {
                    'subclusters': {},
                },
            },
        },
    },
    '1.0.0': {
        'data': {
            'unmanaged': {
                'services': [
                    'hdfs',
                    'yarn',
                    'mapreduce',
                    'tez',
                    'zookeeper',
                    'hbase',
                    'hive',
                    'sqoop',
                    'flume',
                ],
                'topology': {
                    'subclusters': {},
                },
            },
        },
    },
}

HADOOP_UI_LINKS = {
    'base_url': 'https://cluster-${CLUSTER_ID}.dataproc-ui/',
    'knoxless_base_url': 'https://ui-${CLUSTER_ID}-${HOST}-${PORT}.dataproc-ui/',
    'services': {
        'hdfs': [
            {
                'code': 'hdfs',
                'name': 'HDFS Namenode UI',
                'port': 1001,
            },
        ],
        'yarn': [
            {
                'code': 'yarn',
                'name': 'YARN Resource Manager Web UI',
                'port': 1002,
            },
            {
                'code': 'jobhistory',
                'name': 'JobHistory Server Web UI',
                'port': 1003,
            },
        ],
    },
}

DNS_CACHE: dict[str, str] = {}

UNMANAGED_CLUSTER_TYPES = ['hadoop_cluster']

IAM = IAMClient

IAM_PROVIDER_CONFIG = {
    'url': 'https://iam-endpoint',
    'token': 'iam secret token',
    'ca_path': '/my/ca/path',
}

IAM_JWT = YCIamJwtProvider

IAM_JWT_CONFIG = {
    'audience': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
    'url': '',
    'cert_file': '',
    'expire_thresh': 180,
    'request_expire': 3600,
    'server_name': '',
    'insecure': False,
    'service_account_id': '',
    'key_id': '',
    'private_key': '',
}

RESOURCE_MANAGER = ResourceManagerGRPC

RESOURCE_MANAGER_CONFIG = {
    'url': '',
    'token': 'my secret SA token',
    'ca_path': '/my/ca/path',
}

DATAPROC_MANAGER_CONFIG = {
    'class': DataprocManagerClient,
    'private_url': 'https://dataproc-manager',
    'cert_file': '',
    'server_name': '',
    'url': '',
    'insecure': False,
}

DATAPROC_MANAGER_PUBLIC_CONFIG = {
    'url': '',
    'server_name': '',
    'cert_file': '',
}

DATAPROC_JOBLOG = DataprocS3JobLogClient

# Default options for s3 client if they are not customized on cluster pillar
DATAPROC_JOBLOG_CONFIG = {
    'endpoint': 'storage.yandexcloud.net',
    'region': 'ru-central1',
}

# zone_ids in which we don't allow
# create and upscale host
DECOMMISSIONING_ZONES: List[str] = []

# List of names of decommissioning flavors,
# user can't create cluster with such flavor
# or change resource preset to it.
DECOMMISSIONING_FLAVORS: List[str] = ['db1.nano']

# If CH hosts have at least "ch_subcluster_cpu" cores in total,
# then each ZK host must have at least "zk_host_cpu" cores.
MINIMAL_ZK_RESOURCES = [
    {
        "ch_subcluster_cpu": 16,
        "zk_host_cpu": 2,
    },
    {
        "ch_subcluster_cpu": 48,
        "zk_host_cpu": 4,
    },
]

DEFAULT_BACKUP_SCHEDULE = {
    'postgresql_cluster': {
        'retain_period': 7,
        'use_backup_service': False,
        'sleep': 7200,
        'start': {
            'hours': 22,
            'minutes': 15,
            'seconds': 30,
            'nanos': 100,
        },
    },
    'mysql_cluster': {
        'sleep': 7200,
        'use_backup_service': False,
        'retain_period': 7,
        'start': {
            'hours': 22,
            'minutes': 15,
            'seconds': 30,
            'nanos': 100,
        },
    },
    'clickhouse_cluster': {
        'sleep': 7200,
        'start': {
            'hours': 22,
            'minutes': 15,
            'seconds': 30,
            'nanos': 100,
        },
    },
    'mongodb_cluster': {
        'sleep': 7200,
        'start': {
            'hours': 22,
            'minutes': 15,
            'seconds': 30,
            'nanos': 100,
        },
        'retain_period': 7,
        'use_backup_service': False,
    },
    'redis_cluster': {
        'sleep': 7200,
        'start': {
            'hours': 22,
            'minutes': 15,
            'seconds': 30,
            'nanos': 100,
        },
    },
}

INSTANCE_GROUP_SERVICE_CONFIG = {
    'url': 'instance-group.private-api.ycp.cloud-preprod.yandex.net:443',
    'grpc_timeout': 30,
    'token': '',  # compute api token
}

LOGGING_SERVICE = LoggingService
LOGGING_SERVICE_CONFIG = {
    'url': 'logging-cpl.private-api.ycp.cloud-preprod.yandex.net:443',
    'grpc_timeout': 30,
}
LOGGING_READ_SERVICE = LoggingReadService
LOGGING_READ_SERVICE_CONFIG = {
    'url': 'reader.logging.cloud-preprod.yandex.net:443',
    'grpc_timeout': 30,
}

RESOURCE_MANAGER_CONFIG_GRPC = {
    'url': '',
    'cert_file': '/my/ca_file',
}

YANDEX_TEAM_INTEGRATION_CONFIG = {
    'url': 'ti.cloud.yandex-team.ru:443',
    'cert_file': '/my/ca_file',
}

RACKTABLES_CLIENT_CONFIG = {
    'base_url': 'https://ro.racktables.yandex-team.ru',
    'oauth_token': '',
}
