# -*- coding: utf-8 -*-
"""
Task executors utils
"""
from collections import defaultdict
from copy import deepcopy
from types import SimpleNamespace
from typing import Callable, Dict, List, NamedTuple, Optional, Type, Tuple, Union, Iterable
import string

from ..utils import to_host_list, to_host_map
from .common.base import BaseExecutor
from ..types import HostGroup, GenericHost
from ..providers.juggler import JugglerApi
from ..providers.tls_cert import TLSCert


class FeaturedExecutor(NamedTuple):
    executor: Type[BaseExecutor]
    feature_flag: Optional[str]


EXECUTORS = {}  # type: Dict[str, FeaturedExecutor]
ALPHANUMERIC = string.ascii_letters + string.digits


class GetExecutorException(Exception):
    """
    Base executor get exception
    """


class ExecutorNotDefined(GetExecutorException):
    """
    No such executor exception
    """


class FeatureFlagMissing(GetExecutorException):
    """
    No feature flag on cloud to run executor
    """


class ShardNotHealthyError(Exception):
    """
    Shard health error.
    """

    def __init__(self, shard_id):
        super().__init__('No healthy hosts found in shard \'{0}\''.format(shard_id))


def register_executor(task_type: str, feature_flag: Optional[str] = None):
    """
    Put executor class into executors
    """

    def wrapper(executor: Type[BaseExecutor]):
        """
        Wrapper function for class.
        """
        if task_type not in EXECUTORS:
            EXECUTORS[task_type] = FeaturedExecutor(executor, feature_flag)
            return executor

        first = EXECUTORS[task_type].executor
        raise Exception(f'Multiple declarations of {task_type}. First: {first}. Second: {executor}')

    return wrapper


def get_executor(task, config, queue, task_args):
    """
    Get executor for task_type
    """
    if task['task_type'] not in EXECUTORS:
        raise ExecutorNotDefined('Unable to locate executor for {task_type}'.format(task_type=task['task_type']))
    executor = EXECUTORS[task['task_type']]
    if executor.feature_flag and executor.feature_flag not in task['feature_flags']:
        raise FeatureFlagMissing(
            'No feature flag {flag} on folder '
            'ext_id: {id} to run {task_type}'.format(
                flag=executor.feature_flag, id=task['folder_id'], task_type=task['task_type']
            )
        )
    return EXECUTORS[task['task_type']].executor(config, task, queue, task_args)


def build_host_group(properties: SimpleNamespace, hosts: dict) -> HostGroup:
    """
    Safely build host group
    """
    hosts_copy = deepcopy(hosts)
    for host, opts in hosts_copy.items():
        if opts['vtype'] == 'compute':
            opts['managed_fqdn'] = '{name}.{zone}'.format(
                name=host.split('.')[0], zone=getattr(properties, 'managed_zone')
            )
        if opts['vtype'] == 'aws':
            private_zone = getattr(properties, 'private_zone', '')
            if private_zone != '':
                private_fqdn = host.replace(properties.public_zone, private_zone)
                opts['private_fqdn'] = private_fqdn
                # make wildcard alt_name only
                opts['private_cert_alt_name'] = '.'.join(['*'] + private_fqdn.split('.')[1:])
    return HostGroup(properties=deepcopy(properties), hosts=hosts_copy)


def build_host_group_from_list(properties: SimpleNamespace, hosts: list) -> HostGroup:
    """
    Build host group from list
    """
    return build_host_group(properties, to_host_map(hosts))


def get_managed_hostname(host: str, opts: Union[dict, GenericHost]) -> str:
    """
    Get hostname in managed network for host
    """
    return opts.get('managed_fqdn', host)  # type: ignore


def get_private_hostname(host: str, opts: dict):
    """
    Get hostname in private zone for host
    """
    if 'private_fqdn' not in opts:
        raise Exception(f'No private fqdn for host {host}')
    return opts['private_fqdn']


def get_private_cert_alt_name(host: str, opts: dict):
    """
    Get hostname in private zone for host
    """
    return opts.get('private_cert_alt_name', host)


def choose_sharded_backup_hosts(
    hosts: Dict[str, dict], health_checker: Callable[[str, Dict[str, dict]], bool]
) -> List[str]:
    """
    Get list with one healthy host from each shard
    """
    shards = set()
    shard_host_map = dict()  # type: Dict[str, str]
    for host, opts in hosts.items():
        shard_id = opts['shard_id']

        if shard_id in shard_host_map:
            continue

        shards.add(shard_id)
        if health_checker(host, opts):
            shard_host_map[shard_id] = host

    for shard_id in shards:
        if shard_id not in shard_host_map:
            ShardNotHealthyError(shard_id)

    return list(shard_host_map.values())


