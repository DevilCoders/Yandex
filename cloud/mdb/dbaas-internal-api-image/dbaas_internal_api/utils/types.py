"""
Types shared between api and modules
"""
import attr
from datetime import datetime, timedelta
from types import SimpleNamespace
from typing import Any, Dict, List, NamedTuple, Optional, Tuple, Type, TypedDict, TypeVar, Union
from uuid import UUID

import copy
from enum import Enum, unique
from psycopg2.extensions import ISQLQuote, adapt

from .maintenance_helpers import calculate_nearest_maintenance_window
from .version import Version, get_valid_versions
from ..core.exceptions import ConfigTargetVersionMismatchError, NoVersionError, ParseConfigError, VersionNotExistsError

TERABYTE = 1024**4
GIGABYTE = 1024**3
MEGABYTE = 1024**2
KILOBYTE = 1024
ENV_PROD = 'prod'
VTYPE_COMPUTE = 'compute'
VTYPE_PORTO = 'porto'
DTYPE_LOCAL_SSD = 'local-ssd'
DTYPE_NETWORK_SSD = 'network-ssd'
DTYPE_NETWORK_HDD = 'network-hdd'
DTYPE_NRD = 'network-ssd-nonreplicated'

RestorePoint = NamedTuple('RestorePoint', [('backup_id', str)])

Pillar = Dict[Any, Any]
Host = Dict[str, Any]
EncryptedField = Dict[str, Union[str, int]]
LabelsDict = Dict[str, str]

MetadbHostType = TypedDict(
    'MetadbHostType',
    {
        'subcid': str,
        'shard_id': Union[str, None],
        'shard_name': str,
        'space_limit': int,  # bigint
        'flavor': str,
        'geo': str,
        'fqdn': str,
        'assign_public_ip': bool,
        'flavor_name': str,
        'vtype': Union[str, None],
        'vtype_id': str,
        'disk_type_id': str,
        'subnet_id': str,
        'roles': List[str],
    },
)


class Alert(SimpleNamespace):
    """
    Alert
    """

    # pylint: disable=redefined-builtin
    def __init__(
        self, template_id, notification_channels=None, disabled=False, warning_threshold=None, critical_threshold=None
    ):
        super().__init__(
            template_id=template_id,
            warning_threshold=warning_threshold,
            critical_threshold=critical_threshold,
            notification_channels=notification_channels,
            disabled=disabled,
        )

    def __eq__(self, other):
        return self.template_id == other.template_id

    def __hash__(self):
        return hash(
            (
                self.template_id,
                self.warning_threshold,
                self.critical_threshold,
                self.notification_channels,
                self.disabled,
            )
        )


class AlertTemplate(SimpleNamespace):
    """
    Alert Template
    """

    # pylint: disable=redefined-builtin
    def __init__(
        self,
        template_id,
        notification_channels=None,
        disabled=False,
        warning_threshold=None,
        critical_threshold=None,
        description=None,
        name=None,
        mandatory=None,
    ):
        super().__init__(
            template_id=template_id,
            warning_threshold=warning_threshold,
            critical_threshold=critical_threshold,
            notification_channels=notification_channels,
            disabled=disabled,
            description=description,
            name=name,
            mandatory=mandatory,
        )


class AlertGroup(SimpleNamespace):
    """
    Cluster alert group.
    """

    # pylint: disable=redefined-builtin
    def __init__(self, alert_group_id, monitoring_folder_id, alerts: List[Alert] = None):
        super().__init__(
            alert_group_id=alert_group_id,
            alerts=alerts,
            monitoring_folder_id=monitoring_folder_id,
        )


class Backup(SimpleNamespace):
    """
    Cluster backup.
    """

    # pylint: disable=redefined-builtin
    def __init__(self, id, start_time, end_time, managed_backup=False, name=None):
        super().__init__(id=id, start_time=start_time, end_time=end_time, managed_backup=managed_backup, name=name)


class ShardedBackup(Backup):
    """
    Multilevel cluster backup.
    """

    # pylint: disable=redefined-builtin
    def __init__(
        self,
        id,
        start_time,
        end_time,
        shard_names,
        s3_path,
        tool=None,
        managed_backup=False,
        size=None,
        btype=None,
        name=None,
    ):
        super().__init__(id, start_time, end_time, managed_backup, name)
        self.shard_names = shard_names
        self.s3_path = s3_path
        self.tool = tool
        self.size = size
        self.type = btype


