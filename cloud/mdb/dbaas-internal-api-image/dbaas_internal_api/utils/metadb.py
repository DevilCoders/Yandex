# -*- coding: utf-8 -*-
# pylint: disable=too-many-lines
"""
DBaaS Internal API utils for operations with metadb.
Contains some wrappers on metadb operations
"""

import json
import psycopg2.errorcodes as pgcode
import psycopg2.extensions
from contextlib import contextmanager
from datetime import datetime, timedelta
from flask import g
from functools import wraps
from typing import Dict, Iterable, List, Optional, Set, Type, TypeVar, Union, Any

from . import config
from .metadata import Metadata
from .tasks_version import TASKS_VERSION
from .types import Host, MetadbHostInfo
from ..core import exceptions as errors
from ..core.base_pillar import BasePillar
from ..core.id_generators import gen_id
from ..core.types import CID, Operation, make_operation
from ..utils.feature_flags import get_feature_flags
from ..utils.request_context import get_rev, get_x_request_id, set_rev
from ..utils.types import (
    ClusterVisibility,
    ComparableEnum,
    IdempotenceData,
    BackupStatus,
    Alert,
    ManagedBackup,
    BackupInitiatorFromDB,
    BackupMethodFromDB,
)
from ..utils.version import Version

T = TypeVar('T')  # pylint: disable=invalid-name
PillarT = Union[BasePillar, dict]


@contextmanager
def commit_on_success():
    """
    Transaction context manager.

    Commit if no exception raised
    """
    yield
    g.metadb.commit()


@contextmanager
def cluster_change(cid):
    """
    Lock cluster and complete change if it success
    """
    with commit_on_success():
        cluster = lock_cluster(cid)
        yield cluster
        complete_cluster_change(cid)


def assert_one_exists(*args) -> None:
    """
    This function need for development. It raises
    assertion error if all arguments is None. This
    need to detect inaccurate function calls.
    """
    assert any([elem is not None for elem in args]), 'one of args should be not None'


def get_first_elem(iterable: Iterable[T]) -> Optional[T]:
    """
    Get first element from iterable or None if iterable is empty
    """
    for elem in iterable:
        return elem
    return None


def returns_first_elem(fun):
    """
    Decorator which return first element from result list.
    Returns none if list is empty. Expected that type of
    result is list, otherwise AssertionError will be raised.
    """

    @wraps(fun)
    def wrapper(*args, **kwargs):
        """
        Wrapper function. Returns first element of result
        """
        return get_first_elem(fun(*args, **kwargs))

    return wrapper


def returns_attr(attribute):
    """
    Decorator which return element attribute with a given name.
    Return none if the element is none.
    Raise AssertionError if the element has no required attribute.
    """

    def decorator(fun):
        """
        Wrapper function. Returns wrapper function.
        """

        @wraps(fun)
        def wrapper(*args, **kwargs):
            """
            Wrapper function. Returns attribute of function result
            """
            result = fun(*args, **kwargs)

            if not result:
                return None

            if isinstance(result, list):
                return [row[attribute] for row in result]

            assert attribute in result, 'key {0} not found in dict {1}'.format(attribute, result)
            return result[attribute]

        return wrapper

    return decorator


def get_valid_resources(cluster_type, role=None, geo=None, resource_preset_id=None, disk_type_ext_id=None):
    """
    Returns valid resources combinations
    """
    return g.metadb.query(
        'get_valid_resources',
        cluster_type=cluster_type,
        role=role,
        geo=geo,
        resource_preset_id=resource_preset_id,
        disk_type_ext_id=disk_type_ext_id,
        feature_flags=get_feature_flags(),
    )


def get_resource_presets(
    cluster_type, decommissioning_flavors=None, page_token_cores=None, page_token_id=None, limit=100
):
    """
    Returns valid resource presets for cluster_type
    """
    if decommissioning_flavors is None:
        decommissioning_flavors = []
    return g.metadb.query(
        'get_resource_presets',
        cluster_type=cluster_type,
        resource_preset_id=None,
        decommissioning_flavors=decommissioning_flavors,
        page_token_cores=page_token_cores,
        page_token_id=page_token_id,
        limit=limit,
        feature_flags=get_feature_flags(),
    )


@returns_first_elem
def get_resource_preset_by_id(cluster_type, resource_preset_id):
    """
    Returns valid resource presets for cluster_type
    """
    return g.metadb.query(
        'get_resource_presets',
        cluster_type=cluster_type,
        resource_preset_id=resource_preset_id,
        page_token_cores=None,
        page_token_id=None,
        decommissioning_flavors=[],
        limit=None,
        feature_flags=get_feature_flags(),
    )


def get_flavors(name=None, limit=10, last_cpu_limit=None, last_id=None):
    """
    Returns all flavors
    """
    return g.metadb.query(
        'get_flavors', instance_type_name=name, limit=limit, last_cpu_limit=last_cpu_limit, last_id=last_id
    )


@returns_first_elem
def get_flavor_by_name(name):
    """
    Search flavor with specified name and return it
    """
    return g.metadb.query('get_flavor_by_name', flavor_name=name)


def get_resource_presets_by_cluster_type(cluster_type):
    """
    Returns valid resource presets for specified cluster type.
    """
    return g.metadb.query(
        'get_resource_presets_by_cluster_type',
        cluster_type=cluster_type,
        feature_flags=get_feature_flags(),
    )


@returns_first_elem
def get_resource_preset_by_cpu(
    cluster_type: str,
    role: Optional[str] = None,
    flavor_type: Optional[str] = None,
    generation: Optional[int] = None,
    min_cpu: Optional[int] = None,
    zones: Optional[Set[str]] = None,
):
    """
    Returns valid resource preset for specified cluster type, role and flavor type
    with at least `min_cpu` cores in cpu_limit, which is available in all specified `zones`.
    """
    return g.metadb.query(
        'get_resource_preset_by_cpu',
        cluster_type=cluster_type,
        role=role,
        flavor_type=flavor_type,
        generation=generation,
        min_cpu=min_cpu,
        zones=list(zones) if zones else [],
        feature_flags=get_feature_flags(),
        decommissioning_flavors=list(config.get_decommissioning_flavors(force=True)),
    )


@returns_first_elem
def create_folder(folder_ext_id, cloud_id):
    """
    Create folder if not exists. Returns folder.
    """
    return g.metadb.query('create_folder', folder_ext_id=folder_ext_id, cloud_id=cloud_id, master=True)


@returns_first_elem
def get_instance_group(subcid):
    """
    Get instance group by subcluster id
    """
    return g.metadb.query('get_instance_group', subcid=subcid)


@returns_first_elem
def get_subcluster_by_instance_group(instance_group_id):
    """
    Get subcluster by instance group id
    """
    return g.metadb.query('get_subcluster_by_instance_group_id', instance_group_id=instance_group_id)


def get_instance_groups(cid):
    """
    Get instance groups by cluster id
    """
    return g.metadb.query('get_instance_groups', cid=cid)


