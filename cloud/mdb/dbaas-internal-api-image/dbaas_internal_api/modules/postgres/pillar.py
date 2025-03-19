"""
Postgres pillar generator
"""
import copy
from typing import Any, Dict, List, Optional

from flask import current_app

from . import utils
from ...core.base_pillar import BackupAndAccessPillar
from ...core.crypto import gen_encrypted_password
from ...core.exceptions import (
    DatabaseAccessExistsError,
    DatabaseAccessNotExistsError,
    DatabaseExistsError,
    DatabaseNotExistsError,
    UserExistsError,
    UserInvalidError,
    UserNotExistsError,
    IncorrectTemplateError,
)
from ...utils.helpers import merge_dict
from ...utils.pillar import SubPillar
from .constants import DBNAME_WILDCARD, MY_CLUSTER_TYPE
from .types import Database, UserWithPassword


class _PostgresSubPillar(SubPillar):
    """
    Holds link to pillar part

    Subclass must redefine _path attribute
    """

    def __init__(self, parent_pillar: Dict, parent: 'PostgresqlClusterPillar') -> None:
        super().__init__(parent_pillar, parent)


class MalformedPostgresqlPillar(Exception):
    """
    We got unexpected data in pillar
    """


class _PerfDiagPillar(_PostgresSubPillar):
    """
    PerfDiag pillar
    """

    __slots__ = []  # type: List[str]
    _path = ['data', 'perf_diag']

    def update(self, config: Dict) -> None:
        """
        Rewrites perf_diag enable option
        """
        self._pillar_part.update(config)

    def get(self) -> Dict:
        """
        Get perf_diag enable option value
        """
        res = {'enable': self._pillar_part.get('enable', False)}
        if self._pillar_part.get('pgss_sample_period') is not None:
            res['pgss_sample_period'] = self._pillar_part.get('pgss_sample_period')
        if self._pillar_part.get('pgsa_sample_period') is not None:
            res['pgsa_sample_period'] = self._pillar_part.get('pgsa_sample_period')
        return res


class _PgSyncPillar(_PostgresSubPillar):
    """
    PGSync pillar
    """

    __slots__ = []  # type: List[str]
    _path = ['data', 'pgsync']

    def get_zk_hosts(self) -> List[str]:
        """
        Returns list of zk hosts
        """
        return self._pillar_part['zk_hosts'].split(',')

    def get_zk_hosts_as_str(self) -> str:
        """
        Return zk hosts str format
        Mostly used as worker task_arg
        """
        return self._pillar_part['zk_hosts']

    def set_zk_id_and_hosts(self, zk_id: str, hosts: List[str]) -> None:
        """
        Retwrites zk_id and zk_hosts in pillar
        """
        self._pillar_part['zk_id'] = zk_id
        self._pillar_part['zk_hosts'] = ','.join(hosts)

    def set_autofailover(self, autofailover: bool) -> None:
        """
        Rewrites autofailover option
        """
        self._pillar_part['autofailover'] = autofailover

    def get_autofailover(self) -> bool:
        """
        Get autofailover option value
        """
        return self._pillar_part.get('autofailover', True)


def _db_from_pillar(db: Dict[str, Dict]) -> Database:
    if len(db) != 1:
        raise MalformedPostgresqlPillar('Got unmanaged_dbs with unexpected items count {0}'.format(db))
    name = list(db.keys())[0]
    props = db[name]
    return Database(
        name=name,
        owner=props['user'],
        extensions=copy.deepcopy(props.get('extensions', [])),
        lc_collate=props.get('lc_collate', 'C'),
        lc_ctype=props.get('lc_ctype', 'C'),
        template=props.get('template', ''),
    )


def _db_to_pillar(db: Database) -> Dict[str, Dict]:
    result = {
        db.name: {
            'user': db.owner,
            'extensions': db.extensions,
            'lc_collate': db.lc_collate,
            'lc_ctype': db.lc_ctype,
        },
    }
    if db.template:
        result[db.name]['template'] = db.template
    return result


