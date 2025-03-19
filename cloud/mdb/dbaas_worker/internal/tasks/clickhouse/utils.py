# -*- coding: utf-8 -*-
"""
Utilities specific for ClickHouse clusters.
"""
from typing import Tuple
from uuid import uuid4

from ...providers.common import Change
from ...providers.deploy import DeployAPI, DeployError
from ...utils import get_first_value
from ..utils import HostGroup, build_host_group, combine_sg_service_rules, get_managed_hostname

CH_ROLE_TYPE = 'clickhouse_cluster'
ZK_ROLE_TYPE = 'zk'


def classify_host_map(hosts: dict, allowed_hosts: list = None, ignored_hosts: list = None) -> Tuple[dict, dict]:
    """
    Classify dict of hosts on ClickHouse and ZooKeeper ones.
    """
    ch_hosts = {}
    zk_hosts = {}
    for host, opts in hosts.items():
        if allowed_hosts is not None and host not in allowed_hosts:
            continue
        if ignored_hosts is not None and host in ignored_hosts:
            continue

        if ZK_ROLE_TYPE in opts.get('roles', []):
            zk_hosts[host] = opts
        else:
            ch_hosts[host] = opts

    return ch_hosts, zk_hosts


def get_kazoo_conn_string(zk_hosts, fallback_hosts):
    """
    Get kazoo zk hosts (use managed hostnames if possible)
    """
    hosts = []
    for host, opts in zk_hosts.items():
        managed_hostname = get_managed_hostname(host, opts)
        hosts.append('{host}:2181'.format(host=managed_hostname))

    if hosts:
        return ','.join(hosts)
    return fallback_hosts


def ch_build_host_groups(
    hosts: dict, config, allowed_hosts: list = None, ignored_hosts: list = None
) -> Tuple[HostGroup, HostGroup]:
    """
    Get ClickHouse and ZooKeeper hosts groups.
    """
    ch_hosts, zk_hosts = classify_host_map(hosts, allowed_hosts, ignored_hosts)

    ch_group = build_host_group(config.clickhouse, ch_hosts)
    assert ch_group.properties
    if ch_hosts:
        ch_group.properties.conductor_group_id = get_first_value(ch_group.hosts)['subcid']

    zk_group = build_host_group(config.zookeeper, zk_hosts)
    assert zk_group.properties
    if zk_hosts:
        zk_group.properties.conductor_group_id = get_first_value(zk_group.hosts)['subcid']

    # Only one service SG per cluster is supported for now. So each host group should have rules
    # combined from all groups.
    sg_service_rules = combine_sg_service_rules(ch_group, zk_group)
    ch_group.properties.sg_service_rules = sg_service_rules
    zk_group.properties.sg_service_rules = sg_service_rules

    return ch_group, zk_group


def create_schema_backup(deploy_api, host):
    """
    Creates schema backup on non target host. Returns backup id.
    """
    backup_id = str(uuid4())
    _run_ch_backup(
        deploy_api,
        host,
        # table schema restored directly from replica, backup needed only for access control objects.
        # Use `--databases _system` to prevent full schema backup which sometime take a lot of time.
        f'backup --schema-only --force --name {backup_id} --databases _system',
        rollback=lambda task, safe_revision: delete_ch_backup(deploy_api, host, backup_id),
    )
    return backup_id


def delete_ch_backup(deploy_api, host, backup_id):
    """
    Removes target backup.
    """
    _run_ch_backup(deploy_api, host, f'delete {backup_id}')


