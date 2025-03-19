# -*- coding: utf-8 -*-
"""
Utilities specific for Hadoop clusters.
"""
import json
from collections import defaultdict
from copy import deepcopy
from types import SimpleNamespace
from typing import Any, Optional

import yaml

from cloud.mdb.dbaas_worker.internal.exceptions import UserExposedException
from cloud.mdb.dbaas_worker.internal.providers.compute import get_core_fraction
from cloud.mdb.dbaas_worker.internal.providers.internal_api import InternalApi
from cloud.mdb.dbaas_worker.internal.providers.resource_manager import ResourceManagerApi
from cloud.mdb.dbaas_worker.internal.providers.user_compute import UserComputeApi
from cloud.mdb.dbaas_worker.internal.types import GenericHost

MASTER_ROLE_TYPE = 'hadoop_cluster.masternode'
DATA_ROLE_TYPE = 'hadoop_cluster.datanode'
COMPUTE_ROLE_TYPE = 'hadoop_cluster.computenode'


def _finish_host_deletion(user_compute_api, dns_api, operation_ids_to_hosts):
    completed_operation_id = user_compute_api.wait_for_any_finished_operation(operation_ids_to_hosts, timeout=1800.0)
    host = operation_ids_to_hosts[completed_operation_id]
    user_compute_api.instance_wait(host, UserComputeApi.INSTANCE_DELETED_STATE)
    dns_api.set_records(host, [])
    del operation_ids_to_hosts[completed_operation_id]


def delete_unmanaged_host_group(user_compute_api, dns_api, host_group, running_ops_limit=None):
    """
    Method for removing unmanaged compute vms from user's folder
    """
    operation_ids_to_hosts = {}
    for fqdn, opts in host_group.hosts.items():
        operation_id = user_compute_api.instance_absent(fqdn, opts['vtype_id'], referrer_id=opts['subcid'])
        if operation_id:
            operation_ids_to_hosts[operation_id] = fqdn
            if running_ops_limit and len(operation_ids_to_hosts) >= running_ops_limit:
                _finish_host_deletion(user_compute_api, dns_api, operation_ids_to_hosts)

    while operation_ids_to_hosts:
        _finish_host_deletion(user_compute_api, dns_api, operation_ids_to_hosts)


def classify_host_map(hosts):
    """
    Classify dict of hosts on masternodes, datanodes or computenodes.
    """
    master_hosts = {}
    data_hosts = {}
    compute_hosts = {}
    for host, opts in hosts.items():
        if MASTER_ROLE_TYPE in opts['roles']:
            master_hosts[host] = opts
        elif DATA_ROLE_TYPE in opts['roles']:
            data_hosts[host] = opts
        else:
            compute_hosts[host] = opts

    return master_hosts, data_hosts, compute_hosts


def get_cluster_pillar(hosts, internal_api):
    master_nodes, _, _ = classify_host_map(hosts)
    master_fqdn, _ = master_nodes.popitem()
    cluster_pillar = internal_api.get_fqdn_pillar_cfg_unmanaged(master_fqdn)
    return cluster_pillar


def split_by_subcid(hosts, subcid):
    """
    Split dict of hosts by provided subcid
    """
    subcluster = {}
    other = {}
    for fqdn, host in hosts.items():
        if subcid == host['subcid']:
            subcluster[fqdn] = host
        else:
            other[fqdn] = host
    return subcluster, other


def split_by_subcids(hosts):
    """
    Split dict of hosts by subcids
    """
    subcluster = defaultdict(dict)
    for fqdn, host in hosts.items():
        subcluster[host['subcid']][fqdn] = host
    return subcluster


