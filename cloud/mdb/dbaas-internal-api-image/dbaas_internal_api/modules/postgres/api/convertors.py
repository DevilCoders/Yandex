"""
Convertors from API object to internal Postgre objects
"""

from typing import List

from .. import utils
from ....core.crypto import encrypt
from ..types import Database, UserWithPassword


def compute_connect_dbs(user_spec: dict, databases: List[Database]) -> List[str]:
    """
    Compute user connect database
    """
    allowed_databases = {d.name for d in databases}
    if 'permissions' in user_spec:
        allowed_databases = set(utils.permissions_to_databases(user_spec['permissions']))

        for database in databases:
            if database.owner == user_spec['name']:
                allowed_databases.add(database.name)
    return list(allowed_databases)


def user_from_spec(user_spec: dict, databases: List[Database]) -> UserWithPassword:
    """
    Construct user from spec
    """
    return UserWithPassword(
        name=user_spec['name'],
        connect_dbs=compute_connect_dbs(user_spec, databases),
        conn_limit=user_spec.get('conn_limit'),
        encrypted_password=encrypt(user_spec['password']),
        settings=user_spec.get('settings', {}),
        login=user_spec.get('login', True),
        grants=user_spec.get('grants', []),
    )


def user_to_spec(user: UserWithPassword, cluster_id: str) -> dict:
    """
    Serialize user to spec
    """
    return {
        'cluster_id': cluster_id,
        'name': user.name,
        'conn_limit': user.conn_limit,
        'permissions': [
            {
                'database_name': dbname,
            }
            for dbname in user.connect_dbs
        ],
        'settings': user.settings,
        'login': user.login,
        'grants': user.grants,
    }


def database_from_spec(spec: dict) -> Database:
    """
    Construct database from spec
    """
    return Database(
        name=spec['name'],
        owner=spec['owner'],
        extensions=utils.parse_extensions(spec.get('extensions', [])),
        lc_collate=spec['lc_collate'],
        lc_ctype=spec['lc_ctype'],
        template=spec.get('template', ''),
    )


def database_to_spec(db: Database, cluster_id: str) -> dict:
    """
    Serialize database to spec
    """
    result = {
        'cluster_id': cluster_id,
        'name': db.name,
        'owner': db.owner,
        'extensions': utils.format_extensions(db.extensions),
        'lc_collate': db.lc_collate,
        'lc_ctype': db.lc_ctype,
    }
    if db.template:
        result['template'] = db.template
    return result