@returns_first_elem
def create_cloud(cloud_ext_id, quota: dict):
    """
    Create cloud if not exists. Returns cloud.
    """
    ret = g.metadb.query(
        'create_cloud', cloud_ext_id=cloud_ext_id, x_request_id=get_x_request_id(), **quota, master=True
    )
    ret[0]['feature_flags'] = get_default_feature_flags() or []
    return ret


@returns_attr('flag_name')
def get_default_feature_flags():
    """
    Returns list of default feature flags
    """
    return g.metadb.query('get_default_feature_flags')


@returns_first_elem
def get_folder(*, folder_id=None, folder_ext_id=None):
    """
    Get folder by one of its params (`folder_id` or `folder_ext_id`).
    """
    assert_one_exists(folder_id, folder_ext_id)
    return g.metadb.query('get_folder', folder_id=folder_id, folder_ext_id=folder_ext_id)


@returns_first_elem
def get_cloud(*, cloud_id=None, cloud_ext_id=None):
    """
    Get cloud by one of its params (`cloud_id` or `cloud_ext_id`).
    """
    assert_one_exists(cloud_id, cloud_ext_id)
    return g.metadb.query('get_cloud', cloud_id=cloud_id, cloud_ext_id=cloud_ext_id)


def lock_cloud(update_context=True):
    """
    Locks cloud in FOR UPDATE mode, updates cloud in context and returns
    True on success.
    """
    res = g.metadb.query('lock_cloud', cloud_id=g.cloud['cloud_id'], master=True)
    if res:
        if update_context:
            feature_flags = g.cloud['feature_flags']
            g.cloud = res[0]
            g.cloud['feature_flags'] = feature_flags
        return True


@returns_first_elem
def lock_pillar(*, cid=None, subcid=None, shard_id=None, fqdn=None):
    """
    Locks pillar in FOR UPDATE mode and returns it.
    One of `cid`, `subcid`, `shard_id`, `fqdn` must exists.
    """
    assert_one_exists(cid, subcid, shard_id, fqdn)
    return g.metadb.query('lock_pillar', cid=cid, subcid=subcid, shard_id=shard_id, fqdn=fqdn, master=True)


@returns_first_elem
def get_folder_by_cluster(*, cid=None, cluster_name=None, visibility: ClusterVisibility = ClusterVisibility.visible):
    """
    Get folder by cluster `cid` or `cluster_name`.
    """
    assert_one_exists(cid, cluster_name)
    return g.metadb.query('get_folder_by_cluster', cid=cid, cluster_name=cluster_name, visibility=visibility.value)


@returns_first_elem
def get_folder_by_operation(*, operation_id=None):
    """
    Get folder by cluster `operation id` (aka task_id).
    """
    return g.metadb.query('get_folder_by_task', task_id=operation_id, master=True)


def get_folders_by_cloud(cloud_ext_id: str) -> List[dict]:
    """
    Get folders by cloud_ext_id
    """
    return g.metadb.query('get_folders_by_cloud', cloud_ext_id=cloud_ext_id)


@returns_attr('value')
@returns_first_elem
def get_cluster_type_pillar(cluster_type):
    """
    Get cluster type pillar for given cluster type
    """
    return g.metadb.query('get_cluster_type_pillar', cluster_type=cluster_type)


def get_pillar_by_host(fqdn, target_id):
    """
    Get pillar for cluster, subcluster, shard and host by host name
    """
    return g.metadb.query('get_pillar_by_host', fqdn=fqdn, target_id=target_id)


def get_custom_pillar_by_host(fqdn):
    """
    Get custom pillar by host name
    """
    return g.metadb.query('get_custom_pillar_by_host', fqdn=fqdn)


@returns_attr('value')
@returns_first_elem
def get_fqdn_pillar(fqdn, target_id=None) -> Dict[str, Any]:
    """
    Get pillar (JSON) for host
    """
    return g.metadb.query('get_fqdn_pillar', fqdn=fqdn, target_id=target_id)


@returns_first_elem
def get_cluster_by_managed_host(host):
    """
    Get cluster info by managed host
    """
    return g.metadb.query('get_cluster_by_managed_host', fqdn=host)


@returns_first_elem
def get_cluster_by_unmanaged_host(host):
    """
    Get cluster info by unmanaged host
    """
    return g.metadb.query('get_cluster_by_unmanaged_host', fqdn=host)


def get_subcluster_pillars_by_cluster(cid):
    """
    Get subclusters with pillar by cluster id

    Can be used for deleted clusters, cause don't check visibility
    """
    return g.metadb.query('get_subcluster_pillars_by_cluster', cid=cid)


def get_subclusters_at_rev(cid: str, rev: int) -> List[dict]:
    """
    Get subclusters with pillar by cluster id and rev
    """
    return g.metadb.query('get_subclusters_at_rev', cid=cid, rev=rev)


def get_subclusters_by_cluster(cid, page_token_subcid=None, limit=None):
    """
    Get metainfo of subclusters by cluster id
    """
    return g.metadb.query('get_subclusters_by_cluster', cid=cid, page_token_subcid=page_token_subcid, limit=limit)


@returns_first_elem
def get_subcluster_info(subcid):
    """
    Get subcluster info by id
    """
    return g.metadb.query('get_subcluster_info', subcid=subcid)


def get_shards(*, cid=None, subcid=None, role=None):
    """
    Get shards.
    """
    assert_one_exists(cid, subcid)
    return g.metadb.query('get_shards', cid=cid, subcid=subcid, role=role)


def get_shards_at_rev(cid: str, rev: int) -> List[dict]:
    """
    Get shards at given rev
    """
    return g.metadb.query('get_shards_at_rev', cid=cid, rev=rev)


@returns_first_elem
def get_oldest_shard(cid, role=None):
    """
    Get shard with min created_at value.
    """
    return g.metadb.query('get_oldest_shard', cid=cid, role=role)


@returns_first_elem
def get_cluster(
    *, cid=None, cluster_name=None, cluster_type=None, visibility: ClusterVisibility = ClusterVisibility.visible
):
    """
    Get cluster info by cluster id or name.
    """
    assert_one_exists(cid, cluster_name)
    return g.metadb.query(
        'get_cluster', cid=cid, cluster_name=cluster_name, cluster_type=cluster_type, visibility=visibility.value
    )


def get_cluster_at_rev(cid: str, rev: int) -> dict:
    """
    Get cluster at rev
    """
    rows = g.metadb.query(
        'get_cluster_at_rev',
        cid=cid,
        rev=rev,
    )
    if not rows:
        raise RuntimeError(f'No data found for "{cid}" cluster at "{rev}" rev')
    return rows[0]


def lock_cluster(cid: str) -> Optional[dict]:
    """
    Lock cluster and get info by cluster id.
    """
    ret = g.metadb.query('lock_cluster', cid=cid, x_request_id=get_x_request_id(), master=True)
    if ret:
        ret = ret[0]
        set_rev(ret['cid'], ret['rev'])
        return ret
    return None


def complete_cluster_change(cid: str) -> None:
    """
    Complete cluster change.
    """
    g.metadb.query('complete_cluster_change', cid=cid, rev=get_rev(cid), master=True)


