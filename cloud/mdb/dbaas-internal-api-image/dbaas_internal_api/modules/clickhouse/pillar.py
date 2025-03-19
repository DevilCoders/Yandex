"""
ClickHouse pillar generator
"""
import hashlib
from copy import deepcopy
from typing import Any, Dict, Optional, Sequence, Set, Tuple, Type, Union  # noqa

from ...core.base_pillar import BackupAndAccessPillar, BasePillar
from ...core.crypto import encrypt, gen_encrypted_password, gen_encrypted_password_with_hash
from ...core.exceptions import (
    DatabaseAccessExistsError,
    DatabaseAccessNotExistsError,
    DatabaseExistsError,
    DatabaseNotExistsError,
    DbaasClientError,
    DbaasError,
    FormatSchemaExistsError,
    FormatSchemaNotExistsError,
    MlModelExistsError,
    MlModelNotExistsError,
    UserExistsError,
    UserNotExistsError,
    ShardGroupNotExistsError,
    PreconditionFailedError,
)
from ...utils.cluster.get import get_subcluster
from ...utils.helpers import merge_dict, remove_none_dict
from ...utils.types import Pillar
from ...utils.version import Version, get_full_version
from .constants import DEFAULT_SHARD_WEIGHT, MY_CLUSTER_TYPE
from .traits import ClickhouseRoles
from .constants import ZK_ACL_USER_SUPER, ZK_ACL_USER_CLICKHOUSE, ZK_ACL_USER_BACKUP, MDB_BACKUP_ADMIN_USER


class DuplicatedDictionaryNamesError(DbaasClientError):
    """
    Error raised on attempt to configure multiple external dictionaries with the same name.
    """

    def __init__(self, name):
        super().__init__(f'Cluster cannot have multiple external dictionaries with the same name (\'{name}\')')


class DictionaryAlreadyExistsError(DbaasClientError):
    """
    Error raised on attempt to configure external dictionary with name that already exists.
    """

    code = 409

    def __init__(self, name):
        super().__init__(f'External dictionary \'{name}\' already exists')


class DictionaryNotExistsError(DbaasClientError):
    """
    Error raised on attempt to delete non-existent external dictionary.
    """

    code = 404

    def __init__(self, name):
        super().__init__(f'External dictionary \'{name}\' does not exist')


class DuplicatedGraphiteRollupNamesError(DbaasClientError):
    """
    Error raised on attempt to configure multiple Graphite rollup settings with the same name.
    """

    def __init__(self, name):
        super().__init__(f'Cluster cannot have multiple Graphite rollup settings with the same name (\'{name}\')')


class DuplicatedKafkaTopicNamesError(DbaasClientError):
    """
    Error raised on attempt to configure multiple Kafka topics with the same name.
    """

    def __init__(self, name):
        super().__init__(f'Configuration cannot have multiple Kafka topics with the same name (\'{name}\')')