def get_create_vms_arguments(host_group):
    """
    Returns arguments for compute instances creation API call for specified host group.
    """
    # pylint: disable=no-self-use
    for host, opts in host_group.hosts.items():
        if opts['vtype'] == 'compute':
            vm_create_attributes = {
                'geo': opts['geo'],
                'fqdn': host,
                'managed_fqdn': '',
                'metadata': opts.get('meta'),
                'image_type': None,
                'image_id': host_group.properties.compute_image_id,
                'platform_id': opts['platform_id'].replace('mdb-', 'standard-'),
                'subnet_id': opts['subnet_id'],
                'memory': opts['memory_limit'],
                'cores': opts['cpu_limit'],
                'core_fraction': get_core_fraction(opts['cpu_guarantee'], opts['cpu_limit']),
                'gpus': opts['gpu_limit'],
                'io_cores': opts['io_cores_limit'],
                'assign_public_ip': opts['assign_public_ip'],
                'service_account_id': opts['service_account_id'],
                'labels': opts.get('labels'),
                'root_disk_size': opts['space_limit'],
                'root_disk_type_id': opts['disk_type_id'],
                'security_groups': opts.get('security_group_ids'),
                'disk_type_id': '',
                'disk_size': None,
                'folder_id': None,
                'revertable': False,
                'references': [{'referrer_type': 'dataproc.subcluster', 'referrer_id': opts['subcid']}],
                'host_group_ids': opts['host_group_ids'],
            }
            yield vm_create_attributes
        else:
            raise RuntimeError('Unknown vtype: {vtype} for host {host}'.format(vtype=opts['vtype'], host=host))


def render_user_data(pillar):
    """
    render user-data for salt pillar and cloud-init
    """
    return '#cloud-config\n' + yaml.safe_dump(
        yaml.safe_load(json.dumps(pillar)),
        default_flow_style=False,
    )


def get_image_version(pillar: dict) -> str:
    """
    Returns version of image from pillar
    """
    return pillar.get('data', {}).get('unmanaged', {}).get('version')


class HadoopHost(GenericHost):
    service_account_id: str
    labels: dict[str, str]
    meta: dict[str, Any]
    security_group_ids: list[str]


class HadoopHostGroup:
    properties: Optional[SimpleNamespace]
    hosts: dict[str, HadoopHost]


def set_hosts_params_from_pillar(
    host_group: HadoopHostGroup,
    folder_id: str,
    internal_api: InternalApi,
    validate_service_account_via: ResourceManagerApi = None,
    security_group_ids: list[str] = None,
):
    """
    For each host within host_group set following attributes: service_account_id, labels, meta.
    """
    for host, opts in host_group.hosts.items():
        host_cfg = internal_api.get_fqdn_pillar_cfg_unmanaged(host)
        service_account_id = host_cfg.get('data', {}).get('service_account_id')
        if not service_account_id:
            raise UserExposedException('Empty service account')
        if validate_service_account_via:
            roles = validate_service_account_via.list_folder_roles(folder_id, service_account_id)
            if not {'mdb.dataproc.agent', 'dataproc.agent'} & roles:
                raise UserExposedException(f'Service account "{service_account_id}" should have role "dataproc.agent"')
        opts['service_account_id'] = service_account_id
        subcid = opts['subcid']
        opts['labels'] = host_cfg.get('data', {}).get('labels', {}).get(subcid, {})
        opts['meta'] = {
            'user-data': render_user_data(host_cfg),
            'folder-id': folder_id,
        }
        # Enable oslogin feature for clusters with image newer than 1.x
        version = get_image_version(host_cfg)
        if version and not version.startswith('1.'):
            opts['meta']['enable-oslogin'] = 'true'
        opts['security_group_ids'] = security_group_ids or []


def update_group_meta(host_group: HadoopHostGroup, user_compute_api: UserComputeApi):
    """
    Method for updating compute vms metadata
    """
    operations = []
    for host, opts in host_group.hosts.items():
        if opts['vtype'] == 'compute':
            operation_id = user_compute_api.update_instance_attributes(
                fqdn=host,
                metadata=opts.get('meta'),
                referrer_id=opts.get('subcid'),
                instance_id=opts.get('vtype_id'),
            )
            operations.append(operation_id)
        else:
            raise RuntimeError('Unknown vtype: {vtype} for host {host}'.format(vtype=opts['vtype'], host=host))

    user_compute_api.operations_wait(operations)