class _DatabasesPillar(_PostgresSubPillar):
    """
    Unmannaged databases pillar
    """

    __slots__ = []  # type: List[str]
    _path = ['data']
    _dbs_key = 'unmanaged_dbs'

    @property
    def __dbs(self) -> List[Dict[str, Dict]]:
        # Little typing hint
        # _pillar_part annotated as Dict,
        # but here we need List[Dict]
        return self._pillar_part[self._dbs_key]

    def database(self, db_name: str) -> Database:
        """
        Returns database object by name.
        """
        for db in self.__dbs:
            if db_name in db:
                return _db_from_pillar(db)
        raise DatabaseNotExistsError(db_name)

    def get_databases(self) -> List[Database]:
        """
        Returns databases list.
        """
        return [_db_from_pillar(d) for d in self.__dbs]

    def get_names(self) -> List[str]:
        """
        Returns database names.
        """
        return [_db_from_pillar(d).name for d in self.__dbs]

    def assert_database_exists(self, db_name: str, allow_wildcard: bool = False) -> None:
        """
        Checks if database exists. Raises DatabaseNotExistsError otherwise.
        """
        if db_name in self.get_names():
            return
        if allow_wildcard and db_name == DBNAME_WILDCARD:
            return
        raise DatabaseNotExistsError(db_name)

    def assert_correct_template(self, db: Database) -> None:
        if not db.template:
            return
        self.assert_database_exists(db.template)
        for template_db in self.get_databases():
            if template_db.name == db.template:
                if db.lc_collate != template_db.lc_collate:
                    raise IncorrectTemplateError(db.name, template_db.name, 'LC_COLLATE')
                if db.lc_ctype != template_db.lc_ctype:
                    raise IncorrectTemplateError(db.name, template_db.name, 'LC_CTYPE')
                break

    def assert_database_not_exists(self, db_name: str) -> None:
        """
        Checks if database not exists. Raises DatabaseExistsError otherwise.
        """
        if db_name not in self.get_names():
            return
        raise DatabaseExistsError(db_name)

    def get_owners_names(self) -> List[str]:
        """
        Returns database owners names.
        """
        return [_db_from_pillar(d).owner for d in self.__dbs]

    def add_database(self, db: Database) -> None:
        """
        Adds database to pillar.
        """
        self.assert_database_not_exists(db.name)
        self.assert_correct_template(db)
        self._parent.pgusers.assert_user_exists(db.owner)
        self.__dbs.append(_db_to_pillar(db))
        self._parent.pgusers.add_user_permission(db.owner, {'database_name': db.name})

    def add_database_initial(self, db: Database) -> None:
        """
        Adds database to pillar without checking owner.
        For create/restore operations.
        """
        self.assert_database_not_exists(db.name)
        self.__dbs.append(_db_to_pillar(db))

    def update_database(self, db: Database) -> None:
        """
        Updates database.
        """
        self.assert_database_exists(db.name)
        self._parent.pgusers.assert_user_exists(db.owner)
        for _, _db in enumerate(self.__dbs):
            if _db_from_pillar(_db).name == db.name:
                merge_dict(_db, _db_to_pillar(db))
                break

    def update_database_name(self, db: Database, new_db_name: str) -> None:
        utils.validate_database_name(new_db_name)

        self.assert_database_exists(db.name)
        self.assert_database_not_exists(new_db_name)

        db_dict = db._asdict()
        db_dict['name'] = new_db_name

        for idx, _db in enumerate(self.__dbs):
            if _db_from_pillar(_db).name == db.name:
                self.__dbs.pop(idx)
                self.__dbs.append(_db_to_pillar(Database(**db_dict)))
                break

        for user_name in self._parent.pgusers.get_names():
            if self._parent.pgusers.user_has_permission(user_name, db.name):
                # pylint: disable=protected-access
                self._parent.pgusers._delete_user_permission(user_name, db.name)
                self._parent.pgusers.add_user_permission(user_name, {'database_name': new_db_name})

    def delete_database(self, db_name: str) -> None:
        """
        Deletes database from cluster
        """
        self.assert_database_exists(db_name)

        for user_name in self._parent.pgusers.get_names():
            if self._parent.pgusers.user_has_permission(user_name, db_name):
                # pylint: disable=protected-access
                self._parent.pgusers._delete_user_permission(user_name, db_name)

        for idx, _db in enumerate(self.__dbs):
            if _db_from_pillar(_db).name == db_name:
                self.__dbs.pop(idx)
                break


def _user_from_pillar(user_name: str, user_pillar: Dict[str, Any]) -> UserWithPassword:
    return UserWithPassword(
        name=user_name,
        encrypted_password=user_pillar['password'],
        connect_dbs=user_pillar.get('connect_dbs', []),
        conn_limit=user_pillar.get('conn_limit'),
        settings=user_pillar.get('settings', {}),
        login=user_pillar.get('login', True),
        grants=user_pillar.get('grants', []),
    )