def get_clusters_by_folder(
    limit: Optional[int],
    cluster_name: str = None,
    cluster_type: str = None,
    cid: str = None,
    env: str = None,
    page_token_name: str = None,
    visibility: ClusterVisibility = ClusterVisibility.visible,
    folder_id: int = None,
):
    """
    Get clusters by folder with optional filters
    """
    assert not (cid and cluster_name), 'you may not specify both cluster_name and cid at the same time'
    if folder_id is None:
        folder_id = g.folder['folder_id']
    return g.metadb.query(
        'get_clusters_by_folder',
        folder_id=folder_id,
        cluster_name=cluster_name,
        cluster_type=cluster_type,
        env=env,
        cid=cid,
        page_token_name=page_token_name,
        limit=limit,
        visibility=visibility.value,
    )


def get_clusters_change_dates(cids: List[str]) -> List[dict]:
    """
    Get dates with given clusters last changed by a user
    - ignore hidden tasks
    - ignore maintenance tasks
    """
    return g.metadb.query(
        'get_clusters_change_dates',
        cids=cids,
    )


def get_clusters_essence_by_folder(
    limit: Optional[int],
    cluster_name: str = None,
    cluster_type: str = None,
    cid: str = None,
    env: str = None,
    page_token_name: str = None,
) -> List[dict]:
    """
    Get clusters by folder with optional filters
    """
    assert not (cid and cluster_name), 'you may not specify both cluster_name and cid at the same time'
    return g.metadb.query(
        'get_clusters_essence_by_folder',
        folder_id=g.folder['folder_id'],
        cluster_name=cluster_name,
        cluster_type=cluster_type,
        env=env,
        cid=cid,
        page_token_name=page_token_name,
        limit=limit,
    )


def get_cluster_rev_by_timestamp(cid: str, timestamp: datetime) -> int:
    """
    Find cluster rev by time
    """
    rows = g.metadb.query(
        'get_cluster_rev_by_timestamp',
        cid=cid,
        timestamp=timestamp,
    )

    if not rows:
        raise RuntimeError(
            f'Unable to find "{cid}" cluster rev for {timestamp}. '
            'Cluster does not exist or no changes near that timestamp'
        )

    cluster_rev = rows[0]['rev']

    if cluster_rev is None:
        raise RuntimeError(f'Looks like there are no "{cid}" cluster revs below "{timestamp}"')

    return cluster_rev


@returns_first_elem
# pylint: disable=too-many-arguments
def update_cloud_quota(
    cloud_ext_id,
    add_cpu=0,
    add_gpu=0,
    add_memory=0,
    add_ssd_space=0,
    add_hdd_space=0,
    add_clusters=0,
):
    """
    Change cloud quota
    """
    return g.metadb.query(
        'update_cloud_quota',
        master=True,
        cloud_ext_id=cloud_ext_id,
        x_request_id=get_x_request_id(),
        add_cpu=add_cpu,
        add_gpu=add_gpu,
        add_memory=add_memory,
        add_ssd_space=add_ssd_space,
        add_hdd_space=add_hdd_space,
        add_clusters=add_clusters,
    )


@returns_first_elem
# pylint: disable=too-many-arguments,invalid-name
def set_cloud_quota(cloud_ext_id, cpu=None, gpu=None, memory=None, ssd_space=None, hdd_space=None, clusters=None):
    """
    Change cloud quota
    """
    return g.metadb.query(
        'set_cloud_quota',
        master=True,
        cloud_ext_id=cloud_ext_id,
        x_request_id=get_x_request_id(),
        cpu=cpu,
        gpu=gpu,
        memory=memory,
        ssd_space=ssd_space,
        hdd_space=hdd_space,
        clusters=clusters,
    )


def get_hosts(cid: str, *, role=None, subcid=None) -> List[Host]:
    """
    Get list of hosts in cluster by cluster id, and optionally filter out the result host list by role and subcid
    and deleted status
    """
    return g.metadb.query('get_hosts', cid=cid, role=role, subcid=subcid)


def get_hosts_at_rev(cid: str, rev: int) -> List[Host]:
    """
    Get list of hosts in cluster by cluster id at given rev
    """
    return g.metadb.query('get_hosts_at_rev', cid=cid, rev=rev)


def get_shard_hosts(shard_id) -> List[Host]:
    """
    Get list of hosts in shard.
    """
    return g.metadb.query('get_hosts_by_shard', shard_id=shard_id)


def get_host_info(fqdn) -> MetadbHostInfo:
    """
    Get info about host by fqdn
    """
    raw = g.metadb.query('get_host_info', fqdn=fqdn)
    for host in raw:
        return MetadbHostInfo.from_dict(host)
    raise RuntimeError(f'No host found for "{fqdn}"')


def delete_hosts_batch(fqdns: List[str], cid: str) -> List[dict]:
    """
    Delete hosts by fqdns
    """
    return g.metadb.query('delete_host', fqdns=fqdns, cid=cid, rev=get_rev(cid), master=True)


def delete_host(fqdn: str, cid: str) -> Optional[dict]:
    """
    Delete host by fqdn
    """
    deleted = delete_hosts_batch([fqdn], cid)
    if deleted:
        return deleted[0]
    return None


@returns_first_elem
def get_flavor_by_id(flavor_id):
    """
    Get flavor by flavor id
    """
    return g.metadb.query('get_flavor_by_id', flavor_id=flavor_id)


@returns_attr('name')
def get_flavor_name_by_id(flavor_id):
    """
    Get flavor name by flavor id
    """
    return get_flavor_by_id(flavor_id)


def get_space_quota_map():
    """
    Get map of disk_type_id into disk space quota type
    """
    ret = {}
    types = g.metadb.query('get_disk_types')
    for disk_type in types:
        ret[disk_type['disk_type_ext_id']] = disk_type['quota_type']

    return ret


def delete_cluster(cid):
    """
    Delete cluster (returning delete hosts list and previously undeleted hosts dict)
    """
    lock_cloud()

    if get_running_task(cid=cid):
        raise errors.ActiveTasksExistError(cid)

    hosts = get_hosts(cid)
    if not cluster_is_managed(cid):
        terminate_hadoop_jobs(cid)
        return hosts, get_undeleted_hosts(cid)

    flavors = {}
    space_quota_map = get_space_quota_map()
    ssd_space = 0
    hdd_space = 0
    for host in hosts:
        if space_quota_map[host['disk_type_id']] == 'hdd':
            hdd_space += host['space_limit']
        elif space_quota_map[host['disk_type_id']] == 'ssd':
            ssd_space += host['space_limit']
        else:
            raise RuntimeError(
                'Unexpected space quota type for disk_type_id {id}: {value}'.format(
                    id=host['disk_type_id'], value=space_quota_map[host['disk_type_id']]
                )
            )
        if host['flavor'] not in flavors:
            flavors[host['flavor']] = 0
        flavors[host['flavor']] += 1

    resources = {
        'add_clusters': -1,
        'add_cpu': 0,
        'add_gpu': 0,
        'add_memory': 0,
        'add_ssd_space': -ssd_space,
        'add_hdd_space': -hdd_space,
    }

    for flavor_id, count in flavors.items():
        flavor = get_flavor_by_id(flavor_id)
        resources['add_cpu'] -= count * flavor['cpu_guarantee']
        resources['add_gpu'] -= count * flavor['gpu_limit']
        resources['add_memory'] -= count * flavor['memory_guarantee']
    cloud_update_used_resources(**resources)

    return hosts, get_undeleted_hosts(cid)


