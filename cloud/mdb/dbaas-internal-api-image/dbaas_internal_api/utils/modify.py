"""
Various helper functions related to ClusterModify operations
"""
from collections import defaultdict
from datetime import timedelta
from typing import Dict, Optional, Sequence, Tuple, Union  # noqa

from flask import g

from dbaas_common.worker import get_expected_resize_hours

from ..core.exceptions import DbaasClientError
from ..core.types import CID
from . import metadb, validation
from .cluster.update import hosts_change_flavor, is_downscale
from .infra import get_host_resources
from .types import ComparableEnum, Host, OperationParameters, RequestedHostResources


def update_cluster_resources(
    cid: CID,
    cluster_type: str,
    resources: RequestedHostResources,
    role: Union[ComparableEnum, str] = None,
    validate_func=None,
) -> Tuple[OperationParameters, bool]:
    """
    Update resources of cluster hosts.
    """
    changes = False
    reverse_order = None
    instance_type_changed = False
    time_limit = timedelta(hours=0)

    for hosts in _get_host_grouped_by_shard(cid, role):
        i_changes, i_reverse_order, i_instance_type_changed, i_time_limit = _update_resources(
            cid, hosts, cluster_type, resources, validate_func=validate_func
        )

        if reverse_order is None:
            reverse_order = i_reverse_order
        else:
            if i_reverse_order != reverse_order:
                raise DbaasClientError('Upscale and downscale of shards cannot be mixed in one operation.')

        if i_changes:
            time_limit += i_time_limit

        changes = changes or i_changes
        instance_type_changed = instance_type_changed or i_instance_type_changed

    return _to_operation_parameters(changes, reverse_order, time_limit), instance_type_changed


def update_shard_resources(
    cid, shard_id: str, cluster_type: str, resources: RequestedHostResources
) -> Tuple[OperationParameters, bool]:
    """
    Update resources of shard hosts.
    """
    hosts = metadb.get_shard_hosts(shard_id)

    changes, reverse_order, instance_type_changed, time_limit = _update_resources(cid, hosts, cluster_type, resources)

    return _to_operation_parameters(changes, reverse_order, time_limit), instance_type_changed


def update_field(field: str, src: object, dst: object) -> bool:
    """
    Check necessary to update and update field. Return True if updated.
    """
    new_val = getattr(src, field)
    if not new_val:
        return False

    if new_val == getattr(dst, field):
        return False

    getattr(dst, 'update_' + field)(new_val)
    return True


def _get_host_grouped_by_shard(cid: CID, role: Optional[Union[ComparableEnum, str]]) -> Sequence[Sequence[Host]]:
    result = defaultdict(list)  # type: Dict[str, list]
    for host in metadb.get_hosts(cid, role=role):
        result[host['shard_id']].append(host)

    return list(result.values())


def _update_resources(
    cid: CID, hosts: Sequence[Host], cluster_type: str, resources: RequestedHostResources, validate_func=None
) -> Tuple[bool, bool, bool, timedelta]:
    """
    - determine if changes are possible
    - determine if changes are required
    - update metadb.
    """
    # pylint: disable=too-many-locals, too-many-branches
    changes = False
    reverse_order = False
    instance_type_changed = False

    current_resources = get_host_resources(hosts[0])

    time_limit = get_expected_resize_hours(current_resources.disk_size, len(hosts))

    validation.check_change_compute_resource(hosts, current_resources, resources)

    target_resource_preset_id = (
        resources.resource_preset_id
        if resources.resource_preset_id is not None
        else current_resources.resource_preset_id
    )

    target_disk_type_id = (
        resources.disk_type_id if resources.disk_type_id is not None else current_resources.disk_type_id
    )

    target_disk_size = resources.disk_size if resources.disk_size is not None else current_resources.disk_size

    role = hosts[0]['roles'][0]
    if target_resource_preset_id != current_resources.resource_preset_id:
        validation.validate_hosts_count(
            cluster_type, role, target_resource_preset_id, target_disk_type_id, len(hosts), validate_func=validate_func
        )
        target_flavor = validation.get_flavor_by_name(target_resource_preset_id)
        validation.validate_resource_preset(target_flavor)

    for host in hosts:
        validation.validate_host_resources_combination(
            cluster_type=cluster_type,
            role=role,
            geo=host['geo'],
            resource_preset_id=target_resource_preset_id,
            disk_type_id=target_disk_type_id,
            disk_size=target_disk_size,
        )

    # Check if instance type has changed. All this `if not None` fuss is
    # because all Resource args are optional.
    if (
        resources.resource_preset_id is not None
        and current_resources.resource_preset_id != resources.resource_preset_id
    ):
        # The resources that user wants to apply, as returned by Metadb
        new_instance_type_obj = metadb.get_flavor_by_name(resources.resource_preset_id)

        diff, id_changed = hosts_change_flavor(cid, hosts, new_instance_type_obj)
        # Check for any actual change
        if any(value != 0 for value in diff.values()) or id_changed:
            changes = True
            instance_type_changed = True
        # Check for downscale: reverse order is required.
        #
        # This is needed for dbaas worker. If we decrease container resources
        # we need to apply states in reverse order (container, host).
        # For example, if we decrease flavor and memory parameters
        # of database, we need to apply this state first on container,
        # and only after on the host to prevent OOM situation.
        if is_downscale(diff):
            reverse_order = True
        else:
            for host in hosts:
                validation.validate_geo_availability(host['geo'])

    if resources.disk_size is not None and current_resources.disk_size != resources.disk_size:
        # Check that cloud has enough `volume_size` quota
        ssd_diff, hdd_diff = validation.volume_size_diffs(hosts, resources.disk_size)
        validation.check_quota_volume_size_diffs(ssd_diff, hdd_diff)
        add_space = 0
        for host in hosts:
            metadb.update_host(host['fqdn'], cid=cid, space_limit=resources.disk_size)
            add_space += resources.disk_size - host['space_limit']
        if add_space != 0:
            changes = True
            metadb.cloud_update_used_resources(add_ssd_space=ssd_diff, add_hdd_space=hdd_diff)
            g.cloud['ssd_space_used'] += ssd_diff
            g.cloud['hdd_space_used'] += hdd_diff
            if add_space < 0:
                reverse_order = True

    # TODO: handle disk_type changes

    return changes, reverse_order, instance_type_changed, time_limit


def _to_operation_parameters(
    changes: bool, reverse_order: Optional[bool], time_limit: timedelta
) -> OperationParameters:
    task_args = {'reverse_order': True} if reverse_order else {}
    return OperationParameters(changes=changes, task_args=task_args, time_limit=time_limit)
