from dataclasses import dataclass
from datetime import datetime

ROLES = {
    'admin': 'admin',
    'editor': 'editor',
    'member': 'resource-manager.clouds.member',
    'member': 'resource-manager.clouds.member',
    'owner': 'resource-manager.clouds.owner',
    'viewer': 'viewer'
}


DB_TYPES = {
    'pg': 'managed-postgresql',
    'mongo': 'managed-mongodb',
    'ch': 'managed-clickhouse',
    'mysql': 'managed-mysql',
    'redis': 'managed-redis',
    'hadoop': 'dataproc',
    'kafka': 'managed-kafka',  # ToDo: parse kafka
    # ToDo: 'greenplum': 'managed-greenplum',
    # ToDo: 'elastic': 'managed-elasticsearch',
    # ToDo: 'sql': 'mdb/sqlserver'

}


@dataclass
class IamToken:
    token: str
    expires_at: datetime