def get_undeleted_hosts(cid):
    """
    Get hosts from failed host/shard/subcluster delete tasks
    """
    host = g.metadb.query('get_undeleted_host', cid=cid)
    if host:
        return {host[0]['host']['fqdn']: host[0]['host']}
    shard_hosts = g.metadb.query('get_undeleted_shard_hosts', cid=cid)
    if shard_hosts:
        return {host['fqdn']: host for host in shard_hosts[0]['shard_hosts']}
    subcluster_host_lists = g.metadb.query('get_undeleted_subcluster_hosts', cid=cid)
    # example: 3 failed modify tasks for instance group subcluster produces result such as
    #   [RealDictRow([('host_list', None)]), RealDictRow([('host_list', None)]), RealDictRow([('host_list', None)])]
    undeleted_hosts = {}
    for subcluster_host_list in subcluster_host_lists:
        if subcluster_host_list['host_list']:
            for host in subcluster_host_list['host_list']:
                undeleted_hosts[host['fqdn']] = host
    return undeleted_hosts


@returns_first_elem
def delete_shard(shard_id, cid):
    """
    Delete shard.
    """
    return g.metadb.query('delete_shard', shard_id=shard_id, cid=cid, rev=get_rev(cid))


@returns_first_elem
def delete_subcluster(subcid, cid):
    """
    Delete subcluster.
    """
    return g.metadb.query('delete_subcluster', subcid=subcid, cid=cid, rev=get_rev(cid))


@returns_first_elem
def get_task_by_idempotence_id(idempotence_id):
    """
    Get task_id by idempotence id
    """
    return g.metadb.query(
        'get_task_by_idempotence_id', idempotence_id=idempotence_id, folder_id=g.folder['folder_id'], user_id=g.user_id
    )


def get_operations(
    limit: Optional[int],
    cluster_id: CID = None,
    environment: str = None,
    cluster_type: str = None,
    created_by: str = None,
    operation_type: ComparableEnum = None,
    page_token_id: str = None,
    page_token_created_at: datetime = None,
    include_hidden: bool = None,
    folder_id: int = None,
) -> List[Operation]:
    # pylint: disable=too-many-arguments
    """
    Get operations
    """
    if folder_id is None:
        folder_id = g.folder['folder_id']
    return [
        make_operation(d)
        for d in g.metadb.query(
            'get_operations',
            cid=cluster_id,
            folder_id=folder_id,
            env=environment,
            cluster_type=cluster_type,
            created_by=created_by,
            operation_type=operation_type,
            page_token_id=page_token_id,
            page_token_create_ts=page_token_created_at,
            limit=limit,
            include_hidden=include_hidden,
        )
    ]


def get_cluster_delete_operation(cluster_id: CID, operations_enum: Type[ComparableEnum]) -> Optional[Operation]:
    """
    Get delete operation for cluster
    """
    return get_first_elem(
        get_operations(
            limit=1,
            cluster_id=cluster_id,
            operation_type=operations_enum.delete,  # type: ignore
        )
    )


def get_cluster_stop_operation(cluster_id: CID, operations_enum: Type[ComparableEnum]) -> Optional[Operation]:
    """
    Get last stop operation for cluster
    """
    return get_first_elem(
        get_operations(
            limit=1,
            cluster_id=cluster_id,
            operation_type=operations_enum.stop,  # type: ignore
        )
    )


def get_operation_by_id(operation_id: str, include_hidden: bool = False) -> Optional[Operation]:
    """
    Get operation by its id
    """
    return get_first_elem(
        make_operation(d)
        for d in g.metadb.query(
            'get_operation_by_id',
            folder_id=g.folder['folder_id'],
            operation_id=operation_id,
        )
        if include_hidden or not d['hidden']
    )


@returns_first_elem
def get_hadoop_job(job_id: str, cluster_id: str = None) -> Dict:
    """
    Returns Hadoop job by id or id and cid.
    """
    return g.metadb.query('get_hadoop_job', job_id=job_id, cid=cluster_id)


def get_hadoop_jobs_by_cluster(
    cluster_id: str = None,
    statuses: List[str] = None,
    page_token_create_ts: datetime = None,
    page_token_job_id: str = None,
    limit: Optional[int] = None,
) -> List[Dict]:
    """
    Returns Hadoop job by id or id and cid.
    """
    return g.metadb.query(
        'get_hadoop_jobs_by_cluster',
        cid=cluster_id,
        statuses=statuses,
        page_token_create_ts=page_token_create_ts,
        page_token_job_id=page_token_job_id,
        limit=limit,
    )


def get_hadoop_jobs_by_service(
    statuses: List[str] = None,
    page_token_create_ts: datetime = None,
    page_token_job_id: str = None,
    limit: Optional[int] = None,
) -> List[Dict]:
    """
    Returns Hadoop job by id or id and cid.
    """
    return g.metadb.query(
        'get_hadoop_jobs_by_service',
        statuses=statuses,
        page_token_create_ts=page_token_create_ts,
        page_token_job_id=page_token_job_id,
        limit=limit,
    )


def add_cluster(
    cid,
    name,
    cluster_type,
    env,
    network_id,
    public_secret,
    description,
    host_group_ids,
    deletion_protection,
    monitoring_cloud_id,
) -> dict:
    """
    Creates new cluster in metaDB and returns it
    """
    try:
        ret = g.metadb.query(
            'add_cluster',
            master=True,
            folder_id=g.folder['folder_id'],
            cid=cid,
            name=name,
            type=cluster_type,
            env=env,
            network_id=network_id,
            public_key=public_secret,
            description=description,
            x_request_id=get_x_request_id(),
            host_group_ids=host_group_ids,
            deletion_protection=deletion_protection,
            monitoring_cloud_id=monitoring_cloud_id,
        )[0]
        set_rev(cid, ret['rev'])
        return ret
    except psycopg2.IntegrityError as error:
        # We need to handle unique violation
        # on `name` column in `dbaas.clusters`
        if error.pgcode == pgcode.UNIQUE_VIOLATION:
            raise errors.ClusterAlreadyExistsError(name) from error
        raise error


def add_backup_schedule(cid: str, backup_schedule: dict) -> None:
    """
    Add backup schedule for the cluster
    """
    g.metadb.query(
        'add_backup_schedule',
        master=True,
        fetch=False,
        cid=cid,
        schedule=backup_schedule,
        rev=get_rev(cid),
    )


def get_clusters_versions(component: str, cids: List[str]) -> Dict[str, Dict]:
    """
    lookup dbaas.version table and return dict{cids: Dict_of_versions}
    :param component:
    :param cids:
    :param column:
    :return:
    """

    rows = g.metadb.query(
        'get_clusters_versions',
        fetch=True,
        cids=cids,
        component=component,
    )
    return {row['cid']: row for row in rows}


