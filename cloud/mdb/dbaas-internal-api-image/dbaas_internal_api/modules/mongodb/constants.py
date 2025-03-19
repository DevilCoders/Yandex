# -*- coding: utf-8 -*-
"""
DBaaS Internal API MongoDB globals
"""
from functools import reduce
from typing import Dict

MY_CLUSTER_TYPE = 'mongodb_cluster'

RESTART_DEFAULTS = [
    'mongod.storage.wiredTiger.engineConfig.cacheSizeGB',
    'mongocfg.storage.wiredTiger.engineConfig.cacheSizeGB',
]

# Feature Flags
UNLIMITED_SHARD_COUNT_FEATURE_FLAG = "MDB_MONGODB_UNLIMITED_SHARD_COUNT"
PITR_RS_FEATURE_FLAG = 'MDB_MONGODB_RS_PITR'
PITR_DISABLE_FEATURE_FLAG = 'MDB_MONGODB_RESTORE_WITHOUT_REPLAY'
MIXED_SHARDING_CONFIG_FEATURE_FLAG = 'MDB_MONGODB_INFRA_CFG'
ALLOW_DEPRECATED_VERSIONS_FEATURE_FLAG = 'MDB_MONGODB_ALLOW_DEPRECATED_VERSIONS'
ALLOW_RESTORE_DOWNGRADE_FEATURE_FLAG = 'MDB_MONGODB_ALLOW_RESTORE_DOWNGRADE'
ENTERPRISE_MONGO_FEATURE_FLAG = 'MDB_MONGODB_ENTERPRISE'
ALLOW_GLOBAL_WRITER_ROLE_FEATURE_FLAG = 'MDB_MONGODB_GLOBAL_WRITER_ROLE'
ALLOW_REPLICATION_ROLE_FEATURE_FLAG = 'MDB_MONGODB_REPLICATION_ROLE'

INTERNAL_USERS = ['admin', 'monitor', 'root']
SYSTEM_DATABASES = ['admin', 'local', 'config', 'mdb_internal']
DB_ROLE_MAP = {
    'local': {},
    'config': {},
    'admin': {
        'mdbMonitor': True,
        'mdbShardingManager': True,
        'mdbReplication': True,
        'mdbGlobalWriter': ALLOW_GLOBAL_WRITER_ROLE_FEATURE_FLAG,
    },
    '*': {
        'read': True,
        'readWrite': True,
        'mdbDbAdmin': True,
    },
}
# Mypy complaints: Unsupported left operand type for + ("object")
VALID_ROLES = reduce(lambda x, y: x + list(y.keys()), list(DB_ROLE_MAP.values()), [])  # type: ignore
DEFAULT_ROLES = ['readWrite']

MONGOD_HOST_TYPE = 'mongod'
MONGOS_HOST_TYPE = 'mongos'
MONGOCFG_HOST_TYPE = 'mongocfg'
MONGOINFRA_HOST_TYPE = 'mongoinfra'

DEFAULT_AUTH_SERVICES = [MONGOD_HOST_TYPE, MONGOS_HOST_TYPE]

DEFAULT_SHARD_NAME = 'rs01'

SUBCLUSTER_NAMES = {
    MONGOD_HOST_TYPE: 'mongod_subcluster',
    MONGOCFG_HOST_TYPE: 'mongocfg_subcluster',
    MONGOS_HOST_TYPE: 'mongos_subcluster',
    MONGOINFRA_HOST_TYPE: 'mongoinfra_subcluster',
}

SHARDING_INFRA_HOST_TYPES = [MONGOINFRA_HOST_TYPE, MONGOCFG_HOST_TYPE, MONGOS_HOST_TYPE]

# As we've added mongoinfra hosts, we can't validate mongos/mongocfg/mongoinfra hosts count via
# metadb
MIN_MONGOS_HOSTS_COUNT = 2
MIN_MONGOCFG_HOSTS_COUNT = 3

EDITION_ENTERPRISE = 'enterprise'
MAX_SHARDS_COUNT = 10

UPGRADE_PATHS: Dict[str, dict] = {
    '4.0': {
        'from': ['3.6'],
        'deprecated_config_values': {
            'mongod.storage.journal.enabled': [False],
        },
    },
    '4.2': {
        'from': ['4.0'],
    },
    '4.4': {
        'from': ['4.2'],
    },
    '5.0': {
        'from': ['4.4'],
    },
}

VERSIONS_SUPPORTS_SHARDING = ['4.0', '4.2', '4.4', '5.0']
VERSIONS_SUPPORTS_PITR = ['4.2', '4.4', f'4.4-{EDITION_ENTERPRISE}', '5.0', f'5.0-{EDITION_ENTERPRISE}']

# deploy api divides task timeout by 6 (number of retries)
# we should preserve calculated timeout in number of cases (e.g. backup restore)
DEPLOY_API_RETRIES_COUNT = 6
DB_LIMIT_DEFAULT = 1000

GBYTES = 1024**3
