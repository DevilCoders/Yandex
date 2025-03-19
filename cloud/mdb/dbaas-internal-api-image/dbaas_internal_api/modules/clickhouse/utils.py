# -*- coding: utf-8 -*-
"""
ClickHouse utils.
"""
from collections import Counter
from datetime import timedelta
from typing import Dict, List, Optional, Sequence, Set, Tuple

from flask_restful import abort

from .constants import MY_CLUSTER_TYPE
from .pillar import ClickhousePillar, get_pillar
from .traits import ClickhouseOperations, ClickhouseRoles, ClickhouseTasks
from ...core.exceptions import (
    ClickhouseKeeperVersionNotSupportedError,
    CloudStorageVersionNotSupportedError,
    DbaasClientError,
)
from ...core.types import Operation
from ...utils import metadb, operation_creator
from ...utils.config import minimal_zk_resources
from ...utils.feature_flags import has_feature_flag
from ...utils.metadata import Metadata
from ...utils.types import ConfigSpec, Host, parse_resources, RequestedHostResources, RestoreResourcesHint
from ...utils.validation import get_available_geo
from ...utils.version import Version
from ...utils.worker import format_zk_hosts


def process_user_spec(user_spec: Dict, db_names: Sequence[str]) -> Dict:
    """
    Validate user_spec and expand default values.
    """
    validate_user_name(user_spec['name'])

    if user_spec.get('permissions') is None:
        user_spec['permissions'] = [
            {
                'database_name': db_name,
            }
            for db_name in db_names
        ]

    return user_spec


def validate_user_name(name: str) -> None:
    """
    Check user name.
    """
    if name in ['default', 'admin'] or name.startswith('mdb_'):
        abort(422, message='User name \'{0}\' is not allowed'.format(name))


def validate_db_name(name: str) -> None:
    """
    Check database name.
    """
    if name in ['default', 'system', 'mdb_system']:
        abort(422, message='Database name \'{0}\' is not allowed'.format(name))


def validate_user_quotas(quotas):
    if quotas is not None:
        intervals = [quota['interval_duration'] for quota in quotas]
        if len(intervals) != len(set(intervals)):
            abort(422, message='Multiple occurrence of the same IntervalDuration is not allowed')


def split_host_specs(host_specs: List[dict]) -> Tuple[List[dict], List[dict]]:
    """
    Split host specs by role, return pair of ClickHouse hosts and ZooKeeper hosts.
    """
    ch_host_specs = []
    zk_host_specs = []

    for host in host_specs:
        if host['type'] == ClickhouseRoles.clickhouse:
            ch_host_specs.append(host)
        else:
            zk_host_specs.append(host)

    return ch_host_specs, zk_host_specs


def get_zk_zones(zk_host_specs: List[dict]) -> Set[str]:
    """
    Return `zone_id`s of ZooKeeper hosts.
    """
    if zk_host_specs:
        return set(h['zone_id'] for h in zk_host_specs)
    return get_available_geo(force_filter_decommissioning=True)


def validate_zk_flavor(ch_cores_total: int, zk_flavor: dict, operation: Optional[ClickhouseOperations] = None):
    """
    Check ZooKeeper flavor for the ClickHouse cluster.
    """
    ops = {
        ClickhouseOperations.host_create: 'create additional ClickHouse hosts',
        ClickhouseOperations.shard_create: 'create an additional ClickHouse shard',
        ClickhouseOperations.shard_modify: 'update the shard\'s resource preset',
    }
    zk_cores = zk_flavor['cpu_limit']
    for resources in reversed(minimal_zk_resources()):
        ch_subcluster_cpu = resources['ch_subcluster_cpu']
        min_zk_cores = resources['zk_host_cpu']
        if ch_cores_total >= ch_subcluster_cpu and zk_cores < min_zk_cores:
            # pylint: disable=no-member, unused-variable
            recommended_flavor = metadb.get_resource_preset_by_cpu(  # noqa
                cluster_type=MY_CLUSTER_TYPE,
                role=ClickhouseRoles.zookeeper.value,
                flavor_type='standard',
                generation=zk_flavor['generation'],
                min_cpu=min_zk_cores,
            )['preset_id']
            msg = (
                f'The resource preset of ZooKeeper hosts must have at least {min_zk_cores} CPU cores '
                f'({recommended_flavor} or higher) for the requested cluster configuration.'
            )
            custom_msg = f'ZooKeeper hosts must be upscaled in order to {ops[operation]}. ' if operation in ops else ''
            raise DbaasClientError(custom_msg + msg)