def get_cluster_version_at_rev(component: str, cid: str, rev: int) -> Dict:
    """
    lookup dbaas.versions_revs table and return version
    :param component:
    :param cids:
    :param rev:
    :return:
    """

    return g.metadb.query(
        'get_cluster_version_at_rev',
        fetch=True,
        cid=cid,
        rev=rev,
        component=component,
    )


def get_subclusters_versions(component: str, cids: List[str]) -> Dict[str, Dict]:
    """
    lookup dbaas.version table and return dict{cids: {subcids: Dict_of_versions}}
    :param component:
    :param cids:
    :param column:
    :return:
    """

    rows = g.metadb.query(
        'get_subclusters_versions',
        fetch=True,
        cids=cids,
        component=component,
    )
    res: dict[str, dict] = {}
    for row in rows:
        cid = row['cid']
        subcid = row['subcid']
        cid_versions = res.get(cid, {})
        cid_versions[subcid] = row
        res[cid] = cid_versions
    return res


def get_default_versions(component: str, env: str, type: str) -> Dict[Version, dict]:
    """
    lookup dbaas.default_versions table for given :type, :env, :component
    """
    metadb_default_versions = {}
    rows = g.metadb.query(
        'get_default_versions',
        fetch=True,
        env=env,
        type=type,
        component=component,
    )
    for row in rows:
        metadb_default_versions[Version.load(row['name'])] = {
            'is_deprecated': row['is_deprecated'],
            'major_version': row['major_version'],
            'edition': row['edition'],
            'updatable_to': row['updatable_to'],
        }
    return metadb_default_versions


def set_default_versions(
    cid: str,
    subcid: Optional[str],
    shard_id: Optional[str],
    env: str,
    major_version: Union[Version, str],
    edition: str,
    ctype: str,
) -> None:
    """
    Fill dbaas.version table for given :cid and :major_version with values from dbaas.default_version
    """
    str_major_version = major_version if isinstance(major_version, str) else str(major_version)

    g.metadb.query(
        'set_default_versions',
        master=True,
        fetch=False,
        cid=cid,
        subcid=subcid,
        shard_id=shard_id,
        env=env,
        ctype=ctype,
        major_version=str_major_version,
        edition=edition,
        rev=get_rev(cid),
    )


def set_labels_on_cluster(cid: str, labels: Dict[str, str]) -> None:
    """
    Set labels on cluster
    """
    # psycopg2 adapt tuples as RECORD,
    # which we can cast to code.label
    labels_tuples = list(labels.items())
    g.metadb.query(
        'set_labels_on_cluster',
        master=True,
        fetch=False,
        folder_id=g.folder['folder_id'],
        cid=cid,
        labels=labels_tuples,
        rev=get_rev(cid),
    )


def update_cluster_description(cid: str, description: Optional[str]) -> None:
    """
    Update cluster description
    """
    g.metadb.query(
        'update_cluster_description',
        master=True,
        fetch=False,
        cid=cid,
        description=description,
        rev=get_rev(cid),
    )


def set_maintenance_window_settings(cid: str, day: Optional[str], hour: Optional[int]) -> None:
    """
    Update cluster description
    """
    g.metadb.query(
        'set_maintenance_window_settings',
        master=True,
        fetch=False,
        cid=cid,
        day=day,
        hour=hour,
        rev=get_rev(cid),
    )


def update_cluster_deletion_protection(cid: str, deletion_protection: bool) -> None:
    """
    Update cluster deletion_protection
    """
    g.metadb.query(
        'update_cluster_deletion_protection',
        master=True,
        fetch=False,
        cid=cid,
        deletion_protection=deletion_protection,
        rev=get_rev(cid),
    )


def reschedule_maintenance_task(cid: str, config_id: str, plan_ts: datetime) -> None:
    """
    Set new time of planned maintenance operation
    """
    g.metadb.query(
        'reschedule_maintenance_task',
        master=True,
        fetch=False,
        cid=cid,
        config_id=config_id,
        plan_ts=plan_ts,
    )


def update_cluster_name(cid: CID, name: str) -> None:
    """
    Updates cluster name
    """
    try:
        g.metadb.query(
            'update_cluster_name',
            master=True,
            fetch=False,
            cid=cid,
            name=name,
            rev=get_rev(cid),
        )
    except psycopg2.IntegrityError as error:
        # We need to handle unique violation
        # on `name` column in `dbaas.clusters`
        if error.pgcode == pgcode.UNIQUE_VIOLATION:
            raise errors.ClusterAlreadyExistsError(name) from error
        raise error


@returns_first_elem
def add_subcluster(cluster_id, subcid, name, roles):
    """
    Creates new subcluster in metaDB and returns it.
    """
    return g.metadb.query(
        'add_subcluster', master=True, cid=cluster_id, subcid=subcid, name=name, roles=roles, rev=get_rev(cluster_id)
    )


@returns_first_elem
def add_instance_group_subcluster(cluster_id, subcid, name, roles):
    """
    Create instance group
    """
    return g.metadb.query(
        'add_instance_group_subcluster',
        master=True,
        cid=cluster_id,
        subcid=subcid,
        name=name,
        roles=roles,
        rev=get_rev(cluster_id),
    )


@returns_first_elem
def add_shard(subcid, shard_id, name, cid):
    """
    Creates new shard in metaDB and returns it.
    """
    return g.metadb.query(
        'add_shard', master=True, subcid=subcid, shard_id=shard_id, name=name, cid=cid, rev=get_rev(cid)
    )


@returns_first_elem
def add_host(  # pylint: disable=too-many-arguments
    subcid, space_limit, flavor, geo, fqdn, subnet_id, assign_public_ip, cid, disk_type_id=None, shard_id=None
):
    """
    Creates new host in metaDB and returns it
    """
    if disk_type_id is None:
        selected_disk_type_id = config.get_disk_type_id(flavor['vtype'])
    else:
        selected_disk_type_id = disk_type_id

    return g.metadb.query(
        'add_host',
        master=True,
        subcid=subcid,
        shard_id=shard_id,
        space_limit=int(space_limit),
        flavor=flavor['id'],
        geo=geo,
        fqdn=fqdn,
        subnet_id=subnet_id,
        assign_public_ip=assign_public_ip,
        disk_type_id=selected_disk_type_id,
        cid=cid,
        rev=get_rev(cid),
    )


@returns_first_elem
def add_hadoop_job(cluster_id: str, job_id: str, name: str, job_spec: Dict) -> Dict:
    """
    Creates new Hadoop job in metaDB and returns it.
    """
    return g.metadb.query(
        'add_hadoop_job',
        cid=cluster_id,
        job_id=job_id,
        name=name,
        created_by=g.user_id,
        job_spec=job_spec,
    )


@returns_attr('operation_id')
@returns_first_elem
def get_hadoop_job_task(cluster_id: CID, job_id: str):
    """
    Get hadoop job task by cluster and job
    """
    return g.metadb.query('get_hadoop_job_task', cid=cluster_id, job_id=job_id)