def _run_ch_backup(deploy_api, host, command, timeout=1800, rollback=None):
    ch_backup_command = f'/usr/bin/ch-backup {command}'
    method = {
        'commands': [
            {
                'type': 'state.single',
                'arguments': [
                    'cmd.run',
                    f'name={ch_backup_command}',
                    f'timeout={timeout}',
                    'env={"LC_ALL": "C.UTF-8", "LANG": "C.UTF-8"}',
                ],
                'timeout': timeout,
            }
        ],
        'fqdns': [host],
        'parallel': 1,
        'stopOnErrorCount': 1,
        'timeout': timeout,
    }
    jid = deploy_api.run(
        host,
        deploy_version=deploy_api.get_deploy_version_from_minion(host),
        deploy_title='ch-backup',
        method=method,
        rollback=rollback or Change.noop_rollback,
    )
    deploy_api.wait([jid])


def cache_user_object(
    deploy_api: DeployAPI,
    host: str,
    env: str,
    object_type: str,
    name: str,
    remove_on_error: bool = True,
    timeout: int = 600,
):
    method = {
        'commands': [
            {
                'type': 'state.sls',
                'arguments': [
                    'components.clickhouse_cluster.operations.cache-user-object',
                    'pillar={pillar}'.format(pillar=dict(object_name=name, object_type=object_type)),
                    'concurrent=True',
                    'saltenv={environment}'.format(environment=env),
                    'timeout={timeout}'.format(timeout=timeout),
                ],
                'timeout': timeout,
            },
        ],
        'fqdns': [host],
        'parallel': 1,
        'stopOnErrorCount': 1,
        'timeout': timeout,
    }

    def rollback(_task, _safe_revision):
        return remove_user_object_from_cache(deploy_api, host, object_type, name)

    jid = deploy_api.run(
        host,
        deploy_version=deploy_api.get_deploy_version_from_minion(host),
        deploy_title=f's3-cache-add-{name}',
        method=method,
        rollback=rollback if remove_on_error else lambda task, safe_rev: None,
    )

    try:
        deploy_api.wait([jid])
    except DeployError as e:
        raise DeployError(f'Unable to download {object_type}: {name}', exposable=True) from e


def remove_user_object_from_cache(deploy_api: DeployAPI, host: str, object_type: str, name: str, timeout: int = 600):
    method = {
        'commands': [
            {
                'type': 'mdb_clickhouse.clear_user_object_cache',
                'arguments': [f'name={name}', f'object_type={object_type}'],
                'timeout': timeout,
            }
        ],
        'fqdns': [host],
        'parallel': 1,
        'stopOnErrorCount': 1,
        'timeout': timeout,
    }
    jid = deploy_api.run(
        host,
        deploy_version=deploy_api.get_deploy_version_from_minion(host),
        deploy_title=f's3-cache-remove-{name}',
        method=method,
    )
    deploy_api.wait([jid])


def update_zero_copy_required(args, ch_group, zk_group, cloud_storage):
    """
    Return True if it's required to migrate zero-copy metadata to new schema.
    """
    # Schema migration is not required for clusters without ZooKeeper.
    if not zk_group.hosts:
        return False

    # Schema migration is not required for clusters without hybrid storage (aka cloud storage).
    ch_subcid = get_first_value(ch_group.hosts)['subcid']
    if not cloud_storage.has_exists_cloud_storage_bucket(ch_subcid):
        return False

    # Schema migration is required when we do major version upgrade to the version 22.1 or newer.
    if version_ge(args.get('version_to'), '22.1') and version_lt(args.get('version_from'), '22.1'):
        return True

    # Schema migration is required when explicitly requested.
    # NOTE: This is a deprecated method to trigger schema migration.
    if args.get('update_zero_copy_schema', False):
        return True

    return False


def version_ge(version1, version2):
    """
    Return True if version1 is greater or equal than version2.
    """
    if version1 is None or version2 is None:
        return False

    return parse_version(version1) >= parse_version(version2)


def version_lt(version1, version2):
    """
    Return True if version1 is less than version2.
    """
    if version1 is None or version2 is None:
        return False

    return parse_version(version1) < parse_version(version2)


def parse_version(version):
    """
    Parse version string.
    """
    return [int(x) for x in version.strip().split('.')]