class ClusterBackup(Backup):
    """
    Simple whole cluster backup (like mysql/pg).
    """

    # pylint: disable=redefined-builtin
    def __init__(
        self,
        id,
        start_time,
        end_time,
        managed_backup=False,
        size=None,
        uncompressed_size=None,
        method=None,
        btype=None,
        name=None,
    ):
        super().__init__(id, start_time, end_time, managed_backup, name)
        self.size = size
        self.uncompressed_size = uncompressed_size
        self.btype = btype
        self.method = method


ClusterDescription = NamedTuple(
    'ClusterDescription',
    [
        ('name', str),
        ('environment', str),
        ('description', Optional[str]),
        ('labels', Optional[LabelsDict]),
    ],
)


class ConfigSet(NamedTuple('ConfigSet', [('effective', dict), ('user', dict), ('default', dict)])):
    """
    Configurations of cluster.
    """

    def as_dict(self) -> dict:
        """
        Returns dict as in spec.
        """
        return {
            'default_config': self.default,
            'effective_config': self.effective,
            'user_config': self.user,
        }


@unique
class ClusterStatus(Enum):
    """
    Cluster status
    """

    unknown = 'UNKNOWN'
    creating = 'CREATING'
    running = 'RUNNING'
    create_error = 'CREATE-ERROR'
    modifying = 'MODIFYING'
    modify_error = 'MODIFY-ERROR'
    stopping = 'STOPPING'
    stop_error = 'STOP-ERROR'
    stopped = 'STOPPED'
    starting = 'STARTING'
    start_error = 'START-ERROR'
    restoring_offline = 'RESTORING-OFFLINE'
    restoring_online = 'RESTORING-ONLINE'
    restoring_offline_error = 'RESTORE-OFFLINE-ERROR'
    restoring_online_error = 'RESTORE-ONLINE-ERROR'
    maintaining_offline = 'MAINTAINING-OFFLINE'
    maintain_offline_error = 'MAINTAIN-OFFLINE-ERROR'
    deleting = 'DELETING'
    delete_error = 'DELETE-ERROR'
    deleted = 'DELETED'
    metadata_deleting = 'METADATA-DELETING'
    metadata_deleted = 'METADATA-DELETED'
    metadata_delete_error = 'METADATA-DELETE-ERROR'
    purging = 'PURGING'
    purge_error = 'PURGE-ERROR'
    purged = 'PURGED'

    # Don't put new statuses below DELETING if it doesn't mean that cluster is 'deleting'
    # Look at function below

    def is_deleting(self) -> bool:
        """
        Return true whenever that status is deleting
        """
        statuses = list(self.__class__)  # type: List[ClusterStatus]
        return statuses.index(self) >= statuses.index(self.deleting)  # type: ignore


@unique
class HostStatus(Enum):
    """
    Possible host health status
    """

    unknown = 'HostStatusUnknown'
    alive = 'HostStatusAlive'
    dead = 'HostStatusDead'
    degraded = 'HostStatusDegraded'


@unique
class ClusterHealth(Enum):
    """
    Possible cluster health.
    """

    unknown = 'ClusterHealthUnknown'
    alive = 'ClusterHealthAlive'
    dead = 'ClusterHealthDead'
    degraded = 'ClusterHealthDegraded'

    def humanize(self):
        return self.value.replace('ClusterHealth', '')


@unique
class ClusterVisibility(Enum):
    """
    Possible host health status
    """

    visible = 'visible'
    visible_or_deleted = 'visible+deleted'
    all = 'all'


class WeeklyMaintenanceWindow(NamedTuple):
    day: str
    hour: int


MaintenanceWindowDict = dict[str, dict[str, Any]]


class MaintenanceWindow:
    def __init__(self, anytime: bool, weekly_maintenance_window: Optional[WeeklyMaintenanceWindow] = None):
        if anytime:
            self.anytime: dict[Any, Any] = {}
        else:
            self.weekly_maintenance_window = weekly_maintenance_window

    @property
    def is_anytime(self) -> bool:
        return hasattr(self, 'anytime')

    @staticmethod
    def make(args_dict):
        if args_dict['mw_day'] is not None and args_dict['mw_hour'] is not None:
            return MaintenanceWindow(
                anytime=False,
                weekly_maintenance_window=WeeklyMaintenanceWindow(
                    day=args_dict['mw_day'],
                    hour=args_dict['mw_hour'],
                ),
            )

        return MaintenanceWindow(anytime=True)


