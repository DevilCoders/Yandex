"""
MongoDB pillar generator
"""
from copy import deepcopy
from typing import Any, Dict, List

from flask import current_app

from ...core.base_pillar import BackupAndAccessPillar
from ...core.crypto import encrypt, gen_encrypted_password, gen_random_string
from ...core.exceptions import (
    DatabaseAccessExistsError,
    DatabaseAccessNotExistsError,
    DatabaseExistsError,
    DatabaseNotExistsError,
    UserExistsError,
    UserNotExistsError,
    DbaasClientError,
)
from ...utils.helpers import merge_dict
from ...utils.types import Pillar
from ...utils.version import Version
from .constants import (
    DEFAULT_AUTH_SERVICES,
    DEFAULT_ROLES,
    INTERNAL_USERS,
    MY_CLUSTER_TYPE,
    SYSTEM_DATABASES,
    DB_LIMIT_DEFAULT,
)


# pylint: disable=too-many-public-methods
class MongoDBPillar(BackupAndAccessPillar):
    """
    MongoDB pillar.
    """

    def __init__(self) -> None:
        super().__init__(
            {
                'data': {
                    'mongodb': {},
                },
            }
        )

    @classmethod
    def make(cls) -> "MongoDBPillar":
        """
        Create new pillar.
        """
        # pylint: disable=protected-access
        pillar = MongoDBPillar.load(deepcopy(current_app.config['DEFAULT_PILLAR_TEMPLATE'][MY_CLUSTER_TYPE]))

        for params in pillar._pillar_users.values():
            if params.get('password') is None:
                params['password'] = gen_encrypted_password()

        if pillar._mongodb.get('cluster_auth') == 'keyfile':
            if pillar._mongodb.get('keyfile') is None:
                pillar._mongodb['keyfile'] = gen_random_string(1024)

        return pillar

    @classmethod
    def load(cls, pillar_data: Pillar) -> "MongoDBPillar":
        """
        Construct pillar object from pillar data.
        """
        # pylint: disable=protected-access
        pillar = MongoDBPillar()
        pillar._pillar = pillar_data
        return pillar

    @property
    def _mongodb(self):
        return self._pillar['data']['mongodb']

    @property
    def cluster_name(self) -> str:
        """Get cluster name"""
        return self._mongodb['cluster_name']

    @cluster_name.setter
    def cluster_name(self, name: str) -> None:
        self._mongodb['cluster_name'] = name

    def user(self, cluster_id: str, user_name: str) -> Dict[str, Any]:
        """
        Return user object conforming to MongodbUserSchema.
        """
        try:
            params = self._pillar_users[user_name]
            return self._to_user(cluster_id, user_name, params)
        except KeyError:
            raise UserNotExistsError(user_name)

    def users(self, cluster_id: str) -> List[Dict[str, Any]]:
        """
        Return list of user objects conforming to MongodbUserSchema.
        """
        return [
            self._to_user(cluster_id, name, params)
            for name, params in sorted(self._pillar_users.items())
            if name not in INTERNAL_USERS
        ]

    @staticmethod
    def _to_user(cluster_id: str, name: str, params: Dict) -> Dict:
        return {
            'cluster_id': cluster_id,
            'name': name,
            'permissions': [
                {
                    'database_name': db_name,
                    'roles': roles,
                }
                for db_name, roles in params.get('dbs', {}).items()
            ],
        }

    def add_user(self, user_spec: Dict) -> None:
        """
        Add user.
        """
        user_name = user_spec['name']
        if user_name in self._pillar_users:
            raise UserExistsError(user_name)

        if user_spec.get('permissions') is None:
            user_spec['permissions'] = self._get_default_user_permissions()

        user = {}  # type: Dict
        user.update(self._format_user_secrets(user_spec['password']))
        user.update(self._format_user_permissions(user_name, user_spec['permissions']))
        self._check_disable_db_management(user_spec['permissions'])
        user.update({'services': deepcopy(DEFAULT_AUTH_SERVICES)})

        self._pillar_users[user_name] = user

    def add_users(self, user_specs: List[Dict]) -> None:
        """
        Add users.
        """
        for user_spec in user_specs:
            self.add_user(user_spec)

    def update_user(self, user_name: str, password: str, permissions: List[Dict]) -> None:
        """
        Update user.
        """
        user = self._get_external_user(user_name)

        if password is not None:
            user.update(self._format_user_secrets(password))

        if permissions is not None:
            user.update(self._format_user_permissions(user_name, permissions))
            self._check_disable_db_management(permissions)

    def delete_user(self, user_name: str) -> None:
        """
        Delete user.
        """
        self._ensure_external_user_exists(user_name)
        del self._pillar_users[user_name]

    def get_user_databases(self, user_name: str) -> List:
        """
        Get user databases list.
        """
        return list(self._get_external_user(user_name).get('dbs', {}).keys())

    def add_user_permission(self, user_name: str, permission: Dict) -> None:
        """
        Add user permission.
        """
        user = self._get_external_user(user_name)

        db_name = permission['database_name']
        if db_name not in list(self._pillar_dbs) + SYSTEM_DATABASES:
            raise DatabaseNotExistsError(db_name)

        if db_name in user['dbs']:
            raise DatabaseAccessExistsError(user_name, db_name)

        user['dbs'].update({db_name: permission['roles']})
        self._check_disable_db_management([permission])

    def delete_user_permission(self, user_name: str, db_name: str) -> None:
        """
        Add user permission.
        """
        user = self._get_external_user(user_name)

        if db_name not in list(self._pillar_dbs) + SYSTEM_DATABASES:
            raise DatabaseNotExistsError(db_name)

        if db_name not in user['dbs']:
            raise DatabaseAccessNotExistsError(user_name, db_name)

        user['dbs'].pop(db_name)

    def _get_external_user(self, user_name: str) -> Dict:
        user = self._pillar_users.get(user_name)
        if not user or user_name in INTERNAL_USERS:
            raise UserNotExistsError(user_name)

        return user

    def _ensure_external_user_exists(self, user_name: str) -> None:
        self._get_external_user(user_name)

    def _get_default_user_permissions(self) -> List:
        permissions = [
            {
                'database_name': db_name,
                'roles': deepcopy(DEFAULT_ROLES),
            }
            for db_name in self._pillar_dbs.keys()
        ]
        return permissions

    @staticmethod
    def _format_user_secrets(password: str) -> Dict:
        return {
            'password': encrypt(password),
        }

    def _format_user_permissions(self, user_name: str, permissions: List[Dict]) -> Dict:
        dbs = {}  # type: Dict
        for permission in permissions:
            db_name = permission['database_name']
            # Users may require maintenance privs on system dbs, like `mdbMonitor` on `admin`
            if db_name not in list(self._pillar_dbs) + SYSTEM_DATABASES:
                raise DatabaseNotExistsError(db_name)
            if db_name in dbs:
                raise DatabaseAccessExistsError(user_name, db_name)

            dbs.update({db_name: permission['roles']})

        return {'dbs': dbs}

    @property
    def pillar_users(self) -> Dict:
        """
        Users without any reformat
        """
        return deepcopy(self._pillar_users)

    @pillar_users.setter
    def pillar_users(self, pillar_users: Dict) -> None:
        self._mongodb['users'] = pillar_users

    @property
    def _pillar_users(self) -> Dict:
        return self._mongodb['users']

    def _check_disable_db_management(self, permissions: List[Dict]) -> None:
        """
        If [any] user has admin.mdbGlobalWriter role,
        then disable database management by API
        (see MDB-17284)
        """
        for permission in permissions:
            if (permission['database_name'] == 'admin') and ('mdbGlobalWriter' in permission.get('roles', [])):
                self._mongodb['disable_db_management'] = True

    def databases(self, cluster_id: str) -> List[Dict]:
        """
        Return list of database objects conforming to MongodbDatabaseSchema.
        """
        return [
            {
                'cluster_id': cluster_id,
                'name': name,
                **params,
            }
            for name, params in self._pillar_dbs.items()
        ]

    def database(self, cluster_id: str, db_name: str) -> Dict:
        """
        Return database object conforming to MongodbDatabaseSchema.
        """
        if db_name not in self._pillar_dbs:
            raise DatabaseNotExistsError(db_name)

        return {
            'cluster_id': cluster_id,
            'name': db_name,
            **self._pillar_dbs[db_name],
        }

    def add_database(self, database_spec: Dict) -> None:
        """
        Add database
        """
        if len(self._pillar_dbs) > self.db_limit:
            raise DbaasClientError("current db count limit {} exceeded".format(self.db_limit))

        dbspec = deepcopy(database_spec)
        db_name = dbspec.pop('name')

        if db_name in self._pillar_dbs:
            raise DatabaseExistsError(db_name)
        self._pillar_dbs[db_name] = dbspec

    def add_databases(self, db_specs: List[Dict]) -> None:
        """
        Add databases
        """
        for db_spec in db_specs:
            self.add_database(db_spec)

    def delete_database(self, db_name: str) -> None:
        """
        Delete database
        """
        if db_name not in self._pillar_dbs:
            raise DatabaseNotExistsError(db_name)

        for options in self._pillar_users.values():
            if db_name in options['dbs']:
                options['dbs'].pop(db_name)

        del self._pillar_dbs[db_name]

    @property
    def db_limit(self) -> int:
        """
        Get databases number limit
        """
        return self._mongodb.get('db_limit', DB_LIMIT_DEFAULT)

    @property
    def pillar_databases(self) -> Dict:
        """
        Databases without any reformat
        """
        return deepcopy(self._pillar_dbs)

    @pillar_databases.setter
    def pillar_databases(self, pillar_databases: Dict) -> None:
        self._mongodb['databases'] = pillar_databases

    @property
    def _pillar_dbs(self) -> Dict[str, Dict]:
        """
        Get databases map as is in pillar
        """
        return self._mongodb['databases']

    @property
    def zk_hosts(self) -> List[str]:
        """
        Return zk_hosts

        list of fqdn
        """
        return self._mongodb['zk_hosts'][:]

    @zk_hosts.setter
    def zk_hosts(self, hosts: List[str]) -> None:
        self._mongodb['zk_hosts'] = hosts

    @property
    def version(self) -> Version:
        """
        Get MongoDB version. For backward compatibility only
        """
        return Version.load(self._mongodb.get('version', {}).get('major_human', '0.0'))

    @property
    def feature_compatibility_version(self) -> str:
        """
        Get MongoDB feature compatibility version.
        """
        return self._mongodb['feature_compatibility_version']

    @feature_compatibility_version.setter
    def feature_compatibility_version(self, fcv: str) -> None:
        """
        Set MongoDB feature compatibility version.
        """
        self._mongodb['feature_compatibility_version'] = fcv

    @property
    def config(self) -> Dict:
        """Get cluster config"""
        return deepcopy(self._mongodb['config'])

    def update_config(self, config: Dict) -> None:
        """
        Update MongoDB config
        """
        merge_dict(self._mongodb['config'], config)

    @property
    def walg(self):
        if 'walg' not in self._pillar['data']:
            return {}
        return deepcopy(self._pillar['data']['walg'])

    def update_walg_config(self, config: Dict) -> None:
        """
        Update Wal-G config
        """
        if 'walg' not in self._pillar['data']:
            self._pillar['data']['walg'] = {}

        merge_dict(self._pillar['data']['walg'], config)

    @property
    def sharding_enabled(self) -> bool:
        """
        Enable sharding flag
        """
        return self._mongodb.get('sharding_enabled', False)

    @sharding_enabled.setter
    def sharding_enabled(self, value: bool) -> None:
        self._mongodb['sharding_enabled'] = value

    @property
    def performance_diagnostics(self) -> dict:
        return {'profiling_enabled': self._pillar['data'].get('perf_diag', {}).get('mongodb', {}).get('enabled', False)}

    def update_performance_diagnostics(self, perf_diag: dict) -> None:
        profiling_enabled = perf_diag.get('profiling_enabled', None)
        if profiling_enabled is None:
            return

        if 'perf_diag' not in self._pillar['data']:
            self._pillar['data']['perf_diag'] = {'mongodb': {}}
        if 'mongodb' not in self._pillar['data']['perf_diag']:
            self._pillar['data']['perf_diag']['mongodb'] = {}
        self._pillar['data']['perf_diag']['mongodb']['enabled'] = profiling_enabled


def get_cluster_pillar(cluster: dict) -> MongoDBPillar:
    """
    Return MongoDB pillar by cluster
    """
    return MongoDBPillar.load(cluster['value'])
