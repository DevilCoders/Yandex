"""
MySQL pillar generator
"""
import copy
from copy import deepcopy
from typing import Any, Dict, List

from flask import current_app

from ...core.base_pillar import BackupAndAccessPillar
from ...core.crypto import encrypt, gen_encrypted_password
from ...core.exceptions import (
    DatabaseAccessExistsError,
    DatabaseAccessNotExistsError,
    DatabaseExistsError,
    DatabaseNotExistsError,
    DatabaseRolesNotExistError,
    DbaasClientError,
    IdmSysUserPropertyEditError,
    IdmUserEditError,
    UserExistsError,
    UserNotExistsError,
)
from ...utils.helpers import merge_dict
from ...utils.pillar import SubPillar, UninitalizedPillarSection
from ...utils.types import Pillar
from ...utils.version import Version
from .constants import (
    AUTH_PLUGINS_5_7,
    DEFAULT_ROLES,
    DEFAULT_SERVICES,
    INTERNAL_USERS,
    MY_CLUSTER_TYPE,
    SPECIAL_GLOBAL_PRIVILEGES,
)

SYSTEM_IDM_USERS = {
    'mdb_writer': None,
    'mdb_reader': ['SELECT'],
}


class _MysqlSubPillar(SubPillar):
    """
    Holds link to pillar part

    Subclass must redefine _path attribute
    """

    def __init__(self, parent_pillar: Dict, parent: 'MySQLPillar') -> None:
        super().__init__(parent_pillar, parent)


class _MySQLConfigPillar(_MysqlSubPillar):
    _path = ['data', 'mysql', 'config']

    def update_config(self, config: Dict) -> None:
        """
        Updates data.config section

        Updates only top-level-keys
        """
        # write it only for create and restore cluster cases
        self._pillar_part.update(config)

    def get_config(self) -> Dict:
        """
        Returns config as a dict
        """
        return copy.deepcopy(self._pillar_part)

    def drop_key(self, key):
        """
        Drops specified key in pillar
        """
        self._pillar_part.pop(key)

    def merge_config(self, config: Dict) -> None:
        """
        Updates data.config section recurisively
        """
        merge_dict(self._pillar_part, config)