class ClickhousePillar(BackupAndAccessPillar):
    """
    ClickHouse pillar.
    """

    # pylint: disable=too-many-public-methods

    def __init__(self) -> None:
        super().__init__(
            {
                'data': {
                    'clickhouse': {
                        'config': {
                            'merge_tree': {},
                        },
                        'databases': [],
                        'users': {},
                        'zk_users': {},
                        'models': {},
                        'format_schemas': {},
                        'system_users': {},
                    },
                },
            }
        )

    @classmethod
    def make(cls) -> "ClickhousePillar":
        """
        Create new pillar.
        """
        # pylint: disable=protected-access
        pillar = ClickhousePillar()
        pillar._ch_data['interserver_credentials'] = {
            'user': 'interserver',
            'password': gen_encrypted_password(),
        }
        pillar._ch_data['config']['merge_tree']['enable_mixed_granularity_parts'] = True
        return pillar

    @classmethod
    def load(cls, pillar_data: Pillar) -> "ClickhousePillar":
        """
        Construct pillar object from pillar data.
        """
        # pylint: disable=protected-access
        pillar = ClickhousePillar()
        pillar._pillar = pillar_data
        return pillar

    @property
    def _ch_data(self) -> dict:
        return self._pillar['data']['clickhouse']

    @property
    def cluster_name(self) -> Optional[str]:
        """
        Get cluster name.
        """
        return self._ch_data.get('cluster_name')

    @cluster_name.setter
    def cluster_name(self, value: str) -> None:
        self._ch_data['cluster_name'] = value

    def database(self, cluster_id: str, db_name: str) -> Dict:
        """
        Return database object conforming to ClickhouseDatabaseSchema.
        """
        if db_name not in self.database_names:
            raise DatabaseNotExistsError(db_name)

        return {'cluster_id': cluster_id, 'name': db_name}

    def databases(self, cluster_id: str) -> Sequence[Dict]:
        """
        Return list of database objects conforming to ClickhouseDatabaseSchema.
        """
        return [
            {
                'cluster_id': cluster_id,
                'name': name,
            }
            for name in self.database_names
        ]

    def add_database(self, db_spec: Dict) -> None:
        """
        Add database
        """
        new_db_name = db_spec['name']
        if new_db_name in self._ch_data['databases']:
            raise DatabaseExistsError(new_db_name)

        self._ch_data['databases'].append(new_db_name)

    def add_databases(self, db_specs: Sequence[Dict]) -> None:
        """
        Add databases
        """
        for db_spec in db_specs:
            self.add_database(db_spec)

    def delete_database(self, db_name: str) -> None:
        """
        Delete database
        """
        if db_name not in self.database_names:
            raise DatabaseNotExistsError(db_name)

        for options in self._pillar_users.values():
            options['databases'].pop(db_name, None)

        self.database_names = [db for db in self.database_names if db != db_name]

    @property
    def database_names(self) -> Sequence[str]:
        """
        Get database names.
        """
        return self._ch_data['databases']

    @database_names.setter
    def database_names(self, databases: Sequence[str]) -> None:
        self._ch_data['databases'] = databases

    def user(self, cluster_id: str, user_name: str) -> Dict[str, Any]:
        """
        Return user object conforming to ClickhouseUserSchema.
        """
        try:
            params = self._pillar_users[user_name]
            return self._to_user(cluster_id, user_name, params)
        except KeyError:
            raise UserNotExistsError(user_name)

    def users(self, cluster_id: str) -> Sequence[Dict[str, Any]]:
        """
        Return list of user objects conforming to ClickhouseUserSchema.
        """
        return [self._to_user(cluster_id, name, params) for name, params in sorted(self._pillar_users.items())]

    @staticmethod
    def _to_user(cluster_id: str, name: str, params: Dict) -> Dict:
        permissions = []
        for db_name, db in params.get('databases', {}).items():
            permission = {'database_name': db_name}
            for table_name, table in db.get('tables', {}).items():
                table_filter = table.get('filter')
                if not table_filter:
                    continue

                permission.setdefault('data_filters', []).append(
                    {
                        'table_name': table_name,
                        'filter': table_filter,
                    }
                )

            permissions.append(permission)

        return {
            'cluster_id': cluster_id,
            'name': name,
            'permissions': permissions,
            'settings': params.get('settings', {}),
            'quotas': params.get('quotas', []),
        }

    def add_user(self, user_spec: Dict) -> None:
        """
        Add user.
        """
        user_name = user_spec['name']
        if user_name in self._pillar_users:
            raise UserExistsError(user_name)

        user = {}  # type: Dict
        user.update(self._format_user_secrets(user_spec['password']))
        user.update(self._format_user_databases(user_spec['permissions']))
        user['settings'] = remove_none_dict(user_spec.get('settings', {}))
        user['quotas'] = user_spec.get('quotas', [])

        self._pillar_users[user_name] = user

    def add_users(self, user_specs: Sequence[Dict]) -> None:
        """
        Add users.
        """
        for user_spec in user_specs:
            self.add_user(user_spec)

    def update_user(
        self,
        user_name: str,
        password: Optional[str],
        permissions: Optional[Sequence[Dict]],
        settings: Optional[dict],
        quotas: Optional[Sequence[Dict]],
    ) -> None:
        """
        Update user.
        """
        if user_name not in self._pillar_users:
            raise UserNotExistsError(user_name)

        user = self._pillar_users[user_name]

        if password is not None:
            user.update(self._format_user_secrets(password))

        if permissions is not None:
            user.update(self._format_user_databases(permissions))

        if settings is not None:
            user_settings = user.get('settings', {})
            merge_dict(user_settings, settings)
            user['settings'] = remove_none_dict(user_settings)

        if quotas is not None:
            user.update({'quotas': quotas})

    def add_user_permission(self, user_name: str, permission: Dict) -> None:
        """
        Add user permission.
        """
        if user_name not in self._pillar_users:
            raise UserNotExistsError(user_name)

        db_name = permission['database_name']

        if db_name not in self.database_names:
            raise DatabaseNotExistsError(db_name, 422)

        user = self._pillar_users[user_name]

        if db_name in user['databases']:
            raise DatabaseAccessExistsError(user_name, db_name)

        user['databases'][db_name] = self._format_user_database(permission)

    def delete_user_permission(self, user_name: str, db_name: str) -> None:
        """
        Delete user permission.
        """
        if user_name not in self._pillar_users:
            raise UserNotExistsError(user_name)

        if db_name not in self.database_names:
            raise DatabaseNotExistsError(db_name, 422)

        user = self._pillar_users[user_name]

        if db_name not in user['databases']:
            raise DatabaseAccessNotExistsError(user_name, db_name)

        user['databases'].pop(db_name)

    def delete_user(self, user_name: str) -> None:
        """
        Delete user.
        """
        if user_name not in self._pillar_users:
            raise UserNotExistsError(user_name)

        del self._pillar_users[user_name]

    @staticmethod
    def _format_user_secrets(password: str) -> Dict:
        return {
            'password': encrypt(password),
            'hash': encrypt(hashlib.sha256(password.encode('utf-8')).hexdigest()),
        }

    @staticmethod
    def _format_user_database(permission: Dict) -> Dict:
        if 'data_filters' not in permission:
            return {}

        return {
            'tables': {
                data_filter['table_name']: {
                    'filter': data_filter['filter'],
                }
                for data_filter in permission['data_filters']
            },
        }

    def _format_user_databases(self, permissions: Sequence[Dict]) -> Dict:
        databases = {}

        for permission in permissions:
            db_name = permission['database_name']
            if db_name not in self.database_names:
                raise DatabaseNotExistsError(db_name, 422)
            databases[db_name] = self._format_user_database(permission)

        return {'databases': databases}

    @property
    def pillar_users(self) -> Dict:
        """
        Users without any reformat
        """
        return deepcopy(self._pillar_users)

    @pillar_users.setter
    def pillar_users(self, pillar_users: Dict) -> None:
        self._ch_data['users'] = pillar_users

    @property
    def _pillar_users(self) -> Dict:
        return self._ch_data['users']

    @property
    def zk_hosts(self) -> Sequence[str]:
        """
        Return list of ZooKeeper hosts (hostnames).

        Applicable either for legacy ClickHouse clusters with shared ZooKeeper of for CH clusters with enabled Keeper.
        """
        return self._ch_data.get('zk_hosts', [])

    @zk_hosts.setter
    def zk_hosts(self, zk_hosts: Sequence[str]) -> None:
        self._ch_data['zk_hosts'] = zk_hosts

    @property
    def keeper_hosts(self) -> Dict:
        """
        Return dict of ZooKeeper or Clickhouse Keeper hostnames.

        It is mapping with 'hostname -> server_id' for hostname's determinated order in config file and unique server-id values.
        For current clusters zk_hosts should be replaced with keeper_hosts in same AZ-order.
        """
        return self._ch_data.get('keeper_hosts', {})

    @keeper_hosts.setter
    def keeper_hosts(self, keeper_hosts: Dict) -> None:
        self._ch_data['keeper_hosts'] = keeper_hosts

    @property
    def _zk_users(self) -> Dict:
        """
        Return users for ZooKeeper ACL.
        """
        return self._ch_data['zk_users']

    @property
    def _system_users(self) -> Dict:
        """
        Return some system users.
        """
        return self._ch_data['system_users']

    @property
    def version(self) -> Version:
        """
        ClickHouse version.
        """
        return Version.load(self._ch_data['ch_version'], strict=False)

    def set_version(self, version: Version) -> None:
        """
        Set ClickHouse version.
        """
        try:
            full_version = get_full_version(MY_CLUSTER_TYPE, version)
        except Exception:
            full_version = version.string

        self._ch_data['ch_version'] = full_version

    @property
    def config(self) -> dict:
        """
        Return ClickHouse config.
        """
        return deepcopy(self._pillar['data']['clickhouse']['config'])

    def update_config(self, config: dict) -> None:
        """
        Update ClickHouse config.
        """
        dictionaries = config.get('dictionaries', [])
        self._check_names_uniqueness(dictionaries, DuplicatedDictionaryNamesError)
        for dictionary in dictionaries:
            self._format_dictionary(dictionary)

        self._check_names_uniqueness(config.get('graphite_rollup'), DuplicatedGraphiteRollupNamesError)
        self._check_names_uniqueness(config.get('kafka_topics'), DuplicatedKafkaTopicNamesError)

        kafka = config.get('kafka')
        if kafka:
            _set_encrypted_field(
                kafka,
                'sasl_password',
                default=self._pillar['data']['clickhouse']['config'].get('kafka', {}).get('sasl_password'),
            )

        src_topics = self._pillar['data']['clickhouse']['config'].get('kafka_topics', [])
        for kafka_topic in config.get('kafka_topics', []):
            previous_password = _find_previous_value(
                src_topics,
                key=kafka_topic['name'],
                by='name',
                getter=lambda topic: topic.get('settings', {}).get('sasl_password'),
            )
            _set_encrypted_field(kafka_topic['settings'], 'sasl_password', default=previous_password)

        rabbitmq = config.get('rabbitmq')
        if rabbitmq:
            _set_encrypted_field(
                rabbitmq,
                'password',
                default=self._pillar['data']['clickhouse']['config'].get('rabbitmq', {}).get('password'),
            )

        merge_dict(self._pillar['data']['clickhouse']['config'], config)
        _validate_clickhouse_config(self._pillar['data']['clickhouse']['config'])

    def add_dictionary(self, dictionary: dict) -> None:
        """
        Add external dictionary to ClickHouse config.
        """
        config = self._pillar['data']['clickhouse']['config']
        dictionaries = config.get('dictionaries', [])

        self._format_dictionary(dictionary)
        dictionaries.append(dictionary)
        self._check_names_uniqueness(dictionaries, DictionaryAlreadyExistsError)

        config['dictionaries'] = dictionaries

    def delete_dictionary(self, name: str) -> None:
        """
        Remove external dictionary from ClickHouse config.
        """
        dictionaries = self._pillar['data']['clickhouse']['config'].get('dictionaries', [])
        for idx, dictionary in enumerate(dictionaries):
            if dictionary['name'] == name:
                del dictionaries[idx]
                return

        raise DictionaryNotExistsError(name)

    def _format_dictionary(self, dictionary: dict) -> None:
        self._encrypt_dictionary_source(dictionary, 'mysql_source', need_replicas=True)
        self._encrypt_dictionary_source(dictionary, 'clickhouse_source')
        self._encrypt_dictionary_source(dictionary, 'mongodb_source')
        self._encrypt_dictionary_source(dictionary, 'yt_source', field_to_encrypt='token')
        self._encrypt_dictionary_source(dictionary, 'postgresql_source')

    def _encrypt_dictionary_source(
        self, dictionary, source_name: str, field_to_encrypt: str = 'password', need_replicas: bool = False
    ):
        source = dictionary.get(source_name)

        if not source:
            return

        _set_encrypted_field(
            source,
            field_to_encrypt,
            default=self._pillar['data']['clickhouse']['config'].get(source_name, {}).get(field_to_encrypt),
        )

        if not need_replicas:
            return

        src_replicas = self._pillar['data']['clickhouse']['config'].get(source_name, {}).get('replicas', [])
        for replica in source.get('replicas', []):
            previous_password = _find_previous_value(
                src_replicas, key=replica['host'], by='host', getter=lambda r: r.get(field_to_encrypt)
            )
            _set_encrypted_field(replica, field_to_encrypt, default=previous_password)

    @staticmethod
    def _check_names_uniqueness(items: Optional[Sequence[dict]], error: Type[Exception]) -> None:
        names = set()  # type: Set[str]
        for item in items or []:
            name = item['name']
            if name in names:
                raise error(name)

            names.add(name)

    def add_model(self, model_name: str, model_type: str, uri: str) -> None:
        """
        Add Ml model.
        """
        models = self._ch_data.get('models', {})

        if model_name in models:
            raise MlModelExistsError(model_name)

        models[model_name] = {'type': model_type, 'uri': uri}
        self._ch_data['models'] = models

    def update_model(self, model_name: str, uri: Optional[str]) -> None:
        """
        Update ML model.
        """
        if model_name not in self._ch_data.get('models', {}):
            raise MlModelNotExistsError(model_name)

        if uri is not None:
            self._ch_data['models'][model_name]['uri'] = uri

    def delete_model(self, model_name: str) -> None:
        """
        Delete ML model.
        """
        if model_name not in self._ch_data.get('models', {}):
            raise MlModelNotExistsError(model_name)

        del self._ch_data['models'][model_name]

    def model(self, cluster_id: str, model_name: str) -> dict:
        """
        Return ML model object conforming to ClickhouseMlModelSchema.
        """
        try:
            return self._to_model(cluster_id, model_name, self._ch_data['models'][model_name])
        except KeyError:
            raise MlModelNotExistsError(model_name)

    def models(self, cluster_id: str) -> Sequence[dict]:
        """
        Return list of ML model objects conforming to ClickhouseMlModelSchema.
        """
        return [
            self._to_model(cluster_id, name, params) for name, params in sorted(self._ch_data.get('models', {}).items())
        ]

    @staticmethod
    def _to_model(cluster_id: str, name: str, params: dict) -> dict:
        return {
            'cluster_id': cluster_id,
            'name': name,
            'type': params['type'],
            'uri': params['uri'],
        }

    def add_format_schema(self, format_schema_name: str, format_schema_type: str, uri: str) -> None:
        """
        Add format schema.
        """
        schemas = self._ch_data.get('format_schemas', {})
        if format_schema_name in schemas:
            raise FormatSchemaExistsError(format_schema_name)

        schemas[format_schema_name] = {'type': format_schema_type, 'uri': uri}
        self._ch_data['format_schemas'] = schemas

    def update_format_schema(self, format_schema_name: str, uri: Optional[str]) -> None:
        """
        Update format schema.
        """
        if format_schema_name not in self._ch_data.get('format_schemas', {}):
            raise FormatSchemaNotExistsError(format_schema_name)

        if uri is not None:
            self._ch_data['format_schemas'][format_schema_name]['uri'] = uri

    def delete_format_schema(self, format_schema_name: str) -> None:
        """
        Delete format schema.
        """
        if format_schema_name not in self._ch_data.get('format_schemas', {}):
            raise FormatSchemaNotExistsError(format_schema_name)

        del self._ch_data['format_schemas'][format_schema_name]

    def format_schema(self, cluster_id: str, format_schema_name: str) -> dict:
        """
        Return format schema object conforming to ClickhouseFormatSchemaSchema.
        """
        try:
            return self._to_format_schema(
                cluster_id, format_schema_name, self._ch_data['format_schemas'][format_schema_name]
            )
        except KeyError:
            raise FormatSchemaNotExistsError(format_schema_name)

    def format_schemas(self, cluster_id: str) -> Sequence[dict]:
        """
        Return list of format schema objects conforming to ClickhouseFormatSchemaSchema.
        """
        return [
            self._to_format_schema(cluster_id, name, params)
            for name, params in sorted(self._ch_data.get('format_schemas', {}).items())
        ]

    @staticmethod
    def _to_format_schema(cluster_id: str, name: str, params: dict) -> dict:
        return {
            'cluster_id': cluster_id,
            'name': name,
            'type': params['type'],
            'uri': params['uri'],
        }

    def add_shard(self, shard_id: str, weight: int = None) -> None:
        """
        Add shard.
        """
        if weight is None:
            weight = DEFAULT_SHARD_WEIGHT

        self._shards[shard_id] = {'weight': weight}

    def update_shard(self, shard_id: str, weight: int) -> None:
        """
        Update shard.
        """
        self.add_shard(shard_id, weight)

    def get_shard_weight(self, shard_id: str) -> int:
        """
        Return shard weight.
        """
        return self._shards.get(shard_id, {}).get('weight', DEFAULT_SHARD_WEIGHT)

    def delete_shard(self, shard_id: str) -> None:
        """
        Delete shard.
        """
        try:
            del self._shards[shard_id]
        except KeyError:
            pass

    def delete_shard_from_all_groups(self, shard_name: str) -> None:
        """
        Delete shard from all shard groups.
        """
        for group, opts in self.shard_groups.items():
            if shard_name in opts.get('shard_names', []):
                if len(opts['shard_names']) == 1:
                    raise PreconditionFailedError(f'Last shard in shard group "{group}" cannot be removed')
                self.shard_groups[group]['shard_names'].remove(shard_name)

    def shard_group(self, cluster_id: str, shard_group_name: str):
        """
        Get shard group by name.
        """
        if shard_group_name in self.shard_groups:
            group = self.shard_groups[shard_group_name]
            return {
                'cluster_id': cluster_id,
                'name': shard_group_name,
                'description': group.get('description'),
                'shard_names': group.get('shard_names'),
            }
        else:
            return ShardGroupNotExistsError(shard_group_name)

    def add_shard_group(self, shard_group_name: str, shard_group: dict):
        """
        Add shard group.
        """
        self.shard_groups[shard_group_name] = shard_group

    @property
    def _shards(self) -> Dict:
        if 'shards' not in self._ch_data:
            self._ch_data['shards'] = {}

        return self._ch_data['shards']

    @property
    def shard_groups(self) -> Dict:
        if 'shard_groups' not in self._ch_data:
            self._ch_data['shard_groups'] = {}

        return self._ch_data['shard_groups']

    def service_account_id(self):
        return self._pillar['data'].get('service_account_id', None)

    def update_service_account_id(self, service_account_id: str):
        self._pillar['data']['service_account_id'] = service_account_id

    @property
    def cloud_storage(self) -> Dict:
        if self._pillar['data'].get('cloud_storage') is None:
            self._pillar['data']['cloud_storage'] = {}

        return self._pillar['data']['cloud_storage']

    @property
    def cloud_storage_enabled(self) -> bool:
        return self.cloud_storage.get('enabled', False)

    def set_cloud_storage_enabled(self, enabled: bool):
        self.cloud_storage['enabled'] = enabled

    def update_cloud_storage(self, cloud_storage: Dict):
        self._pillar['data']['cloud_storage'] = cloud_storage

    def set_cloud_storage_bucket(self, bucket_name):
        self.cloud_storage['s3'] = {'bucket': bucket_name}

    @property
    def cloud_storage_data_cache_enabled(self) -> bool:
        return self.cloud_storage.get('settings', {}).get('data_cache_enabled', None)

    @property
    def cloud_storage_data_cache_max_size(self) -> int:
        return self.cloud_storage.get('settings', {}).get('data_cache_max_size', None)

    @property
    def cloud_storage_move_factor(self) -> float:
        return self.cloud_storage.get('settings', {}).get('move_factor', None)

    def set_cloud_storage_data_cache_enabled(self, enabled: bool):
        if enabled is not None:
            if self.cloud_storage.get('settings') is None:
                self.cloud_storage['settings'] = {}
            self.cloud_storage['settings']['data_cache_enabled'] = enabled

    def set_cloud_storage_data_cache_max_size(self, size: int):
        if size is not None:
            if self.cloud_storage.get('settings') is None:
                self.cloud_storage['settings'] = {}
            self.cloud_storage['settings']['data_cache_max_size'] = size

    def set_cloud_storage_move_factor(self, move_factor: float):
        if move_factor is not None:
            if self.cloud_storage.get('settings') is None:
                self.cloud_storage['settings'] = {}
            self.cloud_storage['settings']['move_factor'] = move_factor

    @property
    def cloud_storage_bucket(self):
        return self.cloud_storage.get('s3', {}).get('bucket', None)

    @property
    def sql_user_management(self) -> bool:
        return self._ch_data.get('sql_user_management', False)

    def update_sql_user_management(self, sql_user_management: bool):
        self._ch_data['sql_user_management'] = sql_user_management

    @property
    def sql_database_management(self) -> bool:
        return self._ch_data.get('sql_database_management', False)

    def update_sql_database_management(self, sql_database_management: bool):
        self._ch_data['sql_database_management'] = sql_database_management

    @property
    def mysql_protocol(self) -> bool:
        return self._ch_data.get('mysql_protocol', False)

    def update_mysql_protocol(self, mysql_protocol: bool):
        self._ch_data['mysql_protocol'] = mysql_protocol

    @property
    def postgresql_protocol(self) -> bool:
        return self._ch_data.get('postgresql_protocol', False)

    def update_postgresql_protocol(self, postgresql_protocol: bool):
        self._ch_data['postgresql_protocol'] = postgresql_protocol

    @property
    def admin_password(self) -> Optional[dict]:
        return self._ch_data.get('admin_password')

    def update_admin_password(self, admin_password: Union[str, dict, None]):
        if isinstance(admin_password, str):
            self._ch_data['admin_password'] = self._format_user_secrets(admin_password)
        elif isinstance(admin_password, dict):
            self._ch_data['admin_password'] = admin_password
        elif admin_password is not None:
            raise DbaasError(f'Unexpected type of admin password {type(admin_password)}')

    @property
    def user_management_v2(self) -> bool:
        return self._ch_data.get('user_management_v2', False)

    def set_user_management_v2(self, value: bool):
        """
        Set user management mode.
        """
        self._ch_data['user_management_v2'] = value

    def _add_zk_user(self, name, password_len: int = None):
        """
        Add user and generate random password for user if none.
        """
        if name not in self._zk_users:
            self._zk_users[name] = {}
        if 'password' not in self._zk_users[name]:
            self._zk_users[name]['password'] = gen_encrypted_password(password_len)

    def add_zk_users(self):
        """
        Add users for ZooKeeper ACL.
        """
        self._add_zk_user(ZK_ACL_USER_CLICKHOUSE, 32)

    def _add_system_user(self, name, password_len: int = None):
        """
        Add user and generate random password for user if none.
        """
        if name not in self._system_users:
            self._system_users[name] = {}
        if 'password' not in self._system_users[name]:
            password, hash = gen_encrypted_password_with_hash(password_len)
            self._system_users[name]['password'] = password
            self._system_users[name]['hash'] = hash

    def add_system_users(self):
        """
        Add users for ZooKeeper ACL.
        """
        self._add_system_user(MDB_BACKUP_ADMIN_USER)

    def set_testing_repos(self, value: bool) -> None:
        """
        Enable/disable repo with testing versions
        """
        self._pillar['data']['testing_repos'] = value

    @property
    def embedded_keeper(self) -> bool:
        return self._ch_data.get('embedded_keeper', False)

    def update_embedded_keeper(self, embedded_keeper: bool):
        self._ch_data['embedded_keeper'] = embedded_keeper