def get_instance_group_config(
    subcid: str,
    pillar: dict,
    with_reference: bool = True,
    security_group_ids: list[str] = None,
    host_group_ids: list[str] = None,
    is_dualstack_subnet: bool = False,
):
    compute_metadata = deepcopy(pillar)
    image_id = compute_metadata['data']['image']
    main_subcluster_id = compute_metadata['data']['subcluster_main_id']
    masternode_fqdn = compute_metadata['data']['topology']['subclusters'][main_subcluster_id]['hosts'][0]
    _, masternode_fqdn_host_suffix = masternode_fqdn.split('.', 1)
    subcluster_config = compute_metadata['data']['topology']['subclusters'][subcid]
    subcluster_name = subcluster_config['name'].lower().replace('_', '-')[:32]
    # instance group name must be unique in folder and must pass [a-z]([-a-z0-9]{0,61}[a-z0-9])? check
    instance_group_name = f'dataproc-{subcid}-{subcluster_name}'
    network_id = compute_metadata['data']['topology']['network_id']
    zone_id = compute_metadata['data']['topology']['zone_id']

    zone_letter = zone_id[-1]
    instance_id_template = '{instance.short_id}'  # Instance Group Service template
    # instance group hostname must be unique in subnet and must pass [a-z][-a-z0-9]{1,61}[a-z0-9] check
    name_template = f'rc1{zone_letter}-dataproc-g-{instance_id_template}'
    fqdn_template = f'{name_template}.{masternode_fqdn_host_suffix}.'

    network_spec = {
        'network_id': network_id,
        'subnet_ids': [subcluster_config['subnet_id']],
        'primary_v4_address_spec': {
            'dns_record_specs': [
                {'fqdn': fqdn_template},
            ],
        },
    }

    if is_dualstack_subnet:
        network_spec['primary_v6_address_spec'] = {
            'dns_record_specs': [
                {'fqdn': fqdn_template},
            ],
        }

    if security_group_ids is not None:
        network_spec['security_group_ids'] = security_group_ids

    if subcluster_config.get('assignPublicIp'):
        network_spec['primary_v4_address_spec']['one_to_one_nat_spec'] = {
            'ip_version': 'IPV4',
        }

    # data:instance is the per-node metadata section
    if 'instance' not in compute_metadata['data']:
        compute_metadata['data']['instance'] = {}
    compute_metadata['data']['instance']['subcid'] = subcid
    compute_metadata['data']['instance']['instance_group_id'] = '{instance_group.id}'  # Instance Group Service template
    compute_metadata['data']['instance']['fqdn'] = fqdn_template

    config = {
        'name': instance_group_name,
        'service_account_id': compute_metadata['data']['service_account_id'],
        'instance_template': {
            'service_account_id': compute_metadata['data']['service_account_id'],
            'name': name_template,
            'hostname': name_template,
            'scheduling_policy': subcluster_config['instance_group_config']['instance_template']['scheduling_policy'],
            'metadata': {
                # allow metadata update without instance restart
                # https://st.yandex-team.ru/CLOUD-39378#5f3b92e8349a666dff2eaf95
                'internal-metadata-live-update-keys': 'data,user-data,ssh-keys,internal-metadata-live-update-keys',
                'user-data': render_user_data(compute_metadata),
            },
            'platform_id': subcluster_config['resources']['platform_id'],
            'resources_spec': {
                'memory': subcluster_config['resources']['memory'],
                'cores': subcluster_config['resources']['cores'],
                'core_fraction': subcluster_config['resources']['core_fraction'],
            },
            'boot_disk_spec': {
                'mode': 'READ_WRITE',
                'disk_spec': {
                    'type_id': subcluster_config['resources']['disk_type_id'],
                    'size': subcluster_config['resources']['disk_size'],
                    'image_id': image_id,
                },
            },
            'network_interface_specs': [
                network_spec,
            ],
        },
        'scale_policy': subcluster_config['instance_group_config']['scale_policy'],
        'deploy_policy': {
            'max_unavailable': 1,
            'max_creating': 3,
            'startup_duration': '0s',
            'strategy': 'OPPORTUNISTIC',
        },
        'allocation_policy': {
            'zones': [
                {'zone_id': zone_id},
            ],
        },
    }
    # Enable oslogin for images 2.0 and newer
    version = get_image_version(compute_metadata)
    if version and not version.startswith('1.'):
        config['instance_template']['metadata']['enable-oslogin'] = 'true'
    if host_group_ids:
        config['instance_template']['placement_policy'] = {
            'host_affinity_rules': [
                {
                    'key': 'yc.hostGroupId',
                    'op': 'IN',
                    'values': host_group_ids,
                },
            ],
        }
    if with_reference:
        config['references'] = [
            {
                'referrer': {
                    'type': 'dataproc.subcluster',
                    'id': subcluster_config['subcid'],
                },
                'type': 'MANAGED_BY',
            },
        ]
    return config