def _add_pillar(  # pylint: disable=unused-argument
    value, cid, pillar_cid=None, pillar_subcid=None, pillar_shard_id=None, pillar_fqdn=None
) -> None:
    """
    Insert new pillar row into dbaas.pillar table
    """
    assert_one_exists(pillar_cid, pillar_subcid, pillar_shard_id, pillar_fqdn)
    return g.metadb.query(
        'add_pillar',
        master=True,
        pillar_cid=pillar_cid,
        pillar_subcid=pillar_subcid,
        pillar_shard_id=pillar_shard_id,
        pillar_fqdn=pillar_fqdn,
        cid=cid,
        value=value,
        rev=get_rev(cid),
    )


def add_cluster_pillar(cid: str, value: PillarT) -> None:
    """
    Add cluster pillar
    """
    _add_pillar(cid=cid, pillar_cid=cid, value=value)


def add_subcluster_pillar(cid: str, subcid: str, value: PillarT) -> None:
    """
    Add subcluster pillar
    """
    _add_pillar(cid=cid, pillar_subcid=subcid, value=value)


def add_shard_pillar(cid: str, shard_id: str, value: PillarT) -> None:
    """
    Add shard pillar
    """
    _add_pillar(cid=cid, pillar_shard_id=shard_id, value=value)


def add_host_pillar(cid: str, fqdn: str, value: PillarT) -> None:
    """
    Add host pillar
    """
    _add_pillar(cid=cid, pillar_fqdn=fqdn, value=value)


def add_target_pillar(
    target_id: str, value: str, cid: str = None, subcid: str = None, shard_id: str = None, fqdn: str = None
) -> None:
    """
    Insert new pillar row into dbaas.target_pillar table
    """
    assert_one_exists(cid, subcid, shard_id, fqdn)
    g.metadb.query(
        'add_target_pillar',
        master=True,
        target_id=target_id,
        cid=cid,
        subcid=subcid,
        shard_id=shard_id,
        fqdn=fqdn,
        value=value,
    )


@returns_attr('operation_id')
@returns_first_elem
def get_running_task(cid: CID):
    """
    Get running task on cluster
    """
    return g.metadb.query('worker_queue_get_running_task', cid=cid, master=True)


@returns_attr('operation_id')
@returns_first_elem
def get_failed_task(cid: CID):
    """
    Get failed task on cluster
    """
    return g.metadb.query('worker_queue_get_failed_task', cid=cid, master=True)


def add_operation(
    task_type: ComparableEnum,
    operation_type: ComparableEnum,
    metadata: Metadata,
    cid: str,
    folder_id: int,
    time_limit: Optional[timedelta],
    task_args: dict,
    hidden: bool,
    delay_by: Optional[timedelta],
    required_operation_id: Optional[str],
    idempotance_data: Optional[IdempotenceData],
    tracing: Optional[str],
) -> Operation:
    # pylint: disable=too-many-arguments
    """
    Creates new task in metaDB and returns it
    """
    task_id = gen_id('worker_task_id')
    if time_limit is not None:
        assert time_limit.total_seconds() > 0, 'Zero time limit'

    return make_operation(
        g.metadb.query(
            'add_operation',
            master=True,
            task_id=task_id,
            user_id=g.user_id,
            cid=cid,
            folder_id=folder_id,
            time_limit=time_limit,
            task_type=task_type,
            operation_type=operation_type,
            metadata=metadata,
            task_args=task_args,
            hidden=hidden,
            version=TASKS_VERSION,
            delay_by=delay_by,
            required_operation_id=required_operation_id,
            rev=get_rev(cid),
            idempotence_data=idempotance_data,
            tracing=tracing,
        )[0]
    )


def add_unmanaged_operation(
    task_type: ComparableEnum,
    operation_type: ComparableEnum,
    metadata: Metadata,
    cid: str,
    folder_id: int,
    time_limit: Optional[timedelta],
    task_args: dict,
    hidden: bool,
    delay_by: Optional[timedelta],
    required_operation_id: Optional[str],
    idempotance_data: Optional[IdempotenceData],
    tracing: Optional[str],
) -> Operation:
    # pylint: disable=too-many-arguments
    """
    Creates new unamanaged task in metaDB and returns it
    """
    task_id = gen_id('worker_task_id')
    if time_limit is not None:
        assert time_limit.total_seconds() > 0, 'Zero time limit'

    return make_operation(
        g.metadb.query(
            'add_unmanaged_operation',
            master=True,
            task_id=task_id,
            user_id=g.user_id,
            cid=cid,
            folder_id=folder_id,
            time_limit=time_limit,
            task_type=task_type,
            operation_type=operation_type,
            metadata=metadata,
            task_args=task_args,
            hidden=hidden,
            version=TASKS_VERSION,
            delay_by=delay_by,
            required_operation_id=required_operation_id,
            rev=get_rev(cid),
            idempotence_data=idempotance_data,
            tracing=tracing,
        )[0]
    )


def add_finished_operation(
    operation_type: ComparableEnum,
    metadata: Metadata,
    cid: CID,
    folder_id: int,
    idempotence_data: Optional[IdempotenceData],
    hidden: bool,
) -> Operation:
    """
    Creates finished task in metaDB and returns this
    """
    task_id = gen_id('worker_task_id')

    return make_operation(
        g.metadb.query(
            'add_finished_operation',
            master=True,
            task_id=task_id,
            user_id=g.user_id,
            cid=cid,
            folder_id=folder_id,
            operation_type=operation_type,
            metadata=metadata,
            hidden=hidden,
            version=TASKS_VERSION,
            rev=get_rev(cid),
            idempotence_data=idempotence_data,
        )[0]
    )


def add_finished_operation_for_current_rev(
    operation_type: ComparableEnum,
    metadata: Metadata,
    cid: CID,
    folder_id: int,
    idempotence_data: Optional[IdempotenceData],
    hidden: bool,
    rev: int,
) -> Operation:
    """
    Creates finished task in metaDB without lock cluster needed and returns this
    """
    task_id = gen_id('worker_task_id')

    return make_operation(
        g.metadb.query(
            'add_finished_operation_for_current_rev',
            master=True,
            task_id=task_id,
            user_id=g.user_id,
            cid=cid,
            folder_id=folder_id,
            operation_type=operation_type,
            metadata=metadata,
            hidden=hidden,
            version=TASKS_VERSION,
            rev=rev,
            idempotence_data=idempotence_data,
        )[0]
    )


def finish_unmanaged_task(
    task_id: str, result: bool = True, changes: dict = None, comment: str = "", task_errors: dict = None
):
    """
    Finishes the unmanaged task
    """
    changes_json = json.dumps(changes) if changes is not None else changes
    task_errors_json = json.dumps(task_errors) if task_errors is not None else task_errors
    return g.metadb.query(
        'finish_unmanaged_task',
        master=True,
        task_id=task_id,
        result=result,
        changes=changes_json,
        comment=comment,
        errors=task_errors_json,
    )


def terminate_hadoop_jobs(cid):
    """
    Terminate hadoop jobs
    """
    return g.metadb.query('terminate_hadoop_jobs', master=True, cid=cid)