class ZookeeperPillar(BackupAndAccessPillar):
    """
    Zookeeper pillar.
    """

    # pylint: disable=too-many-public-methods

    def __init__(self) -> None:
        super().__init__(
            {
                'data': {
                    'zk': {
                        'nodes': {},
                        'users': {},
                    },
                },
            }
        )

    @classmethod
    def load(cls, pillar_data: Pillar) -> "ZookeeperPillar":
        """
        Construct pillar object from pillar data.
        """
        # pylint: disable=protected-access
        pillar = ZookeeperPillar()
        pillar._pillar = pillar_data
        return pillar

    @classmethod
    def make(cls) -> "ZookeeperPillar":
        """
        Create new pillar.
        """
        # pylint: disable=protected-access
        return ZookeeperPillar()

    @property
    def _zk_data(self) -> dict:
        return self._pillar['data']['zk']

    @property
    def _nodes(self) -> dict:
        return self._zk_data['nodes']

    @property
    def _users(self) -> dict:
        return self._zk_data['users']

    def _get_vacant_zid(self):
        zids_sorted = sorted(self._nodes.values())
        zid_prev = 0
        for zid in zids_sorted:
            if zid - zid_prev > 1:
                break
            zid_prev = zid
        return zid_prev + 1

    def add_node(self, fqdn: str) -> int:
        """
        Add node.
        """
        if fqdn in self._nodes:
            raise DbaasError('Node {} is already in Zookeeper'.format(fqdn))

        self._nodes[fqdn] = self._get_vacant_zid()
        return self._nodes[fqdn]

    def add_nodes(self, fqdns: Sequence[str]) -> None:
        """
        Add nodes.
        """
        for fqdn in fqdns:
            self.add_node(fqdn)

    def delete_node(self, fqdn):
        """
        Delete node.
        """
        if fqdn not in self._nodes:
            raise DbaasClientError('Unable to find node \'{fqdn}\''.format(fqdn=fqdn))

        zid = self._nodes[fqdn]
        del self._nodes[fqdn]
        return zid

    def _add_user(self, name, password_len: int = None):
        """
        Add user and generate random password for user if none.
        """
        if name not in self._users:
            self._users[name] = {}
        if 'password' not in self._users[name]:
            self._users[name]['password'] = gen_encrypted_password(password_len)

    def add_users(self):
        """
        Add users for ZooKeeper ACL.
        """
        self._add_user(ZK_ACL_USER_SUPER)
        self._add_user(ZK_ACL_USER_BACKUP, 32)

    def set_version(self, version):
        """
        Set ZooKeeper version in pillar or clear for default version
        """
        self._pillar['data']['zk']['version'] = version