def split_hosts_for_modify_by_shard_batches(
    hosts: dict, max_batch_size=10, fast_mode: bool = False
) -> List[dict]:  # pylint: disable=invalid-name
    """
    Split hosts on groups to perform restart-required modify in parallel.

    At first, hosts are split on shard batches unevenly sized by power of 2 (1, 2, 4, 8, ...) up to limit max_batch_size=10.
    Then hosts within each shard batch are split by groups that hosts from different shards are modified in parallel,
    but within one shard sequentially.
    """
    result: List[dict] = []

    if fast_mode:
        batches = [list(group_host_list_by_shards(to_host_list(hosts)).items())]
    else:
        batches = split_host_list_on_shard_batches(to_host_list(hosts), max_batch_size=max_batch_size)

    for batch in batches:
        batch_lengths = [len(host_list) for _, host_list in batch]
        max_shard_size = max(batch_lengths) if batch_lengths else 0
        for i in range(max_shard_size):
            host_group = []
            for _, host_list in batch:
                if len(host_list) > i:
                    host_group.append(host_list[i])
            result.append(to_host_map(host_group))

    return result


def split_host_list_on_shard_batches(
    hosts: list, max_batch_size=0
) -> List[List[Tuple[str, list]]]:  # pylint: disable=invalid-name
    """
    Split hosts on shard batches unevenly sized by power of 2 (1, 2, 4, 8, ...) up to limit max_batch_size
    where each shard batch is a list of tuples: shard id - shard hosts.
    For unlimited batch set max_batch_size=0
    """
    result = []

    i = 0
    batch_size = 1
    shard_hosts_map = group_host_list_by_shards(hosts)
    tuples = list(shard_hosts_map.items())
    while i < len(tuples):
        result.append(tuples[i : i + batch_size])
        i += batch_size
        batch_size *= 2
        if max_batch_size > 0 and batch_size > max_batch_size:
            batch_size = max_batch_size

    return result


def group_host_list_by_shards(hosts: list) -> Dict[str, list]:
    """
    Group hosts by shard id.
    """
    result: Dict[str, list] = defaultdict(list)
    for host in hosts:
        result[host['shard_id']].append(host)

    return result


def group_host_list_by_shard_names(hosts: list) -> Dict[str, list]:
    """
    Group hosts by shard name.
    """
    result: Dict[str, list] = defaultdict(list)
    for host in hosts:
        result[host['shard_name']].append(host)

    return result


def sort_hosts_by_fqdn(hosts: Iterable[GenericHost]) -> List[GenericHost]:
    """
    Sorts list of hosts by their FQDNs in ascending order
    """
    return sorted(hosts, key=lambda host: host['fqdn'])


def select_host(hosts: Iterable[GenericHost]) -> str:
    """
    Returns first host's (in order of sorted host FQDNs) FQDN
    """
    return sort_hosts_by_fqdn(hosts)[0]['fqdn']


def resolve_order(order, action):
    """
    Dynamic order resolver
    """
    if isinstance(order, Callable):
        return order(action)
    return order


def issue_tls_wrapper(cid: str, host_group, tls_cert: TLSCert, force: bool = False, force_tls_certs: bool = False):
    """
    Helper function for creating TLS certificate for hosts in cluster
    """
    if host_group.properties.issue_tls:
        for host, opts in host_group.hosts.items():
            alt_names = []
            managed_hostname = get_managed_hostname(host, opts)
            if managed_hostname != host:
                alt_names.append(managed_hostname)

            private_hostname = get_private_cert_alt_name(host, opts)
            if private_hostname != host:
                alt_names.append(private_hostname)

            for dns_group in host_group.properties.group_dns:
                if dns_group.get('disable_tls_altname'):
                    continue
                id_key = dns_group['id']
                if id_key == 'cid':
                    group_name = dns_group['pattern'].format(id=cid)
                else:
                    group_name = dns_group['pattern'].format(id=opts[id_key], cid=cid)
                alt_names.append(group_name)
            tls_cert.exists(host, alt_names=alt_names, force=force, force_tls_certs=force_tls_certs)


def combine_sg_service_rules(*host_groups: HostGroup) -> List[dict]:
    """
    Combine service SG rules of multiple host groups.
    """
    sg_service_rules = []
    for host_group in host_groups:
        hg_rules = getattr(host_group.properties, 'sg_service_rules', {})
        for rule in hg_rules:
            if rule not in sg_service_rules:
                sg_service_rules.append(rule)

    return sg_service_rules


class Downtime:
    def __init__(self, juggler: JugglerApi, host_group: HostGroup, timeout):
        self.juggler = juggler
        self.host_group = host_group
        self.timeout = timeout

    def __enter__(self):
        for host, opts in self.host_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            self.juggler.downtime_exists(managed_hostname, duration=self.timeout)
        return self

    def __exit__(self, type, value, traceback):
        for host, opts in self.host_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            self.juggler.downtime_absent(managed_hostname)


def get_cluster_encryption_key_alias(host_group: HostGroup, cid: str) -> str:
    for host in host_group.hosts.values():
        if host['vtype'] == 'aws':
            return f'alias/mdb-dataplane-encryption-{cid}'

    return ''
