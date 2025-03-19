# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop compute quota checking utils
"""

import copy
from collections import Counter
from typing import Iterable, Union

from ...utils.types import RequestedHostResources, ExistedHostResources
from ...utils.compute_quota import check_compute_quota
from .types import HadoopSubclusterSpec
from .utils import get_raw_resources_by_resource_preset


def get_quota_counter(
    resources: Union[RequestedHostResources, ExistedHostResources], hosts_count: int = 1, on_cluster_start: bool = False
):
    """
    Converts RequestedHostResources to Counter with Compute quota names to compare requested resources with quota
    """

    quota_requested_dict: Counter[str] = Counter()
    if not on_cluster_start:
        quota_requested_dict['compute.instances.count'] = hosts_count
        quota_requested_dict['compute.disks.count'] = hosts_count
    if resources.resource_preset_id:
        raw_resources = get_raw_resources_by_resource_preset(resources.resource_preset_id)
        core_score = raw_resources['cores'] / 100 * raw_resources['core_fraction']
        quota_requested_dict['compute.instanceCores.count'] = core_score * hosts_count
        quota_requested_dict['compute.instanceMemory.size'] = raw_resources['memory'] * hosts_count
    if resources.disk_type_id and resources.disk_size and not on_cluster_start:
        if resources.disk_type_id.endswith('ssd'):
            quota_requested_dict['compute.ssdDisks.size'] = resources.disk_size * hosts_count
        else:
            quota_requested_dict['compute.hddDisks.size'] = resources.disk_size * hosts_count
    return quota_requested_dict


def check_compute_quota_on_modification(
    current_resources: ExistedHostResources,
    current_host_count: int,
    requested_resources: RequestedHostResources = None,
    requested_host_count: int = None,
):
    """
    Compares requested additional resources with available Compute quota
       and raises an exception if requested resources do not fit in available Compute quota
    """

    current_quota_counter = get_quota_counter(current_resources, current_host_count)

    filled_requested_resources = copy.deepcopy(current_resources)
    if requested_resources:
        filled_requested_resources.update(requested_resources)
    requested_host_count = requested_host_count or current_host_count

    requested_quota_counter = get_quota_counter(filled_requested_resources, requested_host_count)

    additional_quota_counter = requested_quota_counter - current_quota_counter

    check_compute_quota(additional_quota_counter)


def check_subclusters_compute_quota(subclusters: Iterable, on_cluster_start: bool = False):
    """
    Calculates sum of requested resources for specified subclusters and checks whether this sum fits to Compute quota
    """

    total_requested_resources: Counter[str] = Counter()
    for subcluster in subclusters:
        if not isinstance(subcluster, HadoopSubclusterSpec):
            subcluster = HadoopSubclusterSpec(subcluster)
        total_requested_resources += get_quota_counter(
            resources=subcluster.get_resources(),
            hosts_count=subcluster.get_hosts_count(),
            on_cluster_start=on_cluster_start,
        )

    check_compute_quota(total_requested_resources)
