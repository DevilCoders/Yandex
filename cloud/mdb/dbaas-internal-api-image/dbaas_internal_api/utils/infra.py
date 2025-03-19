"""
Describes cluster-independent features like flavor, hosts etc

The name `infra` instead of full `infrastructure` because the
length of the word just begs to make a typo in it.
"""
from typing import Iterable, Optional, cast

from . import config, metadb
from ..core.types import CID
from .types import ComparableEnum, ExistedHostResources, Host, RequestedHostResources


class Flavors:
    """Get and cache flavors"""

    def __init__(self):
        self._flavors = {}

    def get_flavor_by_name(self, name: str):
        """Get and cache flavors"""
        if name not in self._flavors:
            self._flavors[name] = dict(metadb.get_flavor_by_name(name))
        return self._flavors[name]


def get_resources(cluster_id: CID, role: ComparableEnum = None) -> RequestedHostResources:
    """
    Returns resources object for the specified cluster id and host role.

    Returns empty resources if not hosts found.
    """
    hosts = metadb.get_hosts(cluster_id, role=role)

    if not hosts:
        return RequestedHostResources()

    return cast(RequestedHostResources, get_host_resources(hosts[0]))


def get_resources_strict(cluster_id: CID, role: ComparableEnum = None) -> ExistedHostResources:
    """
    Returns resources object for specified cluster id and host role.

    Fail with RuntimeError if no hosts found
    """
    hosts = metadb.get_hosts(cluster_id, role=role)
    if not hosts:
        raise RuntimeError(f'No resources defined for cluster_id: "{cluster_id}", role: "{role}"')
    return get_host_resources(hosts[0])


def get_resources_at_rev(cluster_id: CID, rev: int, role: ComparableEnum = None) -> ExistedHostResources:
    """
    Returns resources object for specified cluster id and host role at given revision
    """
    hosts = metadb.get_hosts_at_rev(cid=cluster_id, rev=rev)
    if not hosts:
        raise RuntimeError(f'No hosts found for "{cluster_id}" at rev "{rev}"')
    if role is not None:
        hosts = [h for h in hosts if role in h['roles']]
        if not hosts:
            raise RuntimeError(f'There are no hosts with "{role}" in "{cluster_id}" at "{rev}" rev')
    return get_host_resources(hosts[0])


def get_shard_resources(shard_id: str) -> RequestedHostResources:
    """
    Returns resources object for the specified shard.
    """
    hosts = metadb.get_shard_hosts(shard_id)

    if not hosts:
        return RequestedHostResources()

    return cast(RequestedHostResources, get_host_resources(hosts[0]))


def get_shard_resources_strict(shard_id: str) -> ExistedHostResources:
    """
    Returns resources object for the specified shard.
    """
    hosts = metadb.get_shard_hosts(shard_id)

    if not hosts:
        raise RuntimeError(f'No resources defined for shard_id: "{shard_id}"')
    return get_host_resources(hosts[0])


def get_host_resources(host: Host) -> ExistedHostResources:
    """
    Returns resources object for the specified host.
    """
    return ExistedHostResources(
        resource_preset_id=host['flavor_name'], disk_size=host['space_limit'], disk_type_id=host['disk_type_id']
    )


def filter_cluster_resources(cluster, role=None) -> RequestedHostResources:
    """
    Returns some host resources object for the specified cluster.
    """
    if not role:
        role = cluster['type']
    for info in cluster['hosts_info']:
        if role in info['roles']:
            return RequestedHostResources(
                resource_preset_id=info['flavor_name'], disk_size=info['space_limit'], disk_type_id=info['disk_type_id']
            )

    return RequestedHostResources(resource_preset_id=None, disk_size=0, disk_type_id=None)


def filter_shard_resources(cluster, shard_id) -> RequestedHostResources:
    """
    Returns some host resources object for the specified shard.
    """
    for info in cluster['hosts_info']:
        if shard_id == info['shard_id']:
            return RequestedHostResources(
                resource_preset_id=info['flavor_name'], disk_size=info['space_limit'], disk_type_id=info['disk_type_id']
            )

    return RequestedHostResources()


def oldest_shard_id(cluster, role):
    """
    Returns oldest shard id for the specified role in cluster.
    """
    for shard_info in cluster['oldest_shards']:
        if role in shard_info['roles']:
            return shard_info['shard_id']

    cluster_id = cluster['cid']
    raise RuntimeError(f'No info for oldest_shard_id function, cluster id: "{cluster_id}", role: "{role}"')


def suggest_similar_flavor_from(flavors_iter: Iterable[dict], for_flavor: dict) -> Optional[str]:
    """
    Suggest similar flavor for for_flovar from flavors_iter
    """

    def closet_key(flavor):
        return (
            abs(for_flavor['cpu_guarantee'] - flavor['cpu_guarantee']),
            flavor['cpu_guarantee'],
            flavor['memory_guarantee'],
        )

    flavors = list(sorted(flavors_iter, key=closet_key))
    if not flavors:
        return None

    if for_flavor in flavors:
        raise RuntimeError(f'Find {for_flavor} in flavors: {flavors}')

    return flavors[0]['name']


def _find_flavor_by_name(flavors_iter: Iterable[dict], name: str) -> dict:
    for flavor in flavors_iter:
        if flavor['name'] == name:
            return flavor
    raise RuntimeError(f'Flavor {name} not found in {flavors_iter}')


def suggest_similar_flavor(for_flavor_name: str, cluster_type: str, role: str) -> str:
    """
    Suggest similar flavor for decommissioning for_flavor_name
    """

    flavors = metadb.get_flavors(limit=None)
    decommissioned_flavor = _find_flavor_by_name(flavors, for_flavor_name)

    all_valid_flavors_names = {
        f['id']
        for f in metadb.get_valid_resources(
            cluster_type=cluster_type, role=role, geo=None, resource_preset_id=None, disk_type_ext_id=None
        )
    }

    valid_flavors = [
        f
        for f in flavors
        if f['name'] in all_valid_flavors_names and f['name'] not in config.get_decommissioning_flavors()
    ]
    valid_flavors_with_same_type = (f for f in valid_flavors if f['type'] == decommissioned_flavor['type'])

    suggested_name = suggest_similar_flavor_from(valid_flavors_with_same_type, decommissioned_flavor)
    if suggested_name is not None:
        return suggested_name

    suggested_name = suggest_similar_flavor_from(valid_flavors, decommissioned_flavor)
    if suggested_name is not None:
        return suggested_name

    raise RuntimeError(f'Unable to suggest flavor to {for_flavor_name}')


def get_flavor_by_cluster_id(cluster_id, role: ComparableEnum = None):
    resources = get_resources(cluster_id, role)
    return metadb.get_flavor_by_name(resources.resource_preset_id)