def ch_cores_sum(ch_hosts: Sequence[dict], ignored_shard: Optional[str] = None) -> int:
    """
    Count the total number of cores on ClickHouse hosts.
    If `ignored_shard` is specified, ignores hosts of the shard.
    """
    flavors: dict = Counter()
    total = 0
    if ignored_shard:
        ch_hosts = [h for h in ch_hosts if h['shard_id'] != ignored_shard]
    for host in ch_hosts:
        flavors[host['flavor']] += 1
    for flavor_id, count in flavors.items():
        flavor = metadb.get_flavor_by_id(flavor_id)
        total += count * flavor['cpu_limit']
    return total


def get_hosts(cid: str) -> Tuple[List[Host], List[Host]]:
    """
    Get hosts classified on ClickHouse and ZooKeeper ones.
    """
    ch_hosts = []
    zk_hosts = []
    for host in metadb.get_hosts(cid):
        if ClickhouseRoles.clickhouse in host['roles']:
            ch_hosts.append(host)
        else:
            zk_hosts.append(host)

    return ch_hosts, zk_hosts


def get_vtype(cid: str) -> str:
    ch_hosts, _zk_hosts = get_hosts(cid)
    flavor = metadb.get_flavor_by_id(ch_hosts[0]['flavor'])
    return flavor['vtype']


def has_zookeeper(host_specs: List[dict]) -> bool:
    for host in host_specs:
        if host['type'] == ClickhouseRoles.zookeeper:
            return True

    return False


def get_zk_hosts_task_arg(*, hosts: List[Host] = None, cluster: dict = None, pillar: ClickhousePillar = None) -> str:
    """
    Return ZooKeeper hosts in the format suitable for worker task args.

    Firstly the function attempts to use cluster hosts, and then fail back to
    pillar data if the cluster has no dedicated ZooKeeper hosts. The failback
    is required in order to support legacy ClickHouse clusters with shared
    ZooKeeper.

    Either cluster, both hosts and pillar, or both hosts and cluster arguments
    must be specified.
    """
    if hosts:
        zk_hosts = [h for h in hosts if ClickhouseRoles.zookeeper in h['roles']]
    else:
        assert cluster
        zk_hosts = metadb.get_hosts(cluster['cid'], role=ClickhouseRoles.zookeeper)

    hostnames = [h['fqdn'] for h in zk_hosts]  # type: Sequence[str]

    if not hostnames:
        if not pillar:
            assert cluster
            pillar = get_pillar(cluster['cid'])

        hostnames = list(pillar.keeper_hosts.keys())
        if not hostnames:
            hostnames = pillar.zk_hosts

    return format_zk_hosts(hostnames)


class ClickhouseConfigSpec(ConfigSpec):
    """
    ClickHouse config spec.
    """

    cluster_type = MY_CLUSTER_TYPE

    def __init__(self, config_spec: dict) -> None:
        if not has_feature_flag('MDB_CLICKHOUSE_TESTING_VERSIONS'):
            strict_version_parsing = True
        else:
            strict_version_parsing = False

        super().__init__(config_spec, version_required=False, strict_version_parsing=strict_version_parsing)

    @property
    def config(self) -> dict:
        """
        ClickHouse config.
        """
        return self._ch.get('config', {})

    @property
    def resources(self) -> Tuple[RequestedHostResources, RequestedHostResources]:
        """
        Configuration of resources for ClickHouse and Zookeeper hosts.
        """
        return _get_resources(self._ch), _get_resources(self._zk)

    @property
    def backup_start(self) -> dict:
        """
        Clickhouse backup window start
        """
        return self._data.get('backup_window_start', {})

    @property
    def access(self) -> dict:
        """
        Clickhouse access options
        """
        return self._data.get('access', {})

    def __bool__(self) -> bool:
        return any((self.config, *self.resources, self.backup_start))

    @property
    def _ch(self) -> dict:
        return self._data.get('clickhouse', {})

    @property
    def _zk(self) -> dict:
        return self._data.get('zookeeper', {})

    @property
    def cloud_storage(self) -> dict:
        return self._data.get('cloud_storage', {})

    @property
    def cloud_storage_enabled(self) -> bool:
        return self._data.get('cloud_storage', {}).get('enabled', False)

    @property
    def cloud_storage_data_cache_enabled(self) -> bool:
        return self._data.get('cloud_storage', {}).get('data_cache_enabled', None)

    @property
    def cloud_storage_data_cache_max_size(self) -> int:
        return self._data.get('cloud_storage', {}).get('data_cache_max_size', None)

    @property
    def cloud_storage_move_factor(self) -> bool:
        return self._data.get('cloud_storage', {}).get('move_factor', None)

    @property
    def sql_user_management(self) -> bool:
        return self._data.get('sql_user_management')  # type: ignore

    @property
    def sql_database_management(self) -> bool:
        return self._data.get('sql_database_management')  # type: ignore

    @property
    def admin_password(self) -> str:
        return self._data.get('admin_password')  # type: ignore

    @property
    def mysql_protocol(self) -> bool:
        return self._data.get('mysql_protocol')  # type: ignore

    @property
    def postgresql_protocol(self) -> bool:
        return self._data.get('postgresql_protocol')  # type: ignore

    @property
    def embedded_keeper(self) -> bool:
        return self._data.get('embedded_keeper')  # type: ignore


