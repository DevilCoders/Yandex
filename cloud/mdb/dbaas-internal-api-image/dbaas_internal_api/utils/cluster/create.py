"""
DBaaS Internal API cluster create helpers
"""

from typing import List, Optional, Sequence, Tuple

from .. import config, metadb, validation
from ...core.base_pillar import BasePillar
from ...core.id_generators import gen_hostname, gen_id
from ...core.types import CID, Operation
from ..cluster_secrets import generate_cluster_keys
from ..metadata import Metadata
from ..network import get_host_subnet, get_network, get_subnets, validate_security_groups
from ..operation_creator import create_operation
from ..types import ClusterDescription, ComparableEnum, Host
from ..validation import validate_resource_preset


def cluster_get_fqdn(geo: str, flavor: dict, fqdn_mark: str = None) -> str:
    """
    Generate fqdn from dc prefix and flavor vtype
    """
    prefix = '{prefix}-'.format(prefix=config.get_prefix(geo))
    if fqdn_mark:
        prefix = '{prefix}{mark}-'.format(prefix=prefix, mark=fqdn_mark)
    suffix = '.{domain}'.format(domain=config.get_domain(flavor['vtype']))
    return gen_hostname(prefix, suffix)


def create_cluster(
    cluster_type: str,
    network_id: str,
    description: ClusterDescription,
    update_used_resources=True,
    maintenance_window: dict = None,
    monitoring_cloud_id: str = None,
    security_group_ids: List[str] = None,
    host_group_ids: List[str] = None,
    deletion_protection: bool = False,
) -> Tuple[dict, dict, bytes]:
    """
    Create and return cluster.
    """
    cid = gen_id('cid')
    private_key, public_key = generate_cluster_keys()
    network = get_network(network_id)
    subnets = get_subnets(network)
    if security_group_ids:
        validate_security_groups(set(security_group_ids), network.network_id)

    cluster = metadb.add_cluster(
        cid=cid,
        name=description.name,
        cluster_type=cluster_type,
        env=description.environment,
        network_id=network.network_id,
        public_secret=public_key,
        description=description.description,
        host_group_ids=host_group_ids,
        deletion_protection=deletion_protection,
        monitoring_cloud_id=monitoring_cloud_id,
    )

    if update_used_resources:
        metadb.cloud_update_used_resources(add_clusters=1)
    if description.labels:
        metadb.set_labels_on_cluster(cid=cluster['cid'], labels=description.labels)

    if maintenance_window is not None and 'weekly_maintenance_window' in maintenance_window:
        weekly_maintenance_window = maintenance_window['weekly_maintenance_window']
        metadb.set_maintenance_window_settings(
            cid=cluster['cid'], day=weekly_maintenance_window['day'], hour=weekly_maintenance_window['hour']
        )

    return cluster, subnets, private_key


def create_subcluster(
    cluster_id: CID,
    name: str,
    roles: List,
    subnets: dict = None,
    flavor: dict = None,
    volume_size: int = None,
    host_specs: Sequence[Host] = None,
    disk_type_id: str = None,
    pillar: BasePillar = None,
) -> Tuple[dict, List[Host]]:
    """
    Create subcluster with hosts and return the tuple "(subcluster, hosts)".
    """
    # pylint: disable=too-many-arguments
    subcid = gen_id('subcid')
    subcluster = metadb.add_subcluster(cluster_id, subcid, name, roles)

    if pillar:
        metadb.add_subcluster_pillar(cluster_id, subcid, pillar)

    if not host_specs:
        return subcluster, []

    assert subnets is not None, 'subnets is missing'
    assert flavor, 'flavor is missing'
    assert volume_size, 'volume_size is missing'

    return subcluster, _create_hosts(
        cid=cluster_id,
        subcid=subcid,
        shard_id=None,
        flavor=flavor,
        volume_size=volume_size,
        host_specs=host_specs,
        subnets=subnets,
        disk_type_id=disk_type_id,
        fqdn_mark=None,
    )


def create_subcluster_unmanaged(
    cluster_id: CID,
    name: str,
    roles: List,
    subnet_id: str = None,
    flavor: dict = None,
    volume_size: int = None,
    host_specs: Sequence[Host] = None,
    disk_type_id: str = None,
    pillar: BasePillar = None,
    fqdn_mark: str = None,
) -> Tuple[dict, List[Host]]:
    """
    Create unmanaged subcluster with hosts and return the tuple "(subcluster, hosts)".
    """
    # pylint: disable=too-many-arguments
    subcid = gen_id('subcid')
    subcluster = metadb.add_subcluster(cluster_id, subcid, name, roles)

    if pillar:
        metadb.add_subcluster_pillar(cluster_id, subcid, pillar)

    if not host_specs:
        return subcluster, []

    assert subnet_id is not None, 'subnet_id is missing'
    assert flavor, 'flavor is missing'
    assert volume_size, 'volume_size is missing'
    return subcluster, create_hosts_unmanaged(
        cid=cluster_id,
        subcid=subcid,
        shard_id=None,
        flavor=flavor,
        volume_size=volume_size,
        host_specs=host_specs,
        subnet_id=subnet_id,
        disk_type_id=disk_type_id,
        fqdn_mark=fqdn_mark,
    )


