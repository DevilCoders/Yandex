# -*- coding: utf-8 -*-
"""
DBaaS Internal API utils for host-related operations
Contains some wrappers on metadb operations
"""

from datetime import timedelta
from typing import Any, Callable, Dict, List, Optional

from . import metadb, register, validation
from .cluster.create import cluster_get_fqdn
from .helpers import merge_dict, remove_none_dict
from .metadata import AddClusterHostsMetadata, DeleteClusterHostsMetadata, ModifyClusterHostsMetadata
from .network import get_host_subnet, get_network, get_subnets
from .operation_creator import create_operation
from .types import ComparableEnum, Host
from ..core.types import CID


def create_host(
    cluster_type,
    cluster,
    geo,
    subnet_id,
    assign_public_ip,
    fqdn=None,
    role=None,
    shard_id=None,
    pillar=None,
    args=None,
    task_type=None,
    time_limit: timedelta = None,
):
    # pylint: disable=too-many-arguments,too-many-locals
    """
    Creates host for cluster_type.
    Returns task for worker
    """
    validation.validate_geo_availability(geo)
    tasks_obj = register.get_cluster_traits(cluster_type).tasks
    operations_obj = register.get_cluster_traits(cluster_type).operations

    if shard_id:
        hosts = metadb.get_shard_hosts(shard_id)
    else:
        hosts = metadb.get_hosts(cluster['cid'], role=role)

    flavor = metadb.get_flavor_by_id(hosts[0]['flavor'])
    validation.check_quota(flavor, 1, hosts[0]['space_limit'], hosts[0]['disk_type_id'])
    new_host = hosts[0].copy()
    new_host['fqdn'] = fqdn if fqdn else cluster_get_fqdn(geo, flavor)
    new_host['cid'] = cluster['cid']
    new_host['geo'] = geo
    new_host['assign_public_ip'] = assign_public_ip
    new_host['disk_type_id'] = hosts[0]['disk_type_id']
    # Copy value due to arg name difference
    new_host['flavor'] = flavor
    network = get_network(cluster['network_id'])
    subnets = get_subnets(network)
    new_host['subnet_id'] = get_host_subnet(subnets, geo, flavor['vtype'], assign_public_ip, subnet_id)
    fields = [
        'subcid',
        'space_limit',
        'flavor',
        'geo',
        'fqdn',
        'shard_id',
        'disk_type_id',
        'subnet_id',
        'assign_public_ip',
        'cid',
    ]
    add_kwargs = {key: val for key, val in new_host.items() if key in fields}

    metadb.add_host(**add_kwargs)
    if pillar:
        metadb.add_host_pillar(cluster['cid'], new_host['fqdn'], pillar)
    metadb.cloud_update_used_resources(
        add_cpu=flavor['cpu_guarantee'],
        add_memory=flavor['memory_guarantee'],
        add_space=new_host['space_limit'],
        disk_type_id=new_host['disk_type_id'],
    )

    if args:
        task_args = args
    else:
        task_args = {}

    task_args['host'] = new_host['fqdn']
    task_args['subcid'] = new_host['subcid']
    task_args['shard_id'] = shard_id

    if not task_type:
        task_type = tasks_obj.host_create
    if not time_limit:
        time_limit = timedelta(hours=24)
    return create_operation(
        task_type=task_type,
        operation_type=operations_obj.host_create,
        metadata=AddClusterHostsMetadata(host_names=[new_host['fqdn']]),
        cid=cluster['cid'],
        time_limit=time_limit,
        task_args=task_args,
    )


def get_hosts(cid: CID, role: ComparableEnum = None) -> Dict[str, Host]:
    """
    Returns cluster hosts.

    If a role is specified, only hosts with this role will be returned.
    """
    ret = {}
    for host in metadb.get_hosts(cid, role=role):
        ret[host['fqdn']] = host

    return ret


def find_host(hosts: List[Host], fqdn: str) -> Optional[Host]:
    """
    Find host in list by fqdn.
    """
    return next((host for host in hosts if host['fqdn'] == fqdn), None)


