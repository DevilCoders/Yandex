# coding: utf-8
"""
Worker utils
"""
from collections.abc import Mapping
from datetime import datetime
from enum import Enum
from typing import Any, Dict, Optional, Tuple, TypeVar

from dateutil.tz import tzutc


def get_absolute_now() -> datetime:
    """
    Return now with time zone
    """
    return datetime.now(tz=tzutc())


K = TypeVar('K')  # pylint: disable=invalid-name


def get_first_key(mapping: Mapping[K, Any]) -> K:
    """
    Return first (in a sort order) key from dictionary
    """
    try:
        # Value of type variable "SupportsLessThanT" of "sorted" cannot be "K"
        return next(iter(sorted(mapping.keys())))  # type:ignore
    except StopIteration:
        raise IndexError('first key from empty dict')


def get_first_value(mapping: Mapping[Any, K]) -> K:
    """
    Return first value from dictionary
    """
    try:
        return next(iter(mapping.values()))
    except StopIteration:
        raise IndexError('first value from empty dict')


def to_host_list(hosts: dict) -> list:
    """
    Convert hosts from map to list representation.
    """
    return [{'fqdn': host, **opts} for host, opts in hosts.items()]


def to_host_map(hosts: list) -> dict:
    """
    Convert hosts from list to map representation.
    """
    return {host['fqdn']: {x: y for x, y in host.items() if x != 'fqdn'} for host in hosts}


def filter_host_map(hosts: dict, *, role: str = None, subcid: str = None, shard_id: str = None) -> dict:
    """
    Filter dict of hosts by one or several fields from: role, subcid and shard_id.
    """

    def _match(opts):
        if role and role not in opts['roles']:
            return False
        if subcid and subcid != opts['subcid']:
            return False
        if shard_id and shard_id != opts['shard_id']:
            return False
        return True

    return {host: opts for host, opts in hosts.items() if _match(opts)}


def split_host_map(hosts: dict, shard_id: str) -> Tuple[dict, dict]:
    """
    Split dict of hosts by shard ID.
    """
    shard_hosts = {}
    rest_hosts = {}
    for host, opts in hosts.items():
        if shard_id == opts.get('shard_id'):
            shard_hosts[host] = opts
        else:
            rest_hosts[host] = opts

    return shard_hosts, rest_hosts


def get_host_role(host, valid_roles):
    """
    Returns host role.
    """
    for role in valid_roles:
        if role in host['roles']:
            return role


class HostOS(Enum):
    """
    Possible OS
    """

    WINDOWS = 'windows'
    LINUX = 'linux'


class LinuxPaths:
    """
    All file paths used by worker on Lindows
    """

    # pylint: disable=too-few-public-methods
    HOSTNAME = '/etc/hostname'
    MINION_ID = '/etc/salt/minion_id'
    MINION_PEM = '/etc/salt/pki/minion/minion.pem'
    MINION_PUB = '/etc/salt/pki/minion/minion.pub'
    DEPLOY_VERSION = '/etc/yandex/mdb-deploy/deploy_version'
    DEPLOY_API_HOST = '/etc/yandex/mdb-deploy/mdb_deploy_api_host'
    AUTHORIZED_KEYS = '/root/.ssh/authorized_keys'
    AUTHORIZED_KEYS2 = '/root/.ssh/authorized_keys2'


class WindowsPaths:
    """
    All file paths used by worker on Windows
    """

    # pylint: disable=too-few-public-methods
    HOSTNAME = r'C:\salt\conf\minion_id'  # in windows there is no hostname file
    MINION_ID = r'C:\salt\conf\minion_id'
    MINION_PEM = r'C:\salt\conf\pki\minion\minion.pem'
    MINION_PUB = r'C:\salt\conf\pki\minion\minion.pub'
    DEPLOY_VERSION = r'C:\salt\conf\deploy_version'
    DEPLOY_API_HOST = r'C:\salt\conf\mdb_deploy_api_host'
    AUTHORIZED_KEYS = r'C:\Users\Administrator\.ssh\authorized_keys'
    AUTHORIZED_KEYS2 = r'C:\Users\Administrator\.ssh\authorized_keys2'


def get_paths(host_os: str):
    """
    Returns paths set for given operation system
    """
    if host_os == HostOS.WINDOWS.value:
        return WindowsPaths
    return LinuxPaths


def get_image_by_major_version(
    image_template: Optional[Dict[str, Any]], image_fallback: str, task_args: Dict[str, Any]
) -> str:
    """
    get bootstrap_cmd (in porto) or image-search-pattern (in compute)
    by it's definition and task_args
    """

    if not image_template:
        return image_fallback

    param_name = image_template['task_arg']
    param_value = task_args.get(param_name)
    # if no version in task_args or version not whitelisted
    if param_value not in image_template['whitelist']:
        return image_fallback

    # convert param_value by whitelist
    # By 'historical reasons' we might have different major-version notations
    # Example:
    # internal-api - postgresql  10-1c
    # images***    - postgresql-10_1c
    param_value = image_template['whitelist'][param_value]

    try:
        return image_template['template'].format_map({param_name: param_value})
    except KeyError as exc:
        raise RuntimeError(
            f"Image template {image_template['template']} require different args (we have '{param_name}')",
        ) from exc


def get_conductor_root_group(properties, opts):
    """
    Select conductor group with properties and alt group matchers (first match wins)
    """
    group = properties.conductor_root_group
    for alt_group in properties.conductor_alt_groups:
        key = alt_group['matcher']['key']
        values = alt_group['matcher']['values']
        if opts.get(key) in values:
            group = alt_group['group_name']
            break
    return group


def get_eds_root(properties, opts):
    root = properties.eds_root
    for alt_root in properties.eds_alt_roots:
        key = alt_root['matcher']['key']
        values = alt_root['matcher']['values']
        if opts.get(key) in values:
            root = alt_root['root_name']
            break
    return root