def _user_to_pillar(user: UserWithPassword) -> Dict[str, Dict]:
    return {
        user.name: {
            'password': user.encrypted_password,
            'connect_dbs': user.connect_dbs,
            'conn_limit': user.conn_limit,
            'settings': user.settings,
            'login': user.login,
            'grants': user.grants,
        },
    }


class _PGUsersPillar(_PostgresSubPillar):
    """
    Postgres users pillar
    """

    _path = ['data', 'config', 'pgusers']

    def user(self, user_name: str) -> UserWithPassword:
        """
        Returns user by name.
        """
        if user_name not in self._pillar_users:
            raise UserNotExistsError(user_name)
        return _user_from_pillar(user_name, self._pillar_users[user_name])

    def public_user(self, user_name: str) -> UserWithPassword:
        """
        Returns not-internal user by name.
        """
        if user_name not in self._pillar_users:
            raise UserNotExistsError(user_name)
        if not utils.is_public_user(user_name, self._pillar_users[user_name]):
            raise UserNotExistsError(user_name)
        return _user_from_pillar(user_name, self._pillar_users[user_name])

    def get_users(self) -> List[UserWithPassword]:
        """
        Returns list of user.
        """
        return [_user_from_pillar(name, params) for name, params in sorted(self._pillar_users.items())]

    def get_public_users(self) -> List[UserWithPassword]:
        """
        Returns list of not-internal user.
        """
        return [
            _user_from_pillar(name, params)
            for name, params in sorted(self._pillar_users.items())
            if utils.is_public_user(name, params)
        ]

    def get_default_users(self) -> List[UserWithPassword]:
        """
        Returns list of system user.
        """
        return [
            _user_from_pillar(name, params)
            for name, params in sorted(self._pillar_users.items())
            if not _user_from_pillar(name, params).login and not utils.is_public_user(name, params)
        ]

    def get_names(self) -> List[str]:
        """
        Returns names of existing users
        """
        return list(self._pillar_part.keys())

    def assert_user_exists(self, name: str) -> None:
        """
        Checks if user exists. Raises UserNotExistsError otherwise.
        """
        if name in self._pillar_users:
            return
        if self._parent.sox_audit and name in ['reader', 'writer']:
            return
        raise UserNotExistsError(name)

    def assert_user_not_exists(self, name: str) -> None:
        """
        Checks if user not exists. Raises UserExistsError otherwise.
        """
        if name not in self._pillar_users:
            return
        raise UserExistsError(name)

    def add_user(self, user: UserWithPassword) -> None:
        """
        Adds user.
        """
        self.assert_user_not_exists(user.name)

        for db_name in user.connect_dbs:
            self._parent.databases.assert_database_exists(db_name, allow_wildcard=True)
        for grant_role in user.grants:
            self.assert_user_exists(grant_role)
        user_pillar = utils.get_user_pillar(user)
        self._pillar_users[user.name] = user_pillar

    def update_user(self, user: UserWithPassword) -> None:
        """
        Updates user.
        """
        self.assert_user_exists(user.name)
        for db_name in user.connect_dbs:
            self._parent.databases.assert_database_exists(db_name, allow_wildcard=True)
        for grant_role in user.grants:
            self.assert_user_exists(grant_role)
        merge_dict(self._pillar_users, _user_to_pillar(user))

    def set_random_password(self, user_name: str) -> None:
        """
        Resets users password to random one. For system users.
        """
        self.assert_user_exists(user_name)
        user = self._pillar_users[user_name]
        # NOTE: initdb --pwfile  reads only firs 100 bytes of file
        # src/bin/initdb/initdb.c:1568
        password_len = 90 if user_name == 'postgres' else None
        user.update({'password': gen_encrypted_password(password_len)})

    def user_has_permission(self, user_name: str, db_name: str) -> bool:
        """
        Check if user has permission to specified database.
        """
        self.assert_user_exists(user_name)
        user_pillar = self._pillar_users[user_name]
        return db_name in user_pillar.get('connect_dbs', [])

    def add_user_permission(self, user_name: str, permission: Dict) -> None:
        """
        Adds user permission.
        """
        self.assert_user_exists(user_name)
        db_name = permission['database_name']
        self._parent.databases.assert_database_exists(db_name, allow_wildcard=True)
        user_pillar = self._pillar_users[user_name]

        if db_name in user_pillar.get('connect_dbs', []):
            raise DatabaseAccessExistsError(user_name, db_name)

        user_pillar.setdefault('connect_dbs', [])
        user_pillar['connect_dbs'].append(db_name)

    def _delete_user_permission(self, user_name: str, db_name: str) -> None:
        self._pillar_part[user_name]['connect_dbs'].remove(db_name)

    def delete_user_permission(self, user_name: str, db_name: str) -> None:
        """
        Deletes user permission.
        """
        self.assert_user_exists(user_name)
        self._parent.databases.assert_database_exists(db_name, allow_wildcard=True)
        user_pillar = self._pillar_users[user_name]

        if db_name not in user_pillar.get('connect_dbs', []):
            raise DatabaseAccessNotExistsError(user_name, db_name)

        if db_name != DBNAME_WILDCARD:
            db = self._parent.databases.database(db_name)
            if user_name == db.owner:
                raise UserInvalidError(
                    "Unable to revoke permission from " + "owner of database '{dbname}'".format(dbname=db_name)
                )

        self._delete_user_permission(user_name, db_name)

    def delete_user(self, user_name: str) -> None:
        """
        Deletes user and revokes self from all.
        """
        self.assert_user_exists(user_name)
        for db in self._parent.databases.get_databases():
            if user_name == db.owner:
                raise UserInvalidError("Unable to delete owner of database" " '{name}'".format(name=db.name))

        for user in self._pillar_users.keys():
            if user_name in self._pillar_users[user].get('grants', []):
                self._pillar_users[user]['grants'].remove(user_name)

        del self._pillar_users[user_name]

    @property
    def _pillar_users(self) -> Dict:
        return self._pillar_part