def get_zookeeper_subcid_and_pillar(cluster_id) -> Tuple[str, ZookeeperPillar]:
    """
    Return ZooKeeper subcid and pillar.
    """
    subcluster = get_subcluster(role=ClickhouseRoles.zookeeper, cluster_id=cluster_id)
    return subcluster['subcid'], ZookeeperPillar.load(subcluster['value'])


def get_subcid_and_pillar(cluster_id: str) -> Tuple[str, ClickhousePillar]:
    """
    Return ClickHouse subcid and pillar
    """
    subcluster = get_subcluster(role=ClickhouseRoles.clickhouse, cluster_id=cluster_id)
    return subcluster['subcid'], ClickhousePillar.load(subcluster['value'])


def get_pillar(cluster_id: str) -> ClickhousePillar:
    """
    Return ClickHouse pillar by cluster
    """
    return get_subcid_and_pillar(cluster_id)[1]


class ClickhouseShardPillar(BasePillar):
    """
    ClickHouse shard pillar.
    """

    @staticmethod
    def from_config(config: Dict) -> 'ClickhouseShardPillar':
        """
        Create shard pillar from config settings.
        """
        return ClickhouseShardPillar(
            {
                'data': {
                    'clickhouse': {
                        'config': config,
                    },
                },
            }
        )

    @property
    def config(self) -> Dict:
        """
        Return ClickHouse config.
        """
        return deepcopy(self._pillar['data']['clickhouse']['config'])

    def update_config(self, config: Dict) -> None:
        """
        Update ClickHouse config.
        """
        dictionaries = config.get('dictionaries', [])
        self._check_names_uniqueness(dictionaries, DuplicatedDictionaryNamesError)
        for dictionary in dictionaries:
            self._format_dictionary(dictionary)

        self._check_names_uniqueness(config.get('graphite_rollup'), DuplicatedGraphiteRollupNamesError)
        self._check_names_uniqueness(config.get('kafka_topics'), DuplicatedKafkaTopicNamesError)

        kafka = config.get('kafka')
        if kafka:
            _set_encrypted_field(
                kafka,
                'sasl_password',
                default=self._pillar['data']['clickhouse']['config'].get('kafka', {}).get('sasl_password'),
            )

        src_topics = self._pillar['data']['clickhouse']['config'].get('kafka_topics', [])
        for kafka_topic in config.get('kafka_topics', []):
            previous_password = _find_previous_value(
                src_topics,
                key=kafka_topic['name'],
                by='name',
                getter=lambda topic: topic.get('settings', {}).get('sasl_password'),
            )
            _set_encrypted_field(kafka_topic['settings'], 'sasl_password', default=previous_password)

        rabbitmq = config.get('rabbitmq')
        if rabbitmq:
            _set_encrypted_field(
                rabbitmq,
                'password',
                default=self._pillar['data']['clickhouse']['config'].get('rabbitmq', {}).get('password'),
            )

        merge_dict(self._pillar['data']['clickhouse']['config'], config)
        _validate_clickhouse_config(self._pillar['data']['clickhouse']['config'])

    @staticmethod
    def _check_names_uniqueness(items: Optional[Sequence[dict]], error: Type[Exception]) -> None:
        names = set()  # type: Set[str]
        for item in items or []:
            name = item['name']
            if name in names:
                raise error(name)

            names.add(name)

    def add_dictionary(self, dictionary: Dict) -> None:
        """
        Add external dictionary to ClickHouse config.
        """
        config = self._pillar['data']['clickhouse']['config']
        dictionaries = config.get('dictionaries', [])

        self._format_dictionary(dictionary)
        dictionaries.append(dictionary)
        self._check_names_uniqueness(dictionaries, DictionaryAlreadyExistsError)

        config['dictionaries'] = dictionaries

    def _format_dictionary(self, dictionary: dict) -> None:
        self._encrypt_dictionary_source(dictionary, 'mysql_source', need_replicas=True)
        self._encrypt_dictionary_source(dictionary, 'clickhouse_source')
        self._encrypt_dictionary_source(dictionary, 'mongodb_source')
        self._encrypt_dictionary_source(dictionary, 'yt_source', field_to_encrypt='token')
        self._encrypt_dictionary_source(dictionary, 'postgresql_source')

    def _encrypt_dictionary_source(
        self, dictionary, source_name: str, field_to_encrypt: str = 'password', need_replicas: bool = False
    ):
        source = dictionary.get(source_name)

        if not source:
            return

        _encrypt_field(source, field_to_encrypt)
        _set_encrypted_field(
            source,
            field_to_encrypt,
            default=self._pillar['data']['clickhouse']['config'].get(source_name, {}).get(field_to_encrypt),
        )

        if not need_replicas:
            return

        src_replicas = self._pillar['data']['clickhouse']['config'].get(source_name, {}).get('replicas', [])
        for replica in source.get('replicas', []):
            previous_password = _find_previous_value(
                src_replicas, key=replica['host'], by='host', getter=lambda r: r.get(field_to_encrypt)
            )
            _set_encrypted_field(replica, field_to_encrypt, default=previous_password)

    def delete_dictionary(self, name: str) -> None:
        """
        Remove external dictionary from ClickHouse config.
        """
        dictionaries = self._pillar['data']['clickhouse']['config'].get('dictionaries', [])
        for idx, dictionary in enumerate(dictionaries):
            if dictionary['name'] == name:
                del dictionaries[idx]
                return