class MaintenanceOperation(NamedTuple):
    info: str
    delayed_until: datetime
    create_ts: datetime
    config_id: str
    next_maintenance_window_time: datetime
    latest_maintenance_time: datetime

    @staticmethod
    def _get_nearest_maintenance_window_time(delayed_until: datetime, maintenance_window: MaintenanceWindow):
        if maintenance_window.is_anytime:
            return None
        return calculate_nearest_maintenance_window(
            delayed_until,
            maintenance_window.weekly_maintenance_window.day,  # type: ignore
            maintenance_window.weekly_maintenance_window.hour,  # type: ignore
        )

    @staticmethod
    def make(args_dict: dict, maintenance_window: MaintenanceWindow) -> Optional['MaintenanceOperation']:
        if args_dict['mw_config_id'] is not None:
            return MaintenanceOperation(
                info=args_dict['mw_info'],
                config_id=args_dict['mw_config_id'],
                create_ts=args_dict['mw_create_ts'],
                delayed_until=args_dict['mw_delayed_until'],
                next_maintenance_window_time=MaintenanceOperation._get_nearest_maintenance_window_time(
                    args_dict['mw_delayed_until'], maintenance_window
                ),
                latest_maintenance_time=args_dict['mw_max_delay'],
            )
        return None


@unique
class MaintenanceRescheduleType(Enum):
    unspecified = 'MaintenanceRescheduleTypeUnspecified'
    immediate = 'MaintenanceRescheduleTypeImmediate'
    next_available_window = 'MaintenanceRescheduleTypeNextAvailableWindow'
    specific_time = 'MaintenanceRescheduleTypeSpecificTime'


class ClusterInfo(NamedTuple):
    """
    Cluster info in terms of metadb
    """

    cid: str
    name: str
    type: str
    env: str
    created_at: datetime
    network_id: str
    value: dict
    description: Optional[str]
    status: ClusterStatus
    labels: LabelsDict
    maintenance_window: MaintenanceWindow
    planned_operation: Optional[MaintenanceOperation]
    rev: int
    backup_schedule: dict
    user_sgroup_ids: List[str]
    host_group_ids: List[str]
    major_version: Optional[str] = None
    minor_version: Optional[str] = None
    deletion_protection: bool = False
    monitoring_cloud_id: Optional[str] = None

    @property
    def pillar(self) -> Pillar:
        """
        Return pillar dict

        just alias for value
        """
        return self.value

    @staticmethod
    def make(args_dict):
        """
        Return ClusterInfo ignoring extra keys in @args_dict
        """

        status = ClusterStatus(args_dict['status'])
        maintenance_window = MaintenanceWindow.make(args_dict)
        if 'user_sgroup_ids' not in args_dict:
            args_dict['user_sgroup_ids'] = []
        return ClusterInfo(
            status=status,
            maintenance_window=maintenance_window,
            planned_operation=MaintenanceOperation.make(args_dict, maintenance_window),
            **dict((k, args_dict[k]) for k in args_dict if k != 'status' and not k.startswith('mw_'))
        )


class FolderIds(NamedTuple):
    """
    Folder ids
    """

    folder_id: int
    folder_ext_id: str


class IdempotenceData(NamedTuple):
    """
    Idempotence data
    """

    idempotence_id: UUID
    request_hash: bytes


class RequestedHostResources(NamedTuple):
    """
    Class representing host resources.

    All fields are optional
    """

    resource_preset_id: Optional[str] = None
    disk_size: Optional[int] = 0
    disk_type_id: Optional[str] = None

    def __bool__(self):
        return any(self._asdict().values())  # pylint: disable=no-member


class ExistedHostResources(SimpleNamespace):
    """
    Class representing existed host resources.

    All fields are defined, cause we can't have
    host without flavor, disk_size and disk_type_id
    """

    resource_preset_id: str
    disk_size: int
    disk_type_id: str

    def update(self, resources: RequestedHostResources):
        """
        Update fields with values from the passed in resources. Empty values (ones that evaluate to False
        in boolean context) are not propagated.
        """
        if resources.resource_preset_id:
            self.resource_preset_id = resources.resource_preset_id
        if resources.disk_size:
            self.disk_size = resources.disk_size
        if resources.disk_type_id:
            self.disk_type_id = resources.disk_type_id
        return self

    def to_requested_host_resources(self) -> RequestedHostResources:
        return RequestedHostResources(
            resource_preset_id=self.resource_preset_id, disk_size=self.disk_size, disk_type_id=self.disk_type_id
        )