# pylint: disable=too-many-public-methods
class MySQLPillar(BackupAndAccessPillar):
    """
    MySQL pillar access
    """

    def __init__(self, pillar: Pillar) -> None:
        super().__init__(pillar)
        self.config = _MySQLConfigPillar(pillar, self)

    def _set_mysql_data(self, key: str, value: Any) -> None:
        self._pillar['data']['mysql'].update(
            {
                key: value,
            }
        )

    def _get_mysql_data(self, key: str) -> Any:
        return self._pillar['data']['mysql'][key]

    @property
    def max_server_id(self):
        """
        Max server-id used in cluster
        """
        return self._get_mysql_data('_max_server_id')

    @max_server_id.setter
    def max_server_id(self, max_id):
        self._set_mysql_data('_max_server_id', int(max_id))

    @property
    def zk_hosts(self) -> List[str]:
        """
        List of zookeper hosts for cluster
        """
        return self._get_mysql_data('zk_hosts')[:]

    @zk_hosts.setter
    def zk_hosts(self, hosts: List[str]):
        self._set_mysql_data('zk_hosts', hosts)

    def user(self, cluster_id: str, user_name: str) -> Dict[str, Any]:
        """
        Return user object conforming to MysqlUserSchema.
        """
        try:
            params = self._pillar_users[user_name]
            return self._to_user(cluster_id, user_name, params)
        except KeyError:
            raise UserNotExistsError(user_name)

    def users(self, cluster_id: str, with_pass: bool = False) -> List[Dict[str, Any]]:
        """
        Return list of user objects conforming to MysqlUserSchema.
        """
        return [
            self._to_user(cluster_id, name, params, with_pass)
            for name, params in sorted(self._pillar_users.items())
            if name not in INTERNAL_USERS
        ]

    @property
    def user_names(self):
        """
        Return list of all user names (including internal users).
        """
        return sorted(self._pillar_users.keys())

    @staticmethod
    def _to_user(cluster_id: str, name: str, params: Dict, with_pass: bool = False) -> Dict:
        dbs = params.get('dbs', {})

        user = {
            'cluster_id': cluster_id,
            'name': name,
            'permissions': [
                {
                    'database_name': db_name,
                    'roles': roles,
                }
                for db_name, roles in dbs.items()
                if db_name != '*'
            ],
        }

        other_params = ['plugin', 'connection_limits']
        for param in other_params:
            value = params.get(param, {})
            if value:
                user[param] = value

        if '*' in dbs:
            global_permissions = []
            global_permissions = list(set(dbs['*']) - set(SPECIAL_GLOBAL_PRIVILEGES))
            global_permissions = sorted(global_permissions)
            user['global_permissions'] = global_permissions

        if with_pass:
            user['password'] = params.get('password')
        return user

    def add_user(self, user_spec: Dict, version: Version) -> None:
        """
        Add user.
        """
        user_name = user_spec['name']
        if user_name in self._pillar_users:
            raise UserExistsError(user_name)

        if user_spec.get('permissions') is None:
            user_spec['permissions'] = self._get_default_user_permissions()

        if user_spec.get('global_permissions') is not None:
            user_spec['permissions'] += [{'database_name': '*', 'roles': user_spec.get('global_permissions')}]

        user = {}  # type: Dict
        if 'password' in user_spec:
            user.update(self._format_user_secrets(user_spec['password']))
        user.update(self._format_user_permissions(user_name, user_spec['permissions']))
        self._update_user_connection_limits(user, user_spec.get('connection_limits'))  # type: ignore
        plugin = user_spec.get('plugin')

        if plugin:
            user.update({"plugin": plugin})
            self._validate_user_plugin(plugin, version)
        user.update({'services': deepcopy(DEFAULT_SERVICES)})
        self._pillar_users[user_name] = user

    def user_exists(self, user_name: str) -> bool:
        """
        Check if user exists
        """
        return user_name in self._pillar_users

    def update_user(
        self,
        user_name: str,
        password: str,
        permissions: List[Dict],
        global_permissions: List[str],
        connection_limits: Dict,
        plugin: str,
        version: Version,
    ) -> None:
        """
        Update user.
        """
        self._validate_user_plugin(plugin, version)

        user = self._get_external_user(user_name)
        self._ensure_allow_edit(user_name, user, password, plugin)
        global_permissions_inplace = user.get('dbs', {}).get('*', [])
        if password is not None:
            user.update(self._format_user_secrets(password))

        if permissions is not None:
            user.update(self._format_user_permissions(user_name, permissions))

        self._update_user_connection_limits(user, connection_limits)
        dbs = user.get('dbs', {})
        if global_permissions is not None:
            special_permissions = set(SPECIAL_GLOBAL_PRIVILEGES) & set(global_permissions_inplace)
            dbs.update({'*': sorted(global_permissions + list(special_permissions))})

        if plugin:
            user.update({'plugin': plugin})

    def set_random_password(self, user_name: str) -> None:
        """
        Resets users password to random one. For system users.
        """
        if user_name not in self._pillar_users:
            raise UserNotExistsError(user_name)
        user = self._pillar_users[user_name]
        # https://bugs.mysql.com/bug.php?id=43439
        password_len = 32 if user_name == 'repl' else None
        user.update({'password': gen_encrypted_password(password_len)})

    def delete_user(self, user_name: str) -> None:
        """
        Delete user.
        """
        self._ensure_external_user_exists(user_name)
        user = self._get_external_user(user_name)
        self._ensure_allow_edit(user_name, user)

        del self._pillar_users[user_name]

    def get_user_databases(self, user_name: str) -> List:
        """
        Get user databases list.
        """
        if user_name not in self._pillar_users:
            raise UserNotExistsError(user_name)

        return list(self._get_external_user(user_name).get('dbs', {}).keys())

    def add_user_permission(self, user_name: str, permission: Dict, lower_case_table_names: int) -> None:
        """
        Add user permission.
        """
        db_name = self._transform_db_name(permission['database_name'], lower_case_table_names)
        if not self.database_exists(db_name, lower_case_table_names):
            raise DatabaseNotExistsError(db_name)

        user = self._get_external_user(user_name)
        self._ensure_allow_edit(user_name, user)

        old_roles = []
        for key, value in user['dbs'].items():
            transformed_key = key.lower() if lower_case_table_names else key

            if transformed_key == db_name:
                old_roles = value
                break

        old_roles_set = set(old_roles)
        new_roles = [role for role in permission['roles'] if role not in old_roles_set]
        user['dbs'].update({db_name: old_roles + new_roles})

    def delete_user_permission(self, user_name: str, permission: Dict, lower_case_table_names: int) -> List:
        """
        Add user permission.
        """
        db_name = self._transform_db_name(permission['database_name'], lower_case_table_names)
        if not self.database_exists(db_name, lower_case_table_names):
            raise DatabaseNotExistsError(db_name)

        user = self._get_external_user(user_name)
        self._ensure_allow_edit(user_name, user)

        old_roles = None
        for key, value in user['dbs'].items():
            transformed_key = key.lower() if lower_case_table_names else key

            if transformed_key == db_name:
                old_roles = value
                origin_db_name = key
                break

        if old_roles is None:
            raise DatabaseAccessNotExistsError(user_name, db_name)

        delete_roles_set = set(permission['roles'])

        if not delete_roles_set & set(old_roles):
            raise DatabaseRolesNotExistError(user_name, permission['roles'], db_name)

        result_roles = [role for role in old_roles if role not in delete_roles_set]
        if result_roles:
            user['dbs'][origin_db_name] = result_roles
        else:
            user['dbs'].pop(origin_db_name)
        return result_roles

    def _get_external_user(self, user_name: str) -> Dict:
        user = self._pillar_users.get(user_name)
        if not user or user_name in INTERNAL_USERS:
            raise UserNotExistsError(user_name)

        return user

    def _ensure_external_user_exists(self, user_name: str) -> None:
        self._get_external_user(user_name)

    def _ensure_allow_edit(self, user_name: str, user: dict, password: str = None, plugin: str = None) -> None:
        if 'origin' in user and user['origin'] == 'idm':
            raise IdmUserEditError(user_name)

        # we cannot edit pwd and plugin of IDM system users
        if user_name in SYSTEM_IDM_USERS:
            if password:
                raise IdmSysUserPropertyEditError(user_name, 'password')
            if plugin:
                raise IdmSysUserPropertyEditError(user_name, 'plugin')

    def _transform_db_name(self, db_name: str, lower_case_table_names: int) -> str:
        return db_name.lower() if lower_case_table_names else db_name

    def _transform_db_names(self, db_names: list[str], lower_case_table_names: int) -> list[str]:
        if lower_case_table_names:
            return [db.lower() for db in db_names]

        return db_names

    def database_exists(self, db_name: str, lower_case_table_names: int) -> Any:
        """
        Check if db exists
        """
        db_name = self._transform_db_name(db_name, lower_case_table_names)
        db_names = self._transform_db_names(self.database_names, lower_case_table_names)

        return db_name in db_names

    def _get_default_user_permissions(self) -> List:
        permissions = [
            {
                'database_name': db_name,
                'roles': deepcopy(DEFAULT_ROLES),
            }
            for db_name in self.database_names
        ]
        return permissions

    @staticmethod
    def _validate_user_plugin(plugin: str, version: Version):
        if plugin and version.to_string() == '5.7' and plugin not in AUTH_PLUGINS_5_7:
            raise DbaasClientError('Unsupported in mysql 5.7 plugin value: {0}'.format(plugin))

    @staticmethod
    def _format_user_secrets(password: str) -> Dict:
        if isinstance(password, dict) and 'data' in password and 'encryption_version' in password:
            return {'password': password}
        return {'password': encrypt(password)}

    @staticmethod
    def _update_user_connection_limits(user: Dict, connection_limits: Dict) -> None:
        if not connection_limits:
            return
        if 'connection_limits' not in user:
            user['connection_limits'] = connection_limits
            return
        user['connection_limits'].update(connection_limits)

    def _format_user_permissions(self, user_name: str, permissions: List[Dict]) -> Dict:
        dbs = {}  # type: Dict
        for permission in permissions:
            db_name = permission['database_name']
            if db_name not in self.database_names and db_name != '*':
                raise DatabaseNotExistsError(db_name)
            if db_name in dbs and db_name != '*':
                raise DatabaseAccessExistsError(user_name, db_name)
            dbs.update({db_name: permission['roles']})

        return {'dbs': dbs}

    @property
    def pillar_users(self) -> Dict:
        """
        Users without any reformat
        """
        return deepcopy(self._pillar_users)

    @property
    def _pillar_users(self) -> Dict:
        return self._get_mysql_data('users')

    def databases(self, cluster_id: str) -> List[Dict]:
        """
        Return list of database objects conforming to MysqlDatabaseSchema.
        """
        return [
            {
                'cluster_id': cluster_id,
                'name': name,
            }
            for name in self.database_names
        ]

    def database(self, cluster_id: str, db_name: str) -> Dict:
        """
        Return database object conforming to MysqlDatabaseSchema.
        """
        if db_name not in self.database_names:
            raise DatabaseNotExistsError(db_name)

        return {'cluster_id': cluster_id, 'name': db_name}

    def add_database(self, database_spec: Dict, lower_case_table_names: int) -> str:
        """
        Add database, returns DB name (possibly modified)
        """
        db_name = self._transform_db_name(database_spec['name'], lower_case_table_names)
        db_names = self._transform_db_names(self.database_names, lower_case_table_names)

        if db_name in db_names:
            raise DatabaseExistsError(db_name)

        db_names.append(db_name)
        self.database_names = db_names

        return db_name

    def delete_database(self, db_name: str, lower_case_table_names: int) -> str:
        """
        Delete database
        """
        db_name = self._transform_db_name(db_name, lower_case_table_names)
        db_names = self._transform_db_names(self.database_names, lower_case_table_names)

        if db_name not in db_names:
            raise DatabaseNotExistsError(db_name)

        for options in self._pillar_users.values():
            if lower_case_table_names:
                keys = [k.lower() for k in options['dbs'] if k.lower() == db_name]
                for k in keys:
                    options['dbs'].pop(k)
            else:
                if db_name in options['dbs']:
                    options['dbs'].pop(db_name)

        self.database_names = [db for db in db_names if db != db_name]

        return db_name

    @property
    def database_names(self) -> List[str]:
        """
        Get database names.
        """
        return self._get_mysql_data('databases')[:]

    @database_names.setter
    def database_names(self, databases: List[str]) -> None:
        self._set_mysql_data('databases', databases)

    @property
    def mysql_version(self) -> str:
        """
        Get version (in human redable format)
        """
        try:
            return self._pillar['data']['mysql']['version']['major_human']
        except KeyError as exc:
            raise UninitalizedPillarSection('data.mysql.version is uninitialized') from exc

    @mysql_version.setter
    def mysql_version(self, version: str) -> None:
        """
        Set version
        """
        self._pillar['data']['mysql']['version'] = {
            'major_human': version,
            'major_num': Version.load(version).to_num(),
        }

    @property
    def version(self) -> Version:
        """
        Get version as Version object
        """
        return Version.load(self.mysql_version)

    @property
    def perf_diag(self) -> Dict:
        """
        Returns perf_diag
        """
        return self._pillar['data'].get('perf_diag', {})

    @property
    def sox_audit(self) -> bool:
        return self._pillar['data'].get('sox_audit', False)

    @sox_audit.setter
    def sox_audit(self, sox_audit):
        self._pillar['data']['sox_audit'] = sox_audit

    def update_perf_diag(self, perf_diag: Dict) -> None:
        """
        Updates perf_diag
        """
        if not perf_diag:
            return
        self._pillar['data'].setdefault('perf_diag', {})
        merge_dict(self._pillar['data']['perf_diag'], perf_diag)


def make_new_pillar(environment) -> MySQLPillar:
    """
    Return MySQL pillar for new cluster
    """
    pillar = copy.deepcopy(current_app.config['DEFAULT_PILLAR_TEMPLATE'][MY_CLUSTER_TYPE])
    env_pillar = current_app.config['ENVCONFIG'][MY_CLUSTER_TYPE][environment]
    merge_dict(pillar, env_pillar)
    return MySQLPillar(pillar)


def get_cluster_pillar(cluster: dict) -> MySQLPillar:
    """
    Return MySQL pillar by cluster
    """
    return MySQLPillar(cluster['value'])