def create_shard(  # pylint: disable=too-many-arguments
    cid: CID,
    subcid: str,
    name: Optional[str],
    flavor: dict,
    subnets: dict,
    volume_size: int,
    host_specs: Sequence[Host],
    disk_type_id: str = None,
    pillar: BasePillar = None,
) -> Tuple[dict, List[Host]]:
    """
    Create shard with hosts and return the tuple "(shard, hosts)".
    """

    shard_id = gen_id('shard_id')
    if name is None:
        name = shard_id
    shard = metadb.add_shard(subcid=subcid, shard_id=shard_id, name=name, cid=cid)

    if pillar:
        metadb.add_shard_pillar(cid, shard_id, pillar)

    return shard, _create_hosts(
        cid=cid,
        subcid=subcid,
        shard_id=shard_id,
        flavor=flavor,
        subnets=subnets,
        volume_size=volume_size,
        host_specs=host_specs,
        disk_type_id=disk_type_id,
        fqdn_mark=None,
    )


def create_shard_with_task(  # pylint: disable=too-many-arguments,too-many-locals
    cluster: dict,
    name: Optional[str],
    task_type: ComparableEnum,
    operation_type: ComparableEnum,
    metadata: Metadata,
    subcid: str,
    flavor: dict,
    volume_size: int,
    host_specs: Sequence[Host],
    disk_type_id: str = None,
    pillar: BasePillar = None,
) -> Operation:
    """
    Create shard and corresponding task for worker in metadb.
    """

    validation.check_quota(flavor, len(host_specs), volume_size, disk_type_id)

    network = get_network(cluster['network_id'])
    subnets = get_subnets(network)

    shard, _ = create_shard(
        cid=cluster['cid'],
        name=name,
        subcid=subcid,
        flavor=flavor,
        subnets=subnets,
        volume_size=volume_size,
        host_specs=host_specs,
        disk_type_id=disk_type_id,
        pillar=pillar,
    )

    return create_operation(
        task_type=task_type,
        operation_type=operation_type,
        metadata=metadata,
        cid=cluster['cid'],
        task_args={
            'shard_id': shard['shard_id'],
        },
    )


def _create_hosts(  # pylint: disable=too-many-arguments
    cid: str,
    subcid: str,
    shard_id: Optional[str],
    flavor: dict,
    volume_size: int,
    host_specs: Sequence[Host],
    subnets: dict,
    disk_type_id: Optional[str],
    fqdn_mark: Optional[str],
) -> List[Host]:

    validate_resource_preset(flavor)

    hosts = []
    for host in host_specs:
        geo = host['zone_id']
        subnet = get_host_subnet(subnets, geo, flavor['vtype'], host['assign_public_ip'], host.get('subnet_id'))
        hosts.append(
            metadb.add_host(
                subcid,
                shard_id=shard_id,
                space_limit=volume_size,
                flavor=flavor,
                geo=geo,
                fqdn=cluster_get_fqdn(geo, flavor, fqdn_mark),
                subnet_id=subnet,
                assign_public_ip=host['assign_public_ip'],
                disk_type_id=disk_type_id,
                cid=cid,
            )
        )

    host_count = len(host_specs)
    metadb.cloud_update_used_resources(
        add_cpu=host_count * flavor['cpu_guarantee'],
        add_gpu=host_count * flavor['gpu_limit'],
        add_memory=host_count * flavor['memory_guarantee'],
        add_space=host_count * volume_size,
        disk_type_id=disk_type_id,
    )

    return hosts


def create_hosts_unmanaged(
    cid: str,
    subcid: str,
    shard_id: Optional[str],
    flavor: dict,
    volume_size: int,
    host_specs: Sequence[Host],
    subnet_id: str,
    disk_type_id: Optional[str],
    fqdn_mark: Optional[str],
) -> List[Host]:
    """
    Add compute hosts to specified subcluster
    """

    # pylint: disable=too-many-arguments

    validate_resource_preset(flavor)

    hosts = []
    for host in host_specs:
        geo = host['zone_id']
        hosts.append(
            metadb.add_host(
                cid=cid,
                subcid=subcid,
                shard_id=shard_id,
                space_limit=volume_size,
                flavor=flavor,
                geo=geo,
                fqdn=cluster_get_fqdn(geo, flavor, fqdn_mark),
                subnet_id=subnet_id,
                assign_public_ip=host['assign_public_ip'],
                disk_type_id=disk_type_id,
            )
        )

    # We creating host in user compute so we not counting quotas
    return hosts
