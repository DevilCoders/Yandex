# -*- coding: utf-8 -*-
"""
DBaaS Internal API cluster for cluster updates
"""

import itertools
import operator
from typing import Optional, Sequence

from flask import g

from ...core.exceptions import DbaasClientError
from .. import metadb, validation
from ..types import Host, LabelsDict, ClusterInfo, MaintenanceWindowDict
from ..validation import validate_resource_preset


def resources_diff(hosts: Sequence[Host], new_flavor: dict) -> tuple[dict, bool]:
    """
    Returns dict of resources diff.
    """

    # Obtain all host flavors from database. Store it as dictionary with
    # `flavor_id` as key, tuple (hosts_count, flavor_params) as value.
    flavors = {
        flavor_id: (len(list(flavors)), metadb.get_flavor_by_id(flavor_id))
        for flavor_id, flavors in itertools.groupby(hosts, operator.itemgetter('flavor'))
    }

    for flavor_grp in flavors.values():
        _, flavor = flavor_grp
        if new_flavor != flavor:
            validate_resource_preset(new_flavor)

    add_resources = {
        'cpu_guarantee': 0,
        'gpu_limit': 0,
        'memory_guarantee': 0,
    }
    id_changed = False
    # Calculate resources diff stored in `add_resources`
    for count, flavor in flavors.values():
        for resource in add_resources:
            add_resources[resource] += count * (new_flavor[resource] - flavor[resource])
        if flavor['id'] != new_flavor['id']:
            id_changed = True

    return add_resources, id_changed


def is_downscale(diff: dict, resources: list = None) -> bool:
    """
    Check if any of the resources is downgraded.
    """
    if resources is None:
        resources = ['cpu_guarantee', 'cpu_limit', 'memory_guarantee', 'memory_limit']
    for key in resources:
        if key in diff and diff[key] < -0.1:
            return True
    return False


def is_upscale(diff: dict) -> bool:
    """
    Check if any of the resources is upgraded.
    """
    for key in ['cpu_guarantee', 'cpu_limit', 'memory_guarantee', 'memory_limit']:
        if key in diff and diff[key] > 0.1:
            return True
    return False


def hosts_change_flavor(
    cid: str, hosts: Sequence[Host], new_flavor: dict, is_update_resources=True
) -> tuple[dict, bool]:
    """
    Change flavor to `new_flavor` for all `hosts`.
    Returns dict of resources diff.
    """
    add_resources, id_changed = resources_diff(hosts, new_flavor)

    validation.check_quota_change_resources(add_resources)
    for host in hosts:
        metadb.update_host(host['fqdn'], flavor_id=new_flavor['id'], cid=cid)

    # pass this step for Hadoop case
    if is_update_resources:
        g.cloud['cpu_used'] += add_resources['cpu_guarantee']
        g.cloud['gpu_used'] += add_resources['gpu_limit']
        g.cloud['memory_used'] += add_resources['memory_guarantee']

        metadb.cloud_update_used_resources(
            add_cpu=add_resources['cpu_guarantee'],
            add_gpu=add_resources['gpu_limit'],
            add_memory=add_resources['memory_guarantee'],
        )
    return add_resources, id_changed


def update_cluster_name(cluster: dict, name: Optional[str]) -> bool:
    """
    Update cluster name if they are changed
    Return true if update happens
    """
    # if name not defined,
    # treat it as no changes
    if name is None:
        return False
    if 'name' not in cluster:
        raise RuntimeError('update_cluster_name require cluster dict with \'name\', got %r' % cluster)
    old_name = cluster['name']
    if old_name != name:
        metadb.update_cluster_name(cid=cluster['cid'], name=name)
        return True
    return False


def update_cluster_labels(cluster: dict, labels: Optional[LabelsDict]) -> bool:
    """
    Update cluster labels if they are changed
    Return true if update happens
    """
    # if labels not defined,
    # treat it as no changes
    if labels is None:
        return False
    if 'labels' not in cluster:
        raise RuntimeError('update_cluster_labels require cluster dict with labels, got %r' % cluster)
    old_labels_set = set(cluster['labels'].items())
    new_labels_set = set(labels.items())
    if old_labels_set != new_labels_set:
        metadb.set_labels_on_cluster(cid=cluster['cid'], labels=labels)
        return True
    return False


def update_cluster_description(cluster: dict, description: Optional[str]) -> bool:
    """
    Update cluster description if it changed
    Return true if update happens
    """
    if description is None:
        return False
    if 'description' not in cluster:
        raise RuntimeError('update_cluster_description require ' 'cluster dict with description, got %r' % cluster)
    if cluster['description'] != description:
        metadb.update_cluster_description(cid=cluster['cid'], description=description)
        return True
    return False


def update_cluster_maintenance_window(cluster: dict, mw: Optional[MaintenanceWindowDict]) -> bool:
    """
    Update cluster maintenance window if it changed

    return True if something changed
    """
    if mw is None:
        return False

    current_cluster = ClusterInfo.make(cluster)

    if current_cluster.maintenance_window is None:
        raise RuntimeError(
            'update_cluster_maintenance_window require cluster dict ' 'with maintenance_window, got %r' % cluster
        )

    if current_cluster.maintenance_window != mw:
        if 'anytime' in mw and 'weekly_maintenance_window' in mw:
            raise DbaasClientError('Update cluster maintenance window requires only one window type')
        if 'anytime' in mw:
            metadb.set_maintenance_window_settings(cid=cluster['cid'], day=None, hour=None)
        elif 'weekly_maintenance_window' in mw:
            mw_day = mw['weekly_maintenance_window'].get('day')
            mw_hour = mw['weekly_maintenance_window'].get('hour')
            metadb.set_maintenance_window_settings(cid=cluster['cid'], day=mw_day, hour=mw_hour)
        else:
            raise DbaasClientError('Update cluster maintenance window requires weekly or anytime window defined')
        return True
    return False


def update_cluster_deletion_protection(cluster: dict, deletion_protection: Optional[bool]) -> bool:
    """
    Update cluster deletion_protection if it changed
    Return true if update happens
    """
    if deletion_protection is None:
        return False
    if 'deletion_protection' not in cluster:
        raise RuntimeError(
            'update_cluster_deletion_protection require cluster dict with "deletion_protection", got %r' % cluster
        )
    if cluster['deletion_protection'] != deletion_protection:
        metadb.update_cluster_deletion_protection(cid=cluster['cid'], deletion_protection=deletion_protection)
        return True
    return False


def update_cluster_metadb_parameters(
    cluster: dict,
    description: Optional[str],
    maintenance_window: Optional[MaintenanceWindowDict] = None,
    deletion_protection: Optional[bool] = None,
) -> bool:
    """
    Update cluster description, labels and maintenance window combo

    return True if something changed
    """
    return any(
        [
            update_cluster_description(cluster, description),
            update_cluster_maintenance_window(cluster, maintenance_window),
            update_cluster_deletion_protection(cluster, deletion_protection),
        ]
    )