ComparableEnumT = TypeVar('ComparableEnumT', bound='ComparableEnum')


class OperationParameters:
    """
    Class representing operation parameters
    """

    def __init__(self, changes: bool, task_args: dict, time_limit: timedelta):
        self.changes = changes
        self.task_args = task_args
        self.time_limit = time_limit

    def __iadd__(self, other: 'OperationParameters'):
        self.changes = self.changes or other.changes
        self.task_args.update(other.task_args)
        self.time_limit += other.time_limit
        return self


@unique
class ComparableEnum(Enum):
    """
    Enum witch allow compare with str
    """

    def __eq__(self, other):
        """
        Allow compare with string
        """
        if other.__class__ is self.__class__:
            return self is other
        if isinstance(other, str):
            # pylint: disable=comparison-with-callable
            return self.value == other
        raise TypeError('Unable to compare {0!r} with {1!r}'.format(self, other))

    @classmethod
    def from_string(cls: Type[ComparableEnumT], value: str) -> ComparableEnumT:
        """
        Make enum instance from string
        """
        # mypy don't understand enum.* internal magic ;(
        for member in cls:  # type: ignore
            if member.value == value:  # type: ignore
                return member  # type: ignore
        # raise KeyError, cause Enum['...'] do it
        raise KeyError('Unable to find {0!r} in {1!r}'.format(value, cls))

    @classmethod
    def has_value(cls, value: str) -> bool:
        """
        Check if enum has specific value
        """
        try:
            cls.from_string(value)
            return True
        except KeyError:
            return False

    def __hash__(self):
        """
        Hash impl
        We want Enum instances as dict keys.
        """

        # https://docs.python.org/3.7/library/enum.html#enum-members-aka-instances
        # It's okay, cause enum members are singletons
        return id(self)

    def __conform__(self, proto):
        """
        Tell psycopg2 that we this type
        know how it should be adapted
        """
        if proto is ISQLQuote:
            return self

    def getquoted(self):
        """
        Convert pillar to db object
        """
        return adapt(self.value).getquoted()


class RestoreResourcesHint:
    """
    Resources hint for cluster restore
    """

    def __init__(self, resource_preset_id: str, disk_size: int, role: ComparableEnum) -> None:
        self.resource_preset_id = resource_preset_id
        self.disk_size = disk_size
        self.role = role


class RestoreHint:
    """
    Restore hint
    """

    def __init__(
        self, resources: RestoreResourcesHint, version: Optional[str], env: str, network_id: Optional[str]
    ) -> None:
        self.resources = resources
        self.version = version
        self.env = env
        self.network_id = network_id


class RestoreHintWithTime(RestoreHint):
    """
    Restore hint for databases with PITR
    """

    def __init__(
        self, resources: RestoreResourcesHint, version: str, env: str, network_id: Optional[str], time: datetime
    ) -> None:
        super().__init__(resources, version, env, network_id)
        self.time = time


class ConfigSpec:
    """
    Base class for config specs.
    """

    cluster_type: str = ''
    config_key_prefix: str = ''

    def __init__(
        self, config_spec: dict, *, version_required=True, strict_version_parsing=True, config_spec_required=True
    ) -> None:
        assert self.cluster_type
        if config_spec_required and not config_spec:
            raise ValueError('config_spec cannot be empty')

        self._data = config_spec
        self._valid_versions = get_valid_versions(self.cluster_type)

        self.version, self.versioned_config_key = self._find_version(strict_version_parsing)
        if not self.version and version_required:
            raise NoVersionError()

    def _find_version(self, strict) -> Tuple[Optional[Version], Optional[str]]:
        """
        Return version and versioned config key.
        """
        explicit_version = self._get_explicit_version(strict)
        config_version, config_key = self._deduce_config_version()

        if strict:
            for version in (explicit_version, config_version):
                if version and version not in self._valid_versions:
                    self._raise_invalid_version_error(version.to_string())

        if explicit_version and config_version and config_version != explicit_version:
            assert config_key
            raise ConfigTargetVersionMismatchError(explicit_version.to_string(), config_version.to_string())

        version = explicit_version or config_version

        return version, config_key

    def _get_explicit_version(self, strict) -> Optional[Version]:
        version = self._data.get('version')
        if version:
            try:
                return Version.load(version, strict=strict)
            except ValueError:
                self._raise_invalid_version_error(version)

        return None

    def _deduce_config_version(self) -> Tuple[Optional[Version], Optional[str]]:
        if self.config_key_prefix:
            prefix = self.config_key_prefix + '_'
            for key in self._data:
                if key.startswith(prefix):
                    return Version.load(key[len(prefix) :], '_'), key

        return None, None

    def _raise_invalid_version_error(self, version: str) -> None:
        raise VersionNotExistsError(version, [v.to_string() for v in self._valid_versions])

    def has_version_only(self) -> bool:
        """Check if only version was passed"""
        return list(self._data.keys()) == ['version']