def get_clusters_by_pillar(value):
    """
    Search row in dbaas.pillar which contains "value" value
    """
    return g.metadb.query('get_cluster_by_pillar_value', value=value)


@returns_first_elem
def cloud_update_used_resources(
    cloud_id=None,
    add_cpu=None,
    add_gpu=None,
    add_memory=None,
    add_space=None,
    disk_type_id=None,
    add_ssd_space=None,
    add_hdd_space=None,
    add_clusters=None,
):
    """
    Updates cloud's used resources in metaDB and returns
    new values for this
    """
    # pylint: disable=too-many-arguments
    i_add_ssd_space = 0 if add_ssd_space is None else add_ssd_space
    i_add_hdd_space = 0 if add_hdd_space is None else add_hdd_space

    if add_space is not None:
        if disk_type_id is None:
            raise RuntimeError('Using add_space without disk_type_id is deprecated')

        space_quota_map = get_space_quota_map()

        if space_quota_map[disk_type_id] == 'hdd':
            i_add_hdd_space += add_space
        elif space_quota_map[disk_type_id] == 'ssd':
            i_add_ssd_space += add_space
        else:
            raise RuntimeError(
                'Unexpected space quota type for disk_type_id {id}: {value}'.format(
                    id=disk_type_id, value=space_quota_map[disk_type_id]
                )
            )

    return g.metadb.query(
        'update_cloud_used_resources',
        master=True,
        add_cpu=add_cpu,
        add_gpu=add_gpu,
        add_memory=add_memory,
        add_ssd_space=i_add_ssd_space,
        add_hdd_space=i_add_hdd_space,
        add_clusters=add_clusters,
        x_request_id=get_x_request_id(),
        cloud_id=cloud_id if cloud_id is not None else g.cloud['cloud_id'],
    )


@returns_first_elem
def get_cluster_quota_usage(cluster_id):
    """
    Get cluster used resources
    """
    return g.metadb.query('get_cluster_quota_usage', cid=cluster_id)


def update_cluster_folder(cluster_id, folder_id) -> None:
    """
    Update cluster folder in metadb
    """
    g.metadb.query('update_cluster_folder', cid=cluster_id, folder_id=folder_id, rev=get_rev(cluster_id), fetch=False)


@returns_first_elem
def update_pillar_insert(path, value, cid=None, subcid=None, shard_id=None, fqdn=None):
    """
    Updates pillar value in json path with specified
    cid, subcid, shard_id or fqdn with jsonb_insert function.
    """
    assert_one_exists(cid, subcid, shard_id, fqdn)
    assert isinstance(path, list), 'path must be list'
    return g.metadb.query(
        'pillar_insert', master=True, cid=cid, subcid=subcid, shard_id=shard_id, fqdn=fqdn, path=path, value=value
    )


@returns_first_elem
def _update_pillar(  # pylint: disable=unused-argument
    value, cid, pillar_cid=None, pillar_subcid=None, pillar_shard_id=None, pillar_fqdn=None
) -> None:
    """
    Replaces all pillar value with specified `value`.
    """
    assert_one_exists(pillar_cid, pillar_subcid, pillar_shard_id, pillar_fqdn)
    return g.metadb.query(
        'update_pillar',
        master=True,
        pillar_cid=pillar_cid,
        pillar_subcid=pillar_subcid,
        pillar_shard_id=pillar_shard_id,
        pillar_fqdn=pillar_fqdn,
        cid=cid,
        rev=get_rev(cid),
        value=value,
    )


def update_cluster_pillar(cid: str, value: PillarT) -> None:
    """
    Update cluster pillar
    """
    _update_pillar(
        value=value,
        cid=cid,
        pillar_cid=cid,
    )


def update_subcluster_pillar(cid: str, subcid: str, value: PillarT) -> None:
    """
    Update subcluster pillar
    """
    _update_pillar(
        value=value,
        cid=cid,
        pillar_subcid=subcid,
    )


def update_shard_pillar(cid: str, shard_id: str, value: PillarT) -> None:
    """
    Update shard pillar
    """
    _update_pillar(
        value=value,
        cid=cid,
        pillar_shard_id=shard_id,
    )


def update_host_pillar(cid: str, fqdn: str, value: PillarT) -> None:
    """
    Update host pillar
    """
    _update_pillar(
        value=value,
        cid=cid,
        pillar_fqdn=fqdn,
    )


@returns_first_elem
def update_host(fqdn, cid, space_limit=None, flavor_id=None, assign_public_ip=None):
    """
    Updates host in metadb. One of argument `space_limit` or
    `flavor_id` required.
    """
    assert_one_exists(space_limit, flavor_id, assign_public_ip)
    return g.metadb.query(
        'update_host',
        master=True,
        fqdn=fqdn,
        space_limit=space_limit,
        flavor_id=flavor_id,
        cid=cid,
        rev=get_rev(cid),
        assign_public_ip=assign_public_ip,
    )


def update_hadoop_job_status(job_id: str, status: str, application_info: Optional[Dict]):
    """
    Updates hadoop job status
    """
    return g.metadb.query(
        'update_hadoop_job_status',
        master=True,
        job_id=job_id,
        status=status,
        application_info=application_info,
    )


def get_all_geo() -> List[str]:
    """
    Returns list of all geo locations
    """
    return [r['geo'] for r in g.metadb.query('get_all_geo')]


def get_clusters_count_in_folder(folder_id: str, cluster_type: str) -> int:
    """
    Get number of clusters with specified cluster_type in folder
    """
    rows = g.metadb.query('get_clusters_count_in_folder', folder_id=folder_id, cluster_type=cluster_type)
    return rows[0]['count']


def get_zk_hosts_usage():
    """
    Get zk_hosts and number of clusters used in.
    """
    return [(r['hosts'], r['c']) for r in g.metadb.query('get_zk_hosts_usage')]


@returns_attr('cid')
@returns_first_elem
def get_cluster_id_by_host_attrs(vtype_id=None, fqdn=None, shard_id=None, subcid=None):
    """
    Get cluster_id by host search attributes
    """
    return g.metadb.query('get_cid_by_host_attrs', vtype_id=vtype_id, fqdn=fqdn, shard_id=shard_id, subcid=subcid)


def cluster_is_managed(cid: str) -> bool:
    """
    Get unmanaged property of a cluster
    """
    rows = g.metadb.query('cluster_is_managed', cid=cid)
    if not rows:
        raise errors.ClusterNotFound(cid)
    return rows[0]['managed']


def add_to_search_queue(docs: List[dict]) -> None:
    """
    Add document to search_queue
    """
    g.metadb.query('add_to_search_queue', master=True, fetch=False, docs=docs)


def add_to_worker_queue_events(task_id: str, data: dict) -> None:
    """
    Add document to search_queue
    """
    g.metadb.query('add_to_worker_queue_events', master=True, fetch=False, task_id=task_id, data=data)


def get_pg_ha_hosts_by_cid(cid: CID) -> List[str]:
    """
    Get a list of Postgres HA hosts FQDNs by cluster ID
    """
    fqdns = []
    for host in g.metadb.query('get_pg_ha_hosts_by_cid', cid=cid):
        fqdns.append(host['fqdn'])
    return fqdns


