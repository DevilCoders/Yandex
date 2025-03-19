"""
Utility functions for Redis.
"""
from collections import defaultdict
from typing import Dict, List, Optional

from ...core.exceptions import PreconditionFailedError, DbaasClientError
from ...utils import metadb, validation
from ...utils.register import get_cluster_traits
from ...utils.types import DTYPE_LOCAL_SSD, Host, VTYPE_COMPUTE
from ...utils.version import Version
from .constants import (
    DEFAULT_SHARD_NAME,
    MAX_SHARDS_COUNT,
    MIN_HOSTS_IN_SHARD_FOR_HA,
    MIN_SHARDS_COUNT,
    MY_CLUSTER_TYPE,
    UPGRADE_PATHS,
    BYTES_CLIENT_OUTPUT_BUFFER_LIMIT_MAX,
    KILOBYTES_CLIENT_OUTPUT_BUFFER_LIMIT_MAX,
    MEGABYTES_CLIENT_OUTPUT_BUFFER_LIMIT_MAX,
    GIGABYTES_CLIENT_OUTPUT_BUFFER_LIMIT_MAX,
    BUFFER_LIMITS_KEY_TO_ORDER,
)
from .schemas.configs import PersistenceMode
from .traits import RedisRoles, MemoryUnits
from .types import RedisConfigSpec


def validate_disk_size(flavor: dict, disk_size: int):
    """
    Check that disk_size > 2 x RAM.
    """
    min_disk_size = flavor['memory_limit'] * 2
    if disk_size < min_disk_size:
        raise PreconditionFailedError(
            'Disk size must be at least two times larger than memory size '
            f'({min_disk_size} for the current resource preset)'
        )


def group_by_shard(hosts: List[Host]) -> Dict[str, List[Host]]:
    """
    Group the list of hosts by shard name.
    """
    shards: dict = defaultdict(list)
    for host in hosts:
        shard_name = host.get('shard_name', DEFAULT_SHARD_NAME)
        shards[shard_name].append(host)
    return shards


def validate_shards_number(shards_num: int, pillar=None) -> None:
    """
    Assert that the number of shards is valid.
    """
    max_shards_count = MAX_SHARDS_COUNT if pillar is None else pillar.max_shards_count
    if not MIN_SHARDS_COUNT <= shards_num <= max_shards_count:
        raise PreconditionFailedError(f'Number of shards must be between {MIN_SHARDS_COUNT} and {max_shards_count}.')


def is_localssd_in_compute(flavor: Optional[dict], disk_type_id: Optional[str]) -> bool:
    return flavor and disk_type_id == DTYPE_LOCAL_SSD and flavor['vtype'] == VTYPE_COMPUTE  # type: ignore


def validate_shards_ha(shards: Dict[str, List[Host]]):
    for shard_name, shard in shards.items():
        if len(shard) < MIN_HOSTS_IN_SHARD_FOR_HA:
            raise PreconditionFailedError(f'Shard {shard_name} should contain at least 1 replica.')


def calculate_maxmemory(flavor: dict) -> int:
    """
    Returns maxmemory value depending on the flavor.
    """
    return int(0.75 * flavor['memory_limit'])


def calculate_repl_backlog_size(flavor: dict) -> int:
    """
    Returns repl-backlog-size value depending on the flavor.
    """
    return int(0.1 * flavor['memory_limit'])


def validate_client_output_buffer_limit(spec: RedisConfigSpec):
    def check_limit(unit, value):
        if value is None:
            return
        exceeds = (
            unit == MemoryUnits.bytes.value
            and value > BYTES_CLIENT_OUTPUT_BUFFER_LIMIT_MAX
            or unit == MemoryUnits.kilobytes.value
            and value > KILOBYTES_CLIENT_OUTPUT_BUFFER_LIMIT_MAX
            or unit == MemoryUnits.megabytes.value
            and value > MEGABYTES_CLIENT_OUTPUT_BUFFER_LIMIT_MAX
            or unit == MemoryUnits.gigabytes.value
            and value > GIGABYTES_CLIENT_OUTPUT_BUFFER_LIMIT_MAX
        )
        if exceeds:
            raise PreconditionFailedError(
                f"Client output buffer limit can't exceed " f"{GIGABYTES_CLIENT_OUTPUT_BUFFER_LIMIT_MAX}Gb"
            )

    def check_limits(limit):
        """
        {'hard_limit_unit': 'b', 'hard_limit': 100000000000,
         'soft_limit': 3388608, 'soft_seconds': 30, 'soft_limit_unit': 'b'}
        """
        if not limit:
            return
        check_limit(limit.get('hard_limit_unit'), limit.get('hard_limit'))
        check_limit(limit.get('soft_limit_unit'), limit.get('soft_limit'))

    if not spec:
        return
    check_limits(spec.client_output_limit_buffer_normal)
    check_limits(spec.client_output_limit_buffer_pubsub)


def calculate_client_output_buffer_limit(flavor: dict) -> int:
    """
    Returns client-output-buffer-limit value depending on the flavor.
    """
    return int(0.5 * flavor['network_limit'])