def def_disk_size_parser(resources: dict) -> int:
    """
    :param resources: dict
    :return: parsed disk size
    """
    disk_size = resources.get('disk_size')
    if disk_size is not None:
        try:
            disk_size = int(disk_size)
        except Exception:
            raise ParseConfigError("disk_size {} could not be converted to int".format(disk_size))
    return disk_size


def def_disk_type_id_parser(resources: dict) -> Optional[str]:
    """
    :param resources: dict
    :return: parsed disk type id
    """
    return resources.get('disk_type_id')


class PlainConfigSpec(ConfigSpec):
    """
    Example data for single-typed cluster
    config_spec: {
        postgresql_config_<maj>_<min>: {<config>},
        pooler_config: {<config>},
        resources: {
            resource_preset_id: <flavor_id>,
            disk_size: <volume size>,
            disk_type_id: <type id>
        },
    }
    """

    def has_resources(self) -> bool:
        """Check if resources are given"""
        return 'resources' in self._data

    def get_resources(
        self, disk_size_parser=def_disk_size_parser, disk_type_id_parser=def_disk_type_id_parser
    ) -> RequestedHostResources:
        """Returns a Resource object"""
        try:
            res = self._data['resources']
        except KeyError:
            raise ValueError('resources are not specified')
        return parse_resources(res, disk_size_parser=disk_size_parser, disk_type_id_parser=disk_type_id_parser)

    def has_config(self, key: Optional[str] = None) -> bool:
        """Check if key is present in config"""
        if key is None:
            key = self.versioned_config_key
        return key in self._data

    def get_config(self, key: Optional[str] = None) -> dict:
        """Returns config dict"""
        if key is None:
            key = self.versioned_config_key
        return copy.deepcopy(self._data.get(key, dict()))

    def set_config(self, path: List[str], value: Any):
        """Patches config by setting `value` by traversing `path`"""
        ptr = self._data
        for step in path[:-1]:
            if step not in ptr:
                ptr[step] = {}
            ptr = ptr[step]
        ptr[path[-1]] = value


class NestedConfigSpec(ConfigSpec):
    """
    In contrast with PlainConfigSpec, one must specify a service for each
    access method.

    Example data for service with multiple sub-clusters
    config_spec: {
        mongodb_spec_<major>_<minor>: {
            mongod: {
                config: {}
                resources: {
                    resource_preset_id: <flavor_id>,
                    disk_size: <volume size>,
                    disk_type_id: <type id>
                }
            },
            mongos: { ... },
            ...
        }
    }
    """

    valid_roles: list = []

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._services = self._find_roles()

    def has_services(self) -> bool:
        """Check if any service is present"""
        return bool(self._services)

    def _find_roles(self) -> list:
        """Discover services provided in config_spec"""
        role_list = self._data.get(self.versioned_config_key, [])
        return [x for x in role_list if x in self.valid_roles]

    def has_role(self, role: str) -> bool:
        """Check if role is present"""
        return role in self._services

    def _check_role_defined(self, role: str) -> None:
        if not self.has_role(role):
            raise KeyError('role "{0}" is not defined in config'.format(role))

    def has_config(self, role: str, key: str = 'config') -> bool:
        """Check if key is present in config"""
        return key in self._data[self.versioned_config_key].get(role, {})

    def get_config(self, role: str, key: str = 'config') -> dict:
        """Returns config"""
        self._check_role_defined(role)
        try:
            return copy.deepcopy(self._data[self.versioned_config_key][role][key])
        except KeyError as err:
            raise KeyError('no configuration for {0} is provided'.format(err)) from err

    def has_resources(self, role: str) -> bool:
        """Check if resources are given"""
        return 'resources' in self._data[self.versioned_config_key].get(role, {})

    def get_resources(self, role: str) -> RequestedHostResources:
        """Get resources as RequestedHostResources object"""
        # Wrong `role`: e.g. mongocfg when only mongod is supported.
        self._check_role_defined(role)
        # No resources are given for the role.
        try:
            res = self._data[self.versioned_config_key][role]['resources']
        except KeyError:
            raise ValueError('resources are not provided')
        # RequestedHostResources are there, but some are missing.
        return parse_resources(res)