class ClickhouseShardConfigSpec(ConfigSpec):
    """
    ClickHouse shard config spec.
    """

    cluster_type = MY_CLUSTER_TYPE

    def __init__(self, config_spec: dict) -> None:
        super().__init__(config_spec, version_required=False, config_spec_required=False)

    @property
    def config(self) -> dict:
        """
        ClickHouse shard config.
        """
        return self._ch.get('config', {})

    @property
    def resources(self) -> RequestedHostResources:
        """
        Configuration of resources for shard hosts.
        """
        return _get_resources(self._ch)

    @property
    def weight(self) -> Optional[int]:
        """
        ClickHouse shard weight.
        """
        return self._ch.get('weight')

    @property
    def backup_start(self) -> dict:
        """
        Clickhouse backup window start
        """
        return self._data.get('backup_window_start', {})

    def __bool__(self) -> bool:
        return any((self.config, self.resources, self.backup_start))

    @property
    def _ch(self) -> dict:
        return self._data.get('clickhouse', {})


def _get_resources(data: dict) -> RequestedHostResources:
    return parse_resources(data.get('resources', {}))


def create_operation(
    cid: str,
    task_type: ClickhouseTasks,
    operation_type: ClickhouseOperations,
    metadata: Metadata,
    time_limit: timedelta = None,
    task_args: dict = None,
) -> Operation:
    """
    Add ClickHouse task.
    """
    return operation_creator.create_operation(
        cid=cid,
        task_type=task_type,
        operation_type=operation_type,
        metadata=metadata,
        task_args=task_args,
        time_limit=_calculate_time_limit(cid, time_limit, task_args),
    )


def _calculate_time_limit(cid, time_limit: Optional[timedelta], task_args: Optional[dict]):
    """
    Calculate result time limit for a task.

    If restart flag is set, the time limit is adjusted based on cluster size as cluster hosts are updated
    sequentially.
    """
    extend = 2 if _extend_timeout(cid) else 1
    default_time_limit = timedelta(hours=1)
    if not time_limit:
        time_limit = default_time_limit

    restart = task_args.get('restart') if task_args else None
    if not restart:
        return time_limit * extend

    host_count = len(metadb.get_hosts(cid))
    return (time_limit + max(timedelta(), host_count * timedelta(minutes=5) - default_time_limit)) * extend


def _extend_timeout(cid):
    ch_hosts, _zk_hosts = get_hosts(cid)
    flavors = {host['flavor'] for host in ch_hosts}
    return any(map(lambda flavor: metadb.get_flavor_by_id(flavor)['cpu_guarantee'] < 1, flavors))


class ClickHouseRestoreResourcesHint(RestoreResourcesHint):
    """
    ClickHouse restore resources hint
    """

    def __init__(self, resource_preset_id: str, disk_size: int, min_hosts_count: int) -> None:
        super().__init__(
            resource_preset_id=resource_preset_id,
            disk_size=disk_size,
            role=ClickhouseRoles.clickhouse,
        )
        self.min_hosts_count = min_hosts_count


def ensure_version_supported_for_cloud_storage(version):
    """
    Check that Cloud Storage feature is supported for the specified version, and throw exception if it's not.
    """
    min_supported_version = Version(22, 3)
    if version < min_supported_version:
        raise CloudStorageVersionNotSupportedError(min_supported_version)


def ensure_version_supported_for_clickhouse_keeper(version):
    """
    Check that ClickHouse Keeper feature is supported for the specified version, and throw exception if it's not.
    """
    min_supported_version = Version(22, 3)
    if version < min_supported_version:
        raise ClickhouseKeeperVersionNotSupportedError(min_supported_version)
