# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop utils
"""

import string
import random
from typing import cast, Dict, List, Optional

import semver
from flask import g

from ...core.exceptions import DbaasClientError, ParseConfigError
from ...utils.iam_token import get_iam_client
from ...utils.dataproc_joblog.api import DataprocJobLog
from ...utils.dataproc_joblog.client import DataprocS3JobLogClient
from ...utils.metadb import get_resource_preset_by_id
from ...utils.network import get_subnet
from .constants import (
    MY_CLUSTER_TYPE,
    SUBCLUSTER_NAME_PREFIX_BY_ROLE,
    MASTER_SUBCLUSTER_TYPE,
    DATAPROC_VERSION_2,
    DATAPROC_AGENT_METRICS_VERSION_1,
    DATAPROC_AGENT_METRICS_VERSION_2,
)
from .traits import ClusterService, InstanceRoles
from .types import HadoopConfigSpec
from .pillar import HadoopPillar


def get_random_string(length: int) -> str:
    return ''.join([random.choice(string.ascii_lowercase) for i in range(length)])


def generate_subcluster_name(role: str, pillar: HadoopPillar) -> str:
    """Generates subcluster names like "master", "data1", "compute3" """

    name_prefix = SUBCLUSTER_NAME_PREFIX_BY_ROLE[role]
    if role == MASTER_SUBCLUSTER_TYPE:
        return name_prefix
    new_subcluster_index = 1
    for subcluster in pillar.subclusters:
        if subcluster.get('role') == role and subcluster['name'].startswith(name_prefix):
            new_subcluster_index += 1
    return f'{name_prefix}{new_subcluster_index}'


def guess_subnet_id(vpc, zone_id: str, folder_id: str = None, network_id: str = None) -> str:
    if network_id:
        try:
            networks = [vpc.get_network(network_id)]
        except Exception:
            raise DbaasClientError(f'Could not find network {network_id}.')
    else:
        try:
            networks = vpc.get_networks(folder_id)
        except Exception:
            raise DbaasClientError(f'Could not find networks in folder {folder_id}.')
    subnets_in_zone = set()
    for network in networks:
        subnets = vpc.get_subnets(network).get(zone_id, {})
        for subnet_id, _ in subnets.items():
            subnets_in_zone.add(subnet_id)
    if not subnets_in_zone:
        raise DbaasClientError(f'There are no subnets in zone {zone_id} in folder {folder_id}. Please create one.')
    if len(subnets_in_zone) > 1:
        raise DbaasClientError(
            f'There are more than one subnets in zone {zone_id} of folder {folder_id}. '
            'Please specify which subnet to use.'
        )
    subnet_id = subnets_in_zone.pop()
    return subnet_id


def generate_cluster_name() -> str:
    random_string = get_random_string(4)
    return f'dataproc_{random_string}'


def get_default_autoscaling_rule(cid: str, image_version: str):
    image_version = semver.VersionInfo.parse(image_version)
    if image_version >= DATAPROC_AGENT_METRICS_VERSION_2 or (
        image_version < DATAPROC_VERSION_2 and image_version >= DATAPROC_AGENT_METRICS_VERSION_1
    ):
        metric_name = 'dataproc.cluster.neededAutoscalingNodesNumber'
        target = 1.0
    else:
        metric_name = 'yarn.cluster.containersPending'
        target = 2.0
    return {
        'target': target,
        'service': 'data-proc',
        'rule_type': 'WORKLOAD',
        'metric_name': metric_name,
        'metric_type': 'GAUGE',
        'labels': {
            'resource_type': 'cluster',
            'resource_id': cid,
        },
    }


def get_role_services(cluster_services_enum: List[ClusterService], image: Dict, role: str) -> List[str]:
    """Get subcluster services from services and role"""
    cluster_services = ClusterService.to_strings(cluster_services_enum)
    available_services = image['roles_services'][role]
    services = list(set(available_services) & set(cluster_services))

    # Sorting output in order of image available services
    services = sorted(services, key=available_services.index)
    return services


def fetch_subnet(
    subnet_id: str, public_ip_used: bool, zone_id: Optional[str] = None, network_id: Optional[str] = None
) -> dict:
    """Get subnet config dictionary"""
    subnet = get_subnet(subnet_id, public_ip_used)
    if zone_id and subnet.get('zoneId') != zone_id:
        error_text = 'Subnet zone "{}" is not equal to the availability zone "{}" of the cluster'
        error_text = error_text.format(subnet.get('zoneId'), zone_id)
        raise DbaasClientError(error_text)
    subnet_network_id = subnet['networkId']
    if network_id and subnet_network_id != network_id:
        raise DbaasClientError('All subnets must be in network "{}"'.format(network_id))
    return cast(dict, subnet)


def extend_instance_labels(subclusters: list, user_labels: dict) -> dict:
    """
    Extend labels for instance. Return dict
    """
    labels = {}
    for subcluster in subclusters:
        subcluster_labels = fill_subcluster_labels(subcluster, user_labels)
        labels.update(subcluster_labels)
    return labels


def fill_subcluster_labels(subcluster: dict, user_labels: dict) -> dict:
    """
    Fill labels for subcluster. Return dict
    """
    subcluster_labels = {}
    role = InstanceRoles.get_humanize_role(subcluster['role'])
    subcluster_id = subcluster['subcid']
    subcluster_labels.update(
        {
            'cluster_id': subcluster['cid'],
            'subcluster_id': subcluster['subcid'],
            'subcluster_role': role,
            'folder_id': g.folder['folder_ext_id'],
            'cloud_id': g.cloud['cloud_ext_id'],
        }
    )
    subcluster_labels.update(user_labels)
    return {subcluster_id: subcluster_labels}


def set_pillar_labels(pillar, labels):
    """
    Extend user supplied labels with default ones, validate labels and save them to pillar
    """
    subclusters = pillar.subclusters
    subclusters.append(pillar.subcluster_main)
    extended_labels = extend_instance_labels(subclusters, labels or {})
    pillar.labels = extended_labels


def get_raw_resources_by_resource_preset(resource_preset_id: str) -> Dict:
    """
    Return raw resources (cpu_cores, core_fraction, memory) by resource_preset_id
    Masternode requires a knowledge of subcluster's resources for specify yarn scheduler properties
    """
    resource_preset_obj = get_resource_preset_by_id(MY_CLUSTER_TYPE, resource_preset_id)
    if not resource_preset_obj or len(resource_preset_obj) == 0:
        raise ParseConfigError(f'Not found resourcePreset {resource_preset_id}')
    return {
        'cores': int(resource_preset_obj['cores']),
        'core_fraction': resource_preset_obj['core_fraction'],
        'memory': resource_preset_obj['memory'],
    }


def get_subcluster_raw_resources(config_spec: dict, subcluster_name: str) -> Dict:
    """
    Return raw resources (cpu_cores, core_fraction, memory) for specified subcluster_name
    Masternode requires a knowledge of subcluster's resources for specify yarn scheduler properties
    """
    try:
        hadoop_config_spec = HadoopConfigSpec(config_spec)
        for subcluster in hadoop_config_spec.get_subclusters():
            if subcluster.get_name() != subcluster_name:
                continue
            resource_preset_id = subcluster.get_resources().resource_preset_id
            if resource_preset_id is None:
                raise DbaasClientError(
                    (f'Wrong configuration, subcluster `{subcluster_name}` ' f'doesn\'t have resource_preset_id')
                )
            return get_raw_resources_by_resource_preset(resource_preset_id)
    except ValueError as err:
        raise ParseConfigError(err)
    raise DbaasClientError(f'Not found subcluster {subcluster_name}')


def get_joblog_client(pillar: HadoopPillar) -> DataprocJobLog:
    """
    Return client for gathering dataproc job log
    """
    iam_token = get_iam_client().issue_iam_token(pillar.service_account_id)

    return DataprocS3JobLogClient(cast(str, pillar.user_s3_bucket), iam_token, **pillar.s3)


def get_seconds(seconds_string):
    if not seconds_string:
        return None
    return int(seconds_string.replace('s', ''))


def get_autoscaling_config(instance_group_config):
    autoscaling_config = instance_group_config.get('scale_policy', {}).get('auto_scale', {})
    preemptible = instance_group_config.get('instance_template', {}).get('scheduling_policy', {}).get('preemptible')
    autoscaling_config = {
        'max_hosts_count': autoscaling_config.get('max_size'),
        'warmup_duration': get_seconds(autoscaling_config.get('warmup_duration')),
        'measurement_duration': get_seconds(autoscaling_config.get('measurement_duration')),
        'stabilization_duration': get_seconds(autoscaling_config.get('stabilization_duration')),
        'preemptible': preemptible,
        'cpu_utilization_target': autoscaling_config.get('cpu_utilization_rule', {}).get('utilization_target'),
        'custom_rules': autoscaling_config.get('custom_rules'),
    }
    return autoscaling_config