def get_host_objects(
    cluster: Dict, extra_formatter: Callable[[Dict, Dict], Dict] = None, subcid: Optional[str] = None
) -> List[Dict[str, Any]]:
    """
    Return list of database objects conforming to HostSchema.
    """
    return [
        {
            'name': host['fqdn'],
            'cluster_id': cluster['cid'],
            'shard_id': host['shard_id'],
            'shard_name': host['shard_name'],
            'zone_id': host['geo'],
            'subnet_id': host['subnet_id'],
            'assign_public_ip': host['assign_public_ip'],
            'vtype_id': host['vtype_id'],
            'resources': {
                'resource_preset_id': host['flavor_name'],
                'disk_type_id': host['disk_type_id'],
                'disk_size': host['space_limit'],
            },
            'type': get_host_role(host),
            **(extra_formatter(host, cluster) if extra_formatter else {}),
        }
        for host in metadb.get_hosts(cluster['cid'], subcid=subcid)
    ]


def get_host_role(host):
    """
    Returns host role enumeration value.
    """
    for cluster_traits in register.get_traits().values():
        for role in cluster_traits.roles:
            if role.value in host['roles']:
                return role


def delete_host(cluster_type, cluster, fqdn, args=None, task_type=None):
    # pylint: disable=too-many-arguments
    """
    Removes host from cluster.
    Returns task for worker
    """
    tasks_obj = register.get_cluster_traits(cluster_type).tasks
    operations_obj = register.get_cluster_traits(cluster_type).operations
    host = metadb.delete_host(fqdn, cid=cluster['cid'])
    flavor = metadb.get_flavor_by_id(host['flavor'])
    metadb.cloud_update_used_resources(
        add_cpu=-flavor['cpu_guarantee'],
        add_memory=-flavor['memory_guarantee'],
        add_space=-host['space_limit'],
        disk_type_id=host['disk_type_id'],
    )

    if args:
        task_args = args
    else:
        task_args = {}

    task_args['host'] = {
        'fqdn': fqdn,
        'subcid': host['subcid'],
        'shard_id': host['shard_id'],
        'vtype': host['vtype'],
        'vtype_id': host['vtype_id'],
    }

    if not task_type:
        task_type = tasks_obj.host_delete
    return create_operation(
        task_type=task_type,
        operation_type=operations_obj.host_delete,
        metadata=DeleteClusterHostsMetadata(host_names=[fqdn]),
        cid=cluster['cid'],
        task_args=task_args,
        time_limit=timedelta(hours=10),
    )


def modify_host(cluster_type, cluster, fqdn, args=None):
    """
    Modify host in cluster.
    Returns task for worker
    """
    tasks_obj = register.get_cluster_traits(cluster_type).tasks
    operations_obj = register.get_cluster_traits(cluster_type).operations

    host_pillar = metadb.get_fqdn_pillar(fqdn=fqdn)
    if host_pillar is None:
        host_pillar = {}
        metadb.add_host_pillar(cluster['cid'], fqdn, host_pillar)

    if args is None:
        args = {}
    if args:
        task_args = args
    else:
        task_args = {}
    if args.get('pillar'):
        merge_dict(host_pillar, args['pillar'])
        pillar = remove_none_dict(host_pillar)
        metadb.update_host_pillar(cluster['cid'], fqdn, pillar)
    task_args['host'] = {'fqdn': fqdn}
    return create_operation(
        task_type=tasks_obj.host_modify,
        operation_type=operations_obj.host_modify,
        metadata=ModifyClusterHostsMetadata(host_names=[fqdn]),
        cid=cluster['cid'],
        task_args=task_args,
    )


def filter_host_map(hosts: dict, role: ComparableEnum, shard_id=None) -> Optional[dict]:
    """
    Returns dict of hosts with requested role & shard_id
    """

    def _match(opts):
        if role and role.value not in opts['roles']:
            return False
        if shard_id and shard_id != opts['shard_id']:
            return False
        return True

    return {host: opts for host, opts in hosts.items() if _match(opts)}


def collect_zones(hosts: List[dict]) -> List[str]:
    result = []
    for host in hosts:
        if host['zone_id']:
            result.append(host['zone_id'])
    return result
