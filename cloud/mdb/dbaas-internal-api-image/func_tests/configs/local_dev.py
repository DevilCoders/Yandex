"""
Config with dummy auth provider (use for local dev)
"""
import os

from tests.providers import (
    ConfigResourceManager,
    DataprocManagerMock,
    DummyAuthProvider,
    DummyConfigAuthProvider,
    DummyLoggingService,
    DummyLoggingReadService,
    DummyURIValidator,
    DummyYCComputeBillingProvider,
    DummyYCIdentityProvider,
    DummyYCNetworkProvider,
    FileS3Api,
    MDBHealthProviderFromFile,
    MockedCryptoProvider,
    SequenceClusterSecretsProvider,
    SequenceHostnameGenerator,
    SequenceIdGenerator,
)

# pylint: disable=invalid-name

AUTH_PROVIDER = DummyAuthProvider
CRYPTO_PROVIDER = MockedCryptoProvider
MDBH_PROVIDER = MDBHealthProviderFromFile
IDENTITY_PROVIDER = DummyYCIdentityProvider
NETWORK_PROVIDER = DummyYCNetworkProvider
ID_GENERATOR = SequenceIdGenerator
HOSTNAME_GENERATOR = SequenceHostnameGenerator
S3_PROVIDER = FileS3Api
CONFIG_AUTH_PROVIDER = DummyConfigAuthProvider
CLUSTER_SECRETS_PROVIDER = SequenceClusterSecretsProvider
COMPUTE_BILLING_PROVIDER = DummyYCComputeBillingProvider
RESOURCE_MANAGER = ConfigResourceManager
LOGGING_SERVICE = DummyLoggingService
LOGGING_READ_SERVICE = DummyLoggingReadService

IDENTITY = {
    'override_folder': None,
    'override_cloud': None,
    'create_missing': True,
    'allow_move_between_clouds': True,
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

SEQUENCE_PATH = 'func_tests/tmp'

MDBHEALTH = {
    'path': 'func_tests/tmp/mdbhealth.json',
}

LOGCONFIG = {
    'version': 1,
    'disable_existing_loggers': True,
    'formatters': {
        'raw': {
            'format': '%(asctime)s [%(levelname)s] %(name)s:\t%(message)s',
        },
    },
    'handlers': {
        'console': {
            'class': 'logging.StreamHandler',
            'level': 'DEBUG',
            'formatter': 'raw',
            'stream': 'ext://sys.stdout',
        },
    },
    'loggers': {
        'dbaas-internal-api-request': {
            'handlers': ['console'],
            'level': 'DEBUG',
        },
        'dbaas-internal-api-background': {
            'handlers': ['console'],
            'level': 'DEBUG',
        },
        'clickhouse_driver': {
            'handlers': ['console'],
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
                'test-1': ['zkeeper-1-1', 'zkeeper-1-2', 'zkeeper-1-3'],
                'test-2': ['zkeeper-2-1', 'zkeeper-2-2', 'zkeeper-2-3'],
            },
        },
        'qa': {
            'zk': {
                'test-1': ['zkeeper-1-1', 'zkeeper-1-2', 'zkeeper-1-3'],
                'test-2': ['zkeeper-2-1', 'zkeeper-2-2', 'zkeeper-2-3'],
            },
        },
    },
    'mysql_cluster': {
        'dev': {
            'zk': {
                'test-1': ['zkeeper-1-1', 'zkeeper-1-2', 'zkeeper-1-3'],
                'test-2': ['zkeeper-2-1', 'zkeeper-2-2', 'zkeeper-2-3'],
            },
        },
        'qa': {
            'zk': {
                'test-1': ['zkeeper-1-1', 'zkeeper-1-2', 'zkeeper-1-3'],
                'test-2': ['zkeeper-2-1', 'zkeeper-2-2', 'zkeeper-2-3'],
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
    'clickhouse_cluster': {
        'dev': {
            'zk_hosts': ['localhost'],
        },
        'qa': {
            'zk_hosts': ['localhost'],
        },
    },
}

CTYPE_CONFIG = {
    'postgresql_cluster': {},
    'clickhouse_cluster': {
        'zk': {
            'flavor': 's1.porto.1',
            'volume_size': 10737418240,
            'disk_type_id': 'local-ssd',
            'node_count': 3,
        },
        'shard_count_limit': 50,
    },
    'mongodb_cluster': {
        'mongocfg': {
            'flavor': 's1.porto.1',
            'volume_size': 10737418240,
            'disk_type_id': 'local-ssd',
            'node_count': 3,
        },
    },
    'mysql_cluster': {},
}

RESOURCE_MANAGER_CONFIG = {
    'service_account_1': ['dataproc.agent'],
    'service_account_with_permission': ['dataproc.agent'],
    'service_account_without_permission': [],
}

DATAPROC_MANAGER_CONFIG = {
    'class': DataprocManagerMock,
    'health_path': 'func_tests/tmp/dataproc_manager_cluster_health.json',
}

EXTERNAL_URI_VALIDATION = {
    'regexp': r'\S+',
    'validator': DummyURIValidator,
}

# You can enable pretty json with
# RESTFUL_JSON = {'indent': 4, 'sort_keys': True}

S3_PROVIDER_RESPONSE_FILE = 'func_tests/tmp/s3response.json'