class _PgBouncerPillar(_PostgresSubPillar):
    _path = ['data', 'pgbouncer']

    def add_custom_user_param(self, param: str) -> None:
        """
        Add custom pgbouncer parameter
        """
        self._pillar_part['custom_user_params'].append(param)


class _PGConfigPillar(_PostgresSubPillar):
    _path = ['data', 'config']

    @property
    def max_connections(self) -> Optional[int]:
        """
        Get max_connections
        """
        return self._pillar_part.get('max_connections')

    def update_config(self, config: Dict) -> None:
        """
        Updates data.config section

        Updates only top-level-keys
        """
        # write it only for create and restore cluster cases
        assert 'pgusers' not in config
        self._pillar_part.update(config)

    def get_config(self) -> Dict:
        """
        Returns config as a dict
        """
        return copy.deepcopy(self._pillar_part)

    def merge_config(self, config: Dict) -> None:
        """
        Updates data.config section recurisively
        """
        assert 'pgusers' not in config
        merge_dict(self._pillar_part, config)


class _PGWalgPillar(_PostgresSubPillar):
    """
    WAL-G pillar
    """

    __slots__ = []  # type: List[str]
    _path = ['data', 'walg']

    def update(self, config: Dict) -> None:
        self._pillar_part.update(config)

    def get(self) -> Dict:
        """
        Returns WAL-G options as a dict
        """
        return copy.deepcopy(self._pillar_part)


class PostgresqlClusterPillar(BackupAndAccessPillar):
    """
    Postgresql cluster pillar.
    """

    __slots__ = ['pgsync', 'perf_diag', 'pg', 'databases', 'pgusers', 'pgbouncer', 'config', 'walg']

    def __init__(self, pillar: Dict) -> None:
        super().__init__(pillar)
        self.pgsync = _PgSyncPillar(self._pillar, self)
        self.perf_diag = _PerfDiagPillar(self._pillar, self)
        self.databases = _DatabasesPillar(self._pillar, self)
        self.pgusers = _PGUsersPillar(self._pillar, self)
        self.pgbouncer = _PgBouncerPillar(self._pillar, self)
        self.config = _PGConfigPillar(self._pillar, self)
        self.walg = _PGWalgPillar(self._pillar, self)

    @property
    def sox_audit(self) -> bool:
        return self._pillar['data'].get('sox_audit', False)

    @sox_audit.setter
    def sox_audit(self, sox_audit):
        self._pillar['data']['sox_audit'] = sox_audit


def make_new_pillar() -> PostgresqlClusterPillar:  # pragma: no cover
    """
    Create new pillar from App configuration
    """
    return PostgresqlClusterPillar(copy.deepcopy(current_app.config['DEFAULT_PILLAR_TEMPLATE'][MY_CLUSTER_TYPE]))


def get_cluster_pillar(cluster: Dict) -> PostgresqlClusterPillar:
    """
    Creates PostgresqlClusterPillar object from pillar dict
    """
    return PostgresqlClusterPillar(cluster['value'])