class BackupInitiator(ComparableEnum):
    """
    Possible backup initiators.
    """

    unspecified = 'BackupTypeUnspecified'
    automated = 'BackupTypeAutomated'
    manual = 'BackupTypeManual'


class BackupInitiatorFromDB:
    m = {'SCHEDULE': BackupInitiator.automated, 'USER': BackupInitiator.manual}

    @classmethod
    def load(cls: Type['BackupInitiatorFromDB'], raw: str) -> BackupInitiator:
        return cls.m.get(raw, BackupInitiator.unspecified)


class BackupMethod(ComparableEnum):
    """Backup service backup method"""

    unspecified = 'BACKUP_METHOD_UNSPECIFIED'
    base = 'BASE'
    increment = 'INCREMENTAL'


class BackupMethodFromDB(ComparableEnum):
    full = 'FULL'
    incremental = 'INCREMENTAL'

    @classmethod
    def load(cls: Type['BackupMethodFromDB'], raw: str) -> BackupMethod:
        return {'FULL': BackupMethod.base, 'INCREMENTAL': BackupMethod.increment}.get(raw, BackupMethod.base)


class BackupStatus(ComparableEnum):
    """Backup service backup statuses"""

    done = 'DONE'


class ManagedBackup(NamedTuple):
    """Class representing backup-service backup"""

    id: str
    cid: str
    subcid: str
    shard_id: str
    status: BackupStatus
    method: BackupMethod
    initiator: BackupInitiator
    metadata: dict
    started_at: datetime
    finished_at: datetime


def parse_resources(
    resources: dict, disk_size_parser=def_disk_size_parser, disk_type_id_parser=def_disk_type_id_parser
) -> RequestedHostResources:
    """
    Convert host resources from dict representation to RequestedHostResources.
    """
    try:
        disk_size = disk_size_parser(resources)
        disk_type_id = disk_type_id_parser(resources)
        return RequestedHostResources(
            disk_type_id=disk_type_id,
            resource_preset_id=resources.get('resource_preset_id'),
            disk_size=disk_size,
        )
    except ValueError as err:
        raise ValueError('malformed resource: {0}'.format(err)) from err
    except ParseConfigError:
        raise


# python version of metadb.get_host_info
@attr.s(auto_attribs=True, kw_only=True)
class MetadbHostInfo(object):
    subcid: str
    shard_id: Optional[str]
    shard_name: Optional[str]
    flavor: str  # uuid
    space_limit: int
    geo: str  # e.g. 'ru-central1-a'
    fqdn: str
    name: str  # e.g. 's1.large'
    vtype: str  # e.g. 'compute'
    vtype_id: Optional[str]  # e.g. a1a2b3c4d5e...
    disk_type_id: str  # e.g. 'network-ssd'
    subnet_id: Optional[str]  # e.g. b1a2b3c4d5e...
    assign_public_ip: bool
    subcluster_name: str  # user-specified name
    roles: list[str]

    @classmethod
    def from_dict(cls, host: dict):
        return MetadbHostInfo(
            subcid=host['subcid'],
            shard_id=host['shard_id'],
            shard_name=host['shard_name'],
            flavor=host['flavor'],
            space_limit=host['space_limit'],
            geo=host['geo'],
            fqdn=host['fqdn'],
            name=host['name'],
            vtype=host['vtype'],
            vtype_id=host['vtype_id'],
            disk_type_id=host['disk_type_id'],
            subnet_id=host['subnet_id'],
            assign_public_ip=host['assign_public_ip'],
            subcluster_name=host['subcluster_name'],
            roles=host['roles'],
        )