@returns_attr('get_managed_config')
@returns_first_elem
def get_managed_config(fqdn, target_id=None, rev=None):
    """
    Get managed cluster config for host
    """
    return g.metadb.query('get_managed_config', fqdn=fqdn, target_id=target_id, rev=rev, master=True)


@returns_attr('get_unmanaged_config')
@returns_first_elem
def get_unmanaged_config(fqdn, target_id=None, rev=None):
    """
    Get unmanaged cluster config for host
    """
    return g.metadb.query('get_unmanaged_config', fqdn=fqdn, target_id=target_id, rev=rev, master=True)


def list_backups(
    cid: str,
    subcid: Optional[str] = None,
    shard_id: Optional[str] = None,
    backup_statuses: Optional[List[BackupStatus]] = None,
):
    """
    Returns backups for given criteria
    """
    return g.metadb.query('get_backups', cid=cid, subcid=subcid, shard_id=shard_id, backup_statuses=backup_statuses)


def get_managed_backup(bid: str):
    """
    Returns backups for given backup id
    """
    rows = g.metadb.query('get_managed_backup', bid=bid)
    if not rows:
        raise RuntimeError(f'No backup found for "{bid}" id')
    backup = rows[0]
    return ManagedBackup(
        id=backup['backup_id'],
        cid=backup['cid'],
        subcid=backup['subcid'],
        shard_id=backup['shard_id'],
        method=BackupMethodFromDB.load(backup['method']),
        initiator=BackupInitiatorFromDB.load(raw=backup['initiator']),
        status=BackupStatus(backup['status']),
        metadata=backup['metadata'],
        started_at=backup['started_at'],
        finished_at=backup['finished_at'],
    )


def schedule_backup_for_now(
    backup_id: str,
    backup_method: BackupMethodFromDB,
    cid: str,
    subcid: Optional[str] = None,
    shard_id: Optional[str] = None,
    parent_ids: List[str] = None,
):
    """
    Schedule new backup for as soon as possible
    """
    return g.metadb.query(
        'schedule_backup_for_now',
        master=True,
        fetch=False,
        backup_id=backup_id,
        cid=cid,
        subcid=subcid,
        shard_id=shard_id,
        backup_method=backup_method,
        parent_ids=parent_ids,
    )


def list_alert_groups(
    cid: str,
):
    """
    Returns alert groups for given cluster id
    """
    return g.metadb.query('get_alert_groups', cid=cid)


def get_cluster_by_ag(ag_id: str):
    """
    Get cluster by alert group id associated with it
    """

    return g.metadb.query('get_cluster_by_ag', ag_id=ag_id)


def get_managed_alert_group_by_cid(cid: str):
    """
    Get cluster by alert group id associated with it
    """

    ret = g.metadb.query('get_managed_alert_group_by_cid', cid=cid)
    if ret:
        ret = ret[0]  # max 1 managed alert group, else None

    return ret


def add_alert_group(
    cid: str,
    ag_id: str,
    managed: bool,
    monitoring_folder_id: str,
) -> None:
    """
    Add alert group for cluster
    """
    g.metadb.query(
        'add_alert_group',
        master=True,
        fetch=False,
        cid=cid,
        ag_id=ag_id,
        monitoring_folder_id=monitoring_folder_id,
        managed=managed,
        rev=get_rev(cid),
    )


def get_alerts_by_alert_group(ag_id: str):
    """
    List alerts by alert group id
    """
    return g.metadb.query('get_alerts_by_alert_group', master=True, ag_id=ag_id)


def get_alert_group(cid: str, ag_id: str) -> dict:
    """
    List alerts by alert group id
    """
    ret = g.metadb.query('get_alert_group', master=True, ag_id=ag_id, cid=cid)
    if ret:
        return ret[0]
    raise Exception(f"no alert group with id {ag_id} found")


def get_alert_template(ctype: str):
    """
    List alerts template by cluster type
    """

    return g.metadb.query('get_ctype_alert_template', master=True, ctype=ctype)


def add_alert_to_group(cid: str, ag_id: str, alert: Alert, default_thresholds=False):
    """
    Add alert to alert group
    """

    try:
        g.metadb.query(
            'add_alert_to_group',
            master=True,
            fetch=False,
            cid=cid,
            alert_group_id=ag_id,
            template_id=alert.template_id,
            disabled=alert.disabled,
            notification_channels=alert.notification_channels,
            warning_threshold=alert.warning_threshold,
            critical_threshold=alert.critical_threshold,
            default_thresholds=default_thresholds,
            rev=get_rev(cid),
        )
    except psycopg2.IntegrityError as error:
        # We need to handle violation
        # on `name` column in `dbaas.default_alert`
        if error.pgcode == pgcode.FOREIGN_KEY_VIOLATION:
            raise errors.AlertDoesNotExists(alert.template_id) from error
        if error.pgcode == pgcode.CHECK_VIOLATION:
            raise errors.AlertCreationFail(alert.template_id) from error
        raise error


def delete_alert_group(alert_group_id: str, cluster_id: str, force_managed_group_deletion: bool = False):
    """
    Delete alert group for cluster
    """

    try:
        g.metadb.query(
            'delete_alert_group',
            master=True,
            fetch=False,
            alert_group_id=alert_group_id,
            cid=cluster_id,
            force_managed_group_deletion=force_managed_group_deletion,
            rev=get_rev(cluster_id),
        )
    except psycopg2.IntegrityError as error:
        # We need to handle managed group deletion case

        if error.pgcode == pgcode.CHECK_VIOLATION:
            raise errors.ManagedAlertGroupDeletionError(alert_group_id) from error
        raise error


def delete_alert_from_group(alert_group: str, cid: str, template_id: str):
    """
    Delete alert from alert group cluster
    """

    g.metadb.query(
        'delete_alert_from_group',
        master=True,
        fetch=False,
        alert_group_id=alert_group,
        cid=cid,
        template_id=template_id,
        rev=get_rev(cid),
    )


def update_alert(cid: str, alert_group_id: str, alert: Alert, alert_old: Alert, default_thresholds=False):
    """
    Update alert group for cluster
    """

    g.metadb.query(
        'update_alert',
        master=True,
        fetch=False,
        alert_group_id=alert_group_id,
        cid=cid,
        template_id=alert.template_id,
        notification_channels=alert.notification_channels
        if alert.notification_channels
        else alert_old.notification_channels,
        disabled=alert.disabled if alert.disabled else alert_old.disabled,
        warning_threshold=alert.warning_threshold if alert.warning_threshold else alert_old.warning_threshold,
        critical_threshold=alert.critical_threshold if alert.critical_threshold else alert_old.critical_threshold,
        default_thresholds=default_thresholds,
        rev=get_rev(cid),
    )


def update_alert_group(cid, ag_id, monitoring_folder_id):
    g.metadb.query(
        'update_alert_group',
        master=True,
        fetch=False,
        alert_group_id=ag_id,
        cid=cid,
        monitoring_folder_id=monitoring_folder_id,
        rev=get_rev(cid),
    )
