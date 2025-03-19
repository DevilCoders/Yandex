"""
Here we redefine some config options for internal api
"""

# flake8: noqa
# pylint: skip-file

CRYPTO = {
    'api_secret_key': '{{ conf.dynamic.internal_api.pki.secret }}',
    'client_public_key': '{{ conf.dynamic.salt.pki.public }}',
}

YC_ACCESS_SERVICE = {
    'endpoint': 'fake_iam01.{{ conf.network_name }}:4284',
    'timeout': 1,
}

YC_IDENTITY = {
    'cache_ttl': 0,
    'cache_size': 0,
}

RESOURCE_MANAGER_CONFIG_GRPC = {
    'url': 'fake_resourcemanager01.{{ conf.network_name }}:4040',
    'cert_file': '/config/CA.pem',
}

METADB = {
    'hosts': ['metadb01.{{ conf.network_name }}', ],
    'user': '{{ conf.projects.metadb.db.user }}',
    'password': '{{ conf.projects.metadb.db.password }}',
    'dbname': '{{ conf.projects.metadb.db.dbname }}',
    'port': 5432,
    'maxconn': 10,
    'minconn': 5,
    'sslmode': 'allow',
    'connect_timeout': 1,
}

LOGSDB = {
    'hosts': [],
    'dsn': {
        'user': '',
        'password': '',
        'port': 9000,
        'compression': 'lz4',
    },
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
            'stream': 'ext://sys.stderr',
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
    },
}

CTYPE_CONFIG = {
    'postgresql_cluster': {},
    'clickhouse_cluster': {
        'zk': {
            'flavor': 'db1.nano',
            'volume_size': 10737418240,
            'node_count': 3,
            'disk_type_id': 'local-ssd',
        },
        'shard_count_limit': 50,
    },
    'mongodb_cluster': {},
    'redis_cluster': {},
    'mysql_cluster': {},
    'greenplum_cluster': {},
}

ENVCONFIG = {
    'postgresql_cluster': {
        'dev': {
            'zk': {
                'test': ['zookeeper01.{{ conf.network_name }}:2181', ]
            },
        },
    },
    'clickhouse_cluster': {
        'dev': {
            'zk_hosts': ['zookeeper01.{{ conf.network_name }}', ],
        },
    },
    'mongodb_cluster': {
        'dev': {
            'zk_hosts': ['zookeeper01.{{ conf.network_name }}', ],
        },
    },
    'redis_cluster': {
        'dev': {
            'zk_hosts': ['zookeeper01.{{ conf.network_name }}', ],
        },
    },
    'mysql_cluster': {
        'dev': {
            'zk': {
                'test': ['zookeeper01.{{ conf.network_name }}:2181', ]
            },
        },
    },
}

ENV_MAPPING = {
    'dev': 'prestable',
    'qa': 'production',
}

VERSIONS = {
    'clickhouse_cluster': [{'version': '22.3.8.39'}, {'version': '22.4.6.53'}, {'version': '22.5.2.53'}],
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
    ],
    'mongodb_cluster': {
        '306': '30623',
        '400': '40023',
        '402': '40212',
        '404': '40404',
        '500': '50002',
    },
}  # yapf: disable

DEFAULT_PROXY_CLUSTER_PILLAR_TEMPLATE = {
    'postgresql_cluster': {
        'id': None,
        'dbname': None,
        'db_port': 6432,
        'max_lag': 30,
        'max_sessions': 100,
        'auth_user': None,
        'auth_dbname': 'postgres',
        'zk_cluster': None,
        'direct': False,
        'acl': [{
            'type': 'subnet',
            'value': '0.0.0.0/0',
        }],
    },
}

DEFAULT_DISK_TYPE_IDS = {
    'porto': 'local-ssd',
    'compute': 'network-nvme',
}

VTYPES = {
    'porto': '{{ conf.network_name }}',
}

DEFAULT_OPTIONS = {
    'postgresql_cluster': {
        'shared_buffers': '8MB',
    },
    'clickhouse_cluster': {},
    'mongodb_cluster': {},
    'redis_cluster': {},
    'mysql_cluster': {},
    'greenplum_cluster': {},
}

BUCKET_PREFIX = '{{ conf.dynamic.s3.bucket_prefix }}'

S3 = {
    'endpoint_url': "{{ conf.dynamic.s3.endpoint | replace('+path', '', 1) }}",
    'aws_access_key_id': '{{ conf.dynamic.s3.access_key_id }}',
    'aws_secret_access_key': '{{ conf.dynamic.s3.access_secret_key }}',
}

EXPOSE_ALL_TASK_ERRORS = True

DECOMMISSIONING_FLAVORS = []

EXTERNAL_URI_VALIDATION = {
    'regexp': r'\S+',
    'validator_config': {
        'ca_path': False,
    },
}

IAM_JWT_CONFIG = {
    'url': "fake_tokenservice01.{{ conf.network_name }}:50051",
    'insecure': True,
    'service_account_id': "yc.mdb.internal-api",
    'key_id': "{{ conf.jwt.key_id }}",
    'private_key': "{{ conf.jwt.private_key }}",
    'audience': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
    'expire_thresh': 180,
    'request_expire': 3600,
}