def _encrypt_field(obj: dict, field: str) -> None:
    value = obj.get(field)
    if value and isinstance(value, str):
        obj[field] = encrypt(value)


def _validate_clickhouse_config(config: dict) -> None:
    number_of_free_entries_in_pool_to_lower_max_size_of_merge = config.get('merge_tree', {}).get(
        'number_of_free_entries_in_pool_to_lower_max_size_of_merge', 8
    )

    if config.get('background_pool_size', 16) < number_of_free_entries_in_pool_to_lower_max_size_of_merge:
        raise DbaasClientError(
            f'Value of merge_tree.number_of_free_entries_in_pool_to_lower_max_size_of_merge '
            f'({number_of_free_entries_in_pool_to_lower_max_size_of_merge}) is greater than '
            f'background_pool_size ({config.get("background_pool_size", 16)})'
        )


def _set_default_if_empty(obj: dict, field: str, default: Union[str, None]):
    value = obj.get(field, '')
    if value == '' or value is None:
        value = default

    if value is None:
        return

    obj[field] = value


def _set_encrypted_field(obj: dict, field: str, default: Union[str, None]):
    _set_default_if_empty(obj, field, default)
    _encrypt_field(obj, field)


def _find_elem_in(collection, value, by):
    findings = list(filter(lambda elem: elem[by] == value, collection))
    if len(findings) > 0:
        return findings[0]

    return None


def _find_previous_value(collection, key, by, getter):
    cred = None

    elem = _find_elem_in(collection, value=key, by=by)
    if elem:
        cred = getter(elem)

    return cred