def get_buffer_limit(value, unit):
    if value is None or unit is None:
        return

    multiplier = BUFFER_LIMITS_KEY_TO_ORDER.get(unit, 1)
    return int(value) * multiplier


def split_buffer_limits(limits: str):
    hard_limit, soft_limit, soft_seconds = limits.split(" ")
    return hard_limit, soft_limit, soft_seconds


def combine_client_output_buffer_limit(limits: str, conf: dict) -> str:
    hard_limit, soft_limit, soft_seconds = split_buffer_limits(limits)

    hard_unit_new = conf.get('hard_limit_unit', None)
    hard_value_new = conf.get('hard_limit', None)
    hard_limit_new = get_buffer_limit(hard_value_new, hard_unit_new)

    soft_unit_new = conf.get('soft_limit_unit', None)
    soft_value_new = conf.get('soft_limit', None)
    soft_limit_new = get_buffer_limit(soft_value_new, soft_unit_new)

    soft_seconds_new = conf.get('soft_seconds', None)

    hard_limit = hard_limit if hard_limit_new is None else hard_limit_new
    soft_limit = soft_limit if soft_limit_new is None else soft_limit_new
    soft_seconds = soft_seconds if soft_seconds_new is None else soft_seconds_new

    return "{} {} {}".format(hard_limit, soft_limit, soft_seconds)


def get_cluster_nodes_validate_func(flavor, disk_type_id, sharded):
    localssd_compute = is_localssd_in_compute(flavor, disk_type_id)
    return validation.validate_hosts_count_only_max if sharded and localssd_compute else None


def validate_cluster_nodes(flavor, disk_type_id, resource_preset_id, sharded, shards, pillar=None):
    validate_func = get_cluster_nodes_validate_func(flavor, disk_type_id, sharded)
    if sharded:
        validate_shards_number(len(shards), pillar)
        localssd_compute = is_localssd_in_compute(flavor, disk_type_id)
        if localssd_compute:
            validate_shards_ha(shards)

    for shard, hosts in shards.items():
        validation.validate_hosts_count(
            cluster_type=MY_CLUSTER_TYPE,
            role=RedisRoles.redis.value,  # pylint: disable=no-member
            resource_preset_id=resource_preset_id,
            disk_type_id=disk_type_id,
            hosts_count=len(hosts),
            validate_func=validate_func,
        )


def check_if_tls_available(tls_enabled: bool, version: Version):
    if tls_enabled and version < Version(6, 0):
        raise PreconditionFailedError('TLS could not be turned on for Redis version {}'.format(version))


def fill_tls_supported(res: dict):
    available_versions = res['available_versions']
    tls_supported = []
    for version in available_versions:
        try:
            check_if_tls_available(True, version['id'])
            tls_supported.append(version['name'])
        except PreconditionFailedError:
            pass
    res['tls_supported_versions'] = tls_supported


def fill_persistence_modes(res: dict):
    res['persistence_modes'] = list(PersistenceMode().mapping.keys())


def validate_password_change(conf_obj: RedisConfigSpec, old_pass: Optional[str]) -> bool:
    """
    Process password change
    """
    config = conf_obj.get_config()
    if 'password' in config:
        if len(config) > 1:
            raise DbaasClientError('Password change cannot be mixed with other modifications')
        if config['password'] != old_pass:
            return True
    return False


def validate_version_change(cur_version: Version, conf_obj: RedisConfigSpec) -> None:
    """
    Validate version change
    """
    new_version = conf_obj.version
    if new_version is None:
        raise RuntimeError('Version is not specified in spec')

    if not conf_obj.has_version_only():
        raise DbaasClientError('Version change cannot be mixed with other modifications')

    if cur_version > new_version:
        raise PreconditionFailedError('Downgrade Redis version is not possible')

    if new_version.major > (cur_version.major + 1):
        raise PreconditionFailedError('Cant upgrade Redis to more than one major version at a time')

    version_path = UPGRADE_PATHS.get(new_version.to_string())
    if not version_path:
        raise PreconditionFailedError('Invalid version to upgrade to: {new}'.format(new=new_version))

    if cur_version.to_string() not in version_path['from']:
        raise PreconditionFailedError(
            'Upgrade from {old} to {new} is not allowed'.format(old=cur_version, new=new_version)
        )


def get_cluster_version(cid):
    """
    Lookup dbaas.versions entry
    :param cid: cluster id of desired cluster
    :return: actual version of cluster
    """

    # (maj, min, edition) or None
    traits = get_cluster_traits(MY_CLUSTER_TYPE)
    cluster = metadb.get_clusters_versions(traits.versions_component, [cid]).get(cid)
    if not cluster:
        raise RuntimeError(f'Version is not specified for {cid}')
    return Version.load(cluster['major_version'])


def validate_public_ip(host_specs: List[Host], tls_enabled: bool) -> None:
    """
    Validate security group rules for dataproc cluster
    """
    is_public_assigned = any(host.get('assign_public_ip', False) for host in host_specs)
    if is_public_assigned and not tls_enabled:
        raise DbaasClientError('Public ip for host requires TLS enabled.')
