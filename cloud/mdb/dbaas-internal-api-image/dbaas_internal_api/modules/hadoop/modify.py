# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop cluster modification
"""
from copy import deepcopy
from datetime import timedelta
from typing import List, Optional, Dict, Tuple

from flask import g
from marshmallow import Schema
import semver

from ...core.exceptions import DbaasClientError, DbaasNotImplementedError, NoChangesError
from ...core.types import Operation
from ...utils import metadb, validation
from ...utils.cluster.create import create_hosts_unmanaged
from ...utils.cluster.update import hosts_change_flavor, update_cluster_labels, update_cluster_name
from ...utils.metadata import ModifyClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.network import validate_security_groups, get_provider
from ...utils.types import (
    ClusterInfo,
    ClusterStatus,
    ExistedHostResources,
    Host,
    LabelsDict,
    OperationParameters,
    parse_resources,
)
from ...utils.validation import check_cluster_not_in_status
from .constants import (
    FQDN_MARKS,
    MY_CLUSTER_TYPE,
)
from .compute_quota import check_compute_quota_on_modification
from .metadata import ModifySubclusterMetadata
from .pillar import HadoopPillar, get_cluster_pillar
from .traits import HadoopOperations, HadoopTasks
from .types import HadoopSubclusterSpec
from .utils import (
    fetch_subnet,
    get_raw_resources_by_resource_preset,
    get_role_services,
    set_pillar_labels,
    get_default_autoscaling_rule,
)
from .version import get_dataproc_image_config_by_version
from .validation import (
    validate_cluster_is_alive,
    validate_pillar,
    validate_service_account,
    validate_log_group,
    validate_subcluster,
    validate_service_account_for_instance_groups,
    validate_dataproc_security_groups,
    validate_service_account_for_folder,
)


def _add_hosts_to_subcluster(
    cid: str, subcid: str, hosts_count_to_add: int, zone_id: str, current_subcluster_config: dict, role: str
) -> List[Host]:
    host_specs = [
        {
            'zone_id': zone_id,
            'assign_public_ip': current_subcluster_config['assign_public_ip'],
            'subnet_id': current_subcluster_config['subnet_id'],
        }
    ] * hosts_count_to_add

    created_hosts = create_hosts_unmanaged(
        cid=cid,
        subcid=subcid,
        shard_id=None,
        flavor=validation.get_flavor_by_name(current_subcluster_config['resources']['resource_preset_id']),
        volume_size=current_subcluster_config['resources']['disk_size'],
        host_specs=host_specs,
        subnet_id=current_subcluster_config['subnet_id'],
        disk_type_id=current_subcluster_config['resources']['disk_type_id'],
        fqdn_mark=FQDN_MARKS[role],
    )
    return created_hosts


def _modify_subcluster_resources(
    subcluster: dict,
    requested_resources_dict: dict,
    instance_group_id: str = None,
    requested_host_count: int = None,
):
    current_host_count = subcluster['hosts_count']
    hosts = []
    if not instance_group_id:
        hosts = metadb.get_hosts(subcluster['cid'], subcid=subcluster['subcid'])
        if not hosts:
            raise DbaasClientError('Subcluster "{}" is empty.'.format(subcluster['subcid']))
        current_host_count = len(hosts)
    if 'disk_type_id' in requested_resources_dict:
        if subcluster['resources']['disk_type_id'] != requested_resources_dict['disk_type_id']:
            raise DbaasNotImplementedError('Changing disk_type_id for Data Proc subcluster is not supported yet')
    requested_resources = parse_resources(requested_resources_dict)

    current_resources = ExistedHostResources(
        resource_preset_id=subcluster['resources']['resource_preset_id'],
        disk_size=subcluster['resources']['disk_size'],
        disk_type_id=subcluster['resources']['disk_type_id'],
    )

    changes = False

    if (
        requested_resources.resource_preset_id
        and requested_resources.resource_preset_id != current_resources.resource_preset_id
    ):
        # The resources that user wants to apply, as returned by Metadb
        new_instance_type_obj = validation.get_flavor_by_name(requested_resources.resource_preset_id)
        validation.validate_hosts_count(
            MY_CLUSTER_TYPE,
            subcluster['role'],
            requested_resources.resource_preset_id,  # pylint: disable=no-member
            requested_resources.disk_type_id or current_resources.disk_type_id,
            current_host_count,
        )

        if instance_group_id:
            changes = True
        elif hosts:
            diff, id_changed = hosts_change_flavor(
                subcluster['cid'], hosts, new_instance_type_obj, is_update_resources=False
            )
            # Check for any actual change
            if any(value != 0 for value in diff.values()) or id_changed:
                changes = True

    if requested_resources.disk_size is not None and requested_resources.disk_size != current_resources.disk_size:
        if requested_resources.disk_size < current_resources.disk_size:
            raise DbaasNotImplementedError('Decreasing disk size for Data Proc subcluster nodes is not supported yet')

        if instance_group_id:
            changes = True
        else:
            add_space = 0
            for host in hosts:
                metadb.update_host(host['fqdn'], cid=subcluster['cid'], space_limit=requested_resources.disk_size)
                add_space += requested_resources.disk_size - host['space_limit']
            if add_space != 0:
                changes = True

    if changes:
        check_compute_quota_on_modification(
            current_resources=current_resources,
            current_host_count=current_host_count,
            requested_resources=requested_resources,
            requested_host_count=requested_host_count,
        )
        current_resources.update(requested_resources)

    flavor = validation.get_flavor_by_name(current_resources.resource_preset_id)
    current_resources_dict = {
        'disk_type_id': current_resources.disk_type_id,
        'disk_size': current_resources.disk_size,
        'resource_preset_id': current_resources.resource_preset_id,
    }

    task_args = {}
    if instance_group_id:
        current_resources_dict['platform_id'] = flavor['platform_id'].replace('mdb-', 'standard-')
        task_args['instance_group_id'] = instance_group_id
    return (
        OperationParameters(
            changes=changes,
            task_args=task_args,
            time_limit=timedelta(hours=1),
        ),
        current_resources_dict,
    )


def _get_modified_instance_group_config(
    current_subcluster_config,
    requested_autoscaling_config,
    requested_hosts_count,
    image_version,
) -> tuple[Optional[Dict], bool]:
    """
    Modifes instance group based subcluster
    Returns modified instance group config and bool flag "is_downtime_needed"
    """

    current_instance_group_config = deepcopy(current_subcluster_config['instance_group_config'])
    current_scale_policy_config = current_instance_group_config['scale_policy']
    is_downtime_needed = False
    if requested_hosts_count is not None:
        if requested_hosts_count != current_scale_policy_config['auto_scale']['initial_size']:
            current_subcluster_config['hosts_count'] = requested_hosts_count
            current_scale_policy_config['auto_scale']['initial_size'] = requested_hosts_count
            current_scale_policy_config['auto_scale']['min_zone_size'] = requested_hosts_count
    if requested_autoscaling_config:
        if 'max_hosts_count' in requested_autoscaling_config:
            min_hosts_count = current_scale_policy_config['auto_scale']['initial_size']
            if requested_autoscaling_config['max_hosts_count'] < min_hosts_count:
                raise DbaasClientError('Max hosts count must be greater than min hosts count')
            if requested_autoscaling_config['max_hosts_count'] != current_scale_policy_config['auto_scale']['max_size']:
                current_scale_policy_config['auto_scale']['max_size'] = requested_autoscaling_config['max_hosts_count']
        if 'measurement_duration' in requested_autoscaling_config:
            requested_measurement_duration = f"{requested_autoscaling_config['measurement_duration']}s"
            if requested_measurement_duration != current_scale_policy_config['auto_scale']['measurement_duration']:
                current_scale_policy_config['auto_scale']['measurement_duration'] = requested_measurement_duration
        if 'stabilization_duration' in requested_autoscaling_config:
            requested_stabilization_duration = f"{requested_autoscaling_config['stabilization_duration']}s"
            if requested_stabilization_duration != current_scale_policy_config['auto_scale']['stabilization_duration']:
                current_scale_policy_config['auto_scale']['stabilization_duration'] = requested_stabilization_duration
        if 'warmup_duration' in requested_autoscaling_config:
            requested_warmup_duration = f"{requested_autoscaling_config['warmup_duration']}s"
            if requested_warmup_duration != current_scale_policy_config['auto_scale']['warmup_duration']:
                current_scale_policy_config['auto_scale']['warmup_duration'] = requested_warmup_duration

        requested_cpu_utilization_target = requested_autoscaling_config.get('cpu_utilization_target')
        if requested_cpu_utilization_target == 0:
            current_scale_policy_config['auto_scale']['custom_rules'] = [
                get_default_autoscaling_rule(cid=current_subcluster_config['cid'], image_version=image_version),
            ]
            if 'cpu_utilization_rule' in current_scale_policy_config['auto_scale']:
                del current_scale_policy_config['auto_scale']['cpu_utilization_rule']
        elif requested_cpu_utilization_target is not None:
            if requested_cpu_utilization_target < 10:
                requested_autoscaling_config['cpu_utilization_target'] = 10
            if requested_cpu_utilization_target:
                current_scale_policy_config['auto_scale']['cpu_utilization_rule'] = {
                    'utilization_target': requested_cpu_utilization_target
                }
                if 'custom_rules' in current_scale_policy_config['auto_scale']:
                    del current_scale_policy_config['auto_scale']['custom_rules']

        if 'preemptible' in requested_autoscaling_config:
            if (
                requested_autoscaling_config['preemptible']
                != current_instance_group_config['instance_template']['scheduling_policy']['preemptible']
            ):
                current_instance_group_config['instance_template']['scheduling_policy'][
                    'preemptible'
                ] = requested_autoscaling_config['preemptible']
                is_downtime_needed = True

    is_instance_group_changed = current_subcluster_config['instance_group_config'] != current_instance_group_config
    if is_instance_group_changed:
        return current_instance_group_config, is_downtime_needed
    return None, False


def _modify_subcluster(
    cid: str,
    subcid: str,
    pillar: HadoopPillar,
    requested_name: str = None,
    requested_hosts_count: int = None,
    requested_resources: dict = None,
    requested_autoscaling_config: dict = None,
) -> Tuple[List[OperationParameters], bool, bool]:
    # pylint: disable=too-many-locals
    states = []
    current_subcluster_config = pillar.get_subcluster(subcid)
    if not current_subcluster_config:
        raise DbaasClientError(f'Subcluster {subcid} does not exist')
    role = current_subcluster_config['role']

    desc_changes = False

    zone_id = pillar.zone_id
    subnet_id = current_subcluster_config['subnet_id']

    image_version = semver.VersionInfo.parse(pillar.semantic_version)
    image_config = get_dataproc_image_config_by_version(image_version)
    if 'assign_public_ip' not in current_subcluster_config:
        current_subcluster_config['assign_public_ip'] = False

    is_instance_group = 'instance_group_config' in current_subcluster_config

    # subnet from another folder may be used for the subcluster
    vpc = get_provider()
    cluster_subnet_folder_id = vpc.get_subnet(subnet_id, current_subcluster_config['assign_public_ip'])['folderId']
    if is_instance_group:
        validate_service_account_for_folder(pillar.service_account_id, cluster_subnet_folder_id)

    if not is_instance_group and requested_autoscaling_config:
        raise DbaasClientError(
            f'Subcluster {subcid} does not support autoscaling.'
            ' If you want an autoscaling subcluster,'
            ' you must create new subcluster with "max-hosts-count" argument set.'
        )

    if is_instance_group:
        instance_group_id = metadb.get_instance_group(subcid)['instance_group_id']
    else:
        instance_group_id = None

    is_compute_quota_checked = False
    if requested_hosts_count is not None:
        if requested_hosts_count < 1:
            raise DbaasClientError('Host count must be at least 1')
        if requested_hosts_count < current_subcluster_config['hosts_count']:
            if image_version < semver.VersionInfo(1, 2):
                raise DbaasClientError('Decreasing nodes number is supported for clusters with version >= 1.2')

    if requested_resources:
        if 'resource_preset_id' in requested_resources:
            flavor = metadb.get_flavor_by_name(requested_resources['resource_preset_id'])
            is_burstable = flavor['cpu_guarantee'] < flavor['cpu_limit']
            if is_instance_group and is_burstable:
                raise DbaasClientError(
                    'Burstable instances are not supported for instance groups. Choose another preset.'
                )
        state, current_resources_dict = _modify_subcluster_resources(
            current_subcluster_config,
            requested_resources,
            instance_group_id,
            requested_hosts_count,
        )
        if state.changes:
            is_compute_quota_checked = True
            states.append(state)
            # Forward raw resource to pillar for masternode, because she should know sizes for yarn scheduler
            raw_resources = get_raw_resources_by_resource_preset(current_resources_dict['resource_preset_id'])
            current_resources_dict.update(raw_resources)
            current_subcluster_config['resources'] = current_resources_dict

    if requested_hosts_count and not is_compute_quota_checked:
        current_resources = ExistedHostResources(
            resource_preset_id=current_subcluster_config['resources']['resource_preset_id'],
            disk_size=current_subcluster_config['resources']['disk_size'],
            disk_type_id=current_subcluster_config['resources']['disk_type_id'],
        )
        check_compute_quota_on_modification(
            current_resources=current_resources,
            current_host_count=current_subcluster_config['hosts_count'],
            requested_host_count=requested_hosts_count,
        )

    if is_instance_group and (requested_autoscaling_config or requested_hosts_count):
        modified_instance_group_config, is_downtime_needed = _get_modified_instance_group_config(
            current_subcluster_config,
            requested_autoscaling_config,
            requested_hosts_count,
            image_version=pillar.semantic_version,
        )
        # if modified_instance_group_config is None then the config is not changed
        if modified_instance_group_config:
            current_subcluster_config['instance_group_config'] = modified_instance_group_config
            states.append(
                OperationParameters(
                    changes=True,
                    time_limit=timedelta(hours=1),
                    task_args={'instance_group_subclusters': [(subcid, instance_group_id, is_downtime_needed)]},
                )
            )
        if requested_autoscaling_config and 'decommission_timeout' in requested_autoscaling_config:
            current_decommission_timeout = current_subcluster_config['decommission_timeout']
            requested_decommission_timeout = requested_autoscaling_config['decommission_timeout']
            if requested_decommission_timeout != current_decommission_timeout:
                current_subcluster_config['decommission_timeout'] = requested_autoscaling_config['decommission_timeout']
                desc_changes = True
    else:
        if requested_hosts_count is not None:
            if requested_hosts_count < current_subcluster_config['hosts_count']:
                hosts_count_to_remove = current_subcluster_config['hosts_count'] - requested_hosts_count
                current_subcluster_config['hosts_count'] = requested_hosts_count
                hosts_to_leave = current_subcluster_config['hosts'][:-hosts_count_to_remove]
                hosts_to_decommission = current_subcluster_config['hosts'][-hosts_count_to_remove:]
                current_subcluster_config['hosts'] = hosts_to_leave

                host_meta_to_decommission = []
                for host_meta in metadb.get_hosts(cid, subcid=subcid):
                    if host_meta['fqdn'] in hosts_to_decommission:
                        host_meta_to_decommission.append(host_meta)
                metadb.delete_hosts_batch(hosts_to_decommission, cid)

                states.append(
                    OperationParameters(
                        changes=True,
                        task_args={'hosts_to_decommission': host_meta_to_decommission},
                        time_limit=timedelta(hours=1),
                    )
                )

            elif requested_hosts_count > current_subcluster_config['hosts_count']:
                hosts_count_to_add = requested_hosts_count - current_subcluster_config['hosts_count']
                current_subcluster_config['hosts_count'] = requested_hosts_count
                states.append(OperationParameters(changes=True, task_args={}, time_limit=timedelta(hours=1)))

                new_hosts = _add_hosts_to_subcluster(
                    cid=cid,
                    subcid=subcid,
                    hosts_count_to_add=hosts_count_to_add,
                    zone_id=zone_id,
                    current_subcluster_config=current_subcluster_config,
                    role=role,
                )
                current_subcluster_config['hosts'] += [h['fqdn'] for h in new_hosts]

    if requested_name is not None and current_subcluster_config['name'] != requested_name:
        current_subcluster_config['name'] = requested_name
        desc_changes = True
        if instance_group_id:
            states.append(
                OperationParameters(
                    changes=True,
                    task_args={'instance_group_subclusters': [(subcid, instance_group_id, False)]},
                    time_limit=timedelta(hours=1),
                )
            )

    changes = False
    for state in states:
        changes = state.changes or changes

    pillar_changed = False
    if changes or desc_changes:
        subcluster = {
            'cid': cid,
            'name': current_subcluster_config['name'],
            'role': current_subcluster_config['role'],
            'hosts_count': current_subcluster_config['hosts_count'],
            'resources': current_subcluster_config['resources'],
            'subnet_id': subnet_id,
            'services': get_role_services(
                pillar.services,
                image_config,  # type: ignore
                current_subcluster_config['role'],
            ),
        }
        if 'instance_group_config' in current_subcluster_config:
            subcluster['instance_group_config'] = current_subcluster_config['instance_group_config']

        subcluster_spec = HadoopSubclusterSpec(subcluster)
        validate_subcluster(subcluster_spec, zone_id, image_config, pillar.version)  # type: ignore
        pillar.set_subcluster(current_subcluster_config)
        validate_pillar(pillar)
        pillar_changed = True

    return states, desc_changes, pillar_changed


def _modify_subclusters(
    cid: str,
    config_spec: dict,
    pillar: HadoopPillar,
    states: List[OperationParameters],
) -> bool:
    # pylint: disable=too-many-locals
    subclusters = config_spec.get('subclusters', [])

    pillar_changed = False
    for requested_subcluster_config in subclusters:
        current_subcluster_config = pillar.get_subcluster(requested_subcluster_config['subcid'])
        if not current_subcluster_config:
            raise DbaasClientError(
                'Subcluster "{}" not found. To create new subcluster use "subclusters" API.'.format(
                    requested_subcluster_config['subcid']
                )
            )

        subcluster_states, _, subcluster_pillar_changed = _modify_subcluster(
            cid=cid,
            subcid=requested_subcluster_config['subcid'],
            pillar=pillar,
            requested_name=requested_subcluster_config.get('name'),
            requested_hosts_count=requested_subcluster_config.get('hosts_count'),
            requested_resources=requested_subcluster_config.get('resources'),
            requested_autoscaling_config=requested_subcluster_config.get('autoscaling_config'),
        )
        pillar_changed = pillar_changed or subcluster_pillar_changed
        states += subcluster_states

    return pillar_changed


def modify_security_groups(
    cluster_obj: dict,
    pillar: HadoopPillar,
    states: List[OperationParameters],
    security_group_ids: List[str],
):
    sg_ids = set(cluster_obj.get('user_sgroup_ids', []))
    if sg_ids == set(security_group_ids):
        return

    if security_group_ids:
        subcluster = pillar.subcluster_main
        subnet = fetch_subnet(
            subcluster['subnet_id'], subcluster.get('assign_public_ip', False), pillar.zone_id, pillar.network_id
        )
        nat_enabled = subnet.get('egressNatEnable', False)

        validate_security_groups(security_group_ids, cluster_obj['network_id'])
        validate_dataproc_security_groups(
            pillar, nat_enabled, subcluster.get('assign_public_ip', False), security_group_ids
        )
    states.append(
        OperationParameters(
            changes=True, task_args={'security_group_ids': security_group_ids}, time_limit=timedelta(hours=1)
        )
    )
    # For updating IG security_groups we should add list of IG to task_args
    # _modify_subclusters doesn't work, cz we doesn't have changes for subclusters in config_spec
    ig_subclusters = []
    for subcluster in pillar.subclusters:
        if not subcluster.get('instance_group_config'):
            continue
        subcid = subcluster['subcid']
        downtime = False
        instance_group_id = metadb.get_instance_group(subcid)['instance_group_id']
        ig_subclusters.append((subcid, instance_group_id, downtime))

    if ig_subclusters:
        states.append(
            OperationParameters(
                changes=True, task_args={'instance_group_subclusters': ig_subclusters}, time_limit=timedelta(hours=0)
            )
        )


def get_task_args_to_restart_instance_groups(pillar: HadoopPillar):
    task_args = {'instance_group_subclusters': []}  # type: dict
    is_downtime_needed = True
    for subcluster in pillar.subclusters:
        subcid = subcluster['subcid']
        if 'instance_group_config' in subcluster:
            instance_group_id = metadb.get_instance_group(subcid)['instance_group_id']
            task_args['instance_group_subclusters'].append((subcid, instance_group_id, is_downtime_needed))
    return task_args


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MODIFY)
def modify_hadoop_cluster(
    cluster_obj: dict,
    _schema: Schema,
    config_spec: dict = None,
    labels: LabelsDict = None,
    description: str = None,
    name: Optional[str] = None,
    service_account_id: str = None,
    bucket: str = None,
    decommission_timeout: int = None,
    ui_proxy: Optional[bool] = None,
    security_group_ids: List[str] = None,
    deletion_protection: Optional[bool] = None,
    log_group_id: Optional[str] = None,
    **_,
) -> Operation:
    """
    Modifies Hadoop cluster. Return Operation
    """
    # pylint: disable=too-many-locals
    states = []  # type: List[OperationParameters]

    pillar = get_cluster_pillar(cluster_obj)
    image_version = semver.VersionInfo.parse(pillar.semantic_version)
    if decommission_timeout and image_version < semver.VersionInfo(1, 2):
        raise DbaasClientError('Decommission is supported for clusters with version >= 1.2')

    task_args = {'image_id': pillar.image, 'decommission_timeout': decommission_timeout}  # type: dict

    if security_group_ids is not None:
        modify_security_groups(cluster_obj, pillar, states, security_group_ids)

    pillar_changed = _modify_subclusters(
        cid=cluster_obj['cid'],
        config_spec=config_spec or {},
        pillar=pillar,
        states=states,
    )

    config_spec = config_spec or {}
    version = config_spec.get('version_id')
    if version and pillar.version != version:
        raise DbaasClientError('Changing version is not allowed')

    hadoop_config = config_spec.get('hadoop', {})
    services = hadoop_config.get('services')
    if services and pillar.services != set(services):
        raise DbaasClientError('Changing services is not allowed')

    ssh_public_keys = hadoop_config.get('ssh_public_keys')
    if ssh_public_keys and pillar.ssh_public_keys != ssh_public_keys:
        raise DbaasNotImplementedError('Changing ssh_public_keys for Data Proc cluster is not supported yet')

    properties = hadoop_config.get('properties')
    if properties is not None and properties != pillar.user_properties:
        pillar.user_properties = properties
        pillar_changed = True
        task_args['restart'] = True
        task_args.update(get_task_args_to_restart_instance_groups(pillar))

    cluster_folder_id = g.folder['folder_ext_id']
    if service_account_id and service_account_id != pillar.service_account_id:
        validate_service_account(service_account_id, cluster_folder_id)
        if metadb.get_instance_groups(cluster_obj['cid']):
            validate_service_account_for_instance_groups(service_account_id, cluster_folder_id)

        pillar.service_account_id = service_account_id

        if log_group_id is not None or pillar.log_group_id:
            validate_log_group(
                log_group_id=(log_group_id or pillar.log_group_id),
                service_account_id=(service_account_id or pillar.service_account_id),
                cluster_folder_id=cluster_folder_id,
            )

        pillar_changed = True

    # if new log group or existent log group and new service account
    # log_group_id will be None if it is not present in update_mask,
    # otherwise user wants to remove log_group_id by setting it to empty string explicitly
    if log_group_id is not None and log_group_id != pillar.log_group_id:
        validate_log_group(
            log_group_id=(log_group_id or pillar.log_group_id),
            service_account_id=(service_account_id or pillar.service_account_id),
            cluster_folder_id=cluster_folder_id,
        )
        pillar.log_group_id = log_group_id or None  # prevent setting empty string to pillar
        pillar_changed = True
        task_args['restart'] = True
        task_args.update(get_task_args_to_restart_instance_groups(pillar))

    if bucket:
        pillar.user_s3_bucket = bucket
        pillar_changed = True

    if labels:
        set_pillar_labels(pillar, labels)
        pillar_changed = True

    if ui_proxy is not None and pillar.ui_proxy != ui_proxy:
        pillar.ui_proxy = ui_proxy
        pillar_changed = True

    if pillar_changed:
        validate_pillar(pillar)
        metadb.update_cluster_pillar(cluster_obj['cid'], pillar)
        states.append(OperationParameters(changes=True, task_args={}, time_limit=timedelta(hours=0)))

    for state in states:
        for key, value in state.task_args.items():
            if key in task_args and isinstance(task_args[key], list) and isinstance(value, list):
                # hosts_to_decommission list extending for updating several subclusters case
                task_args[key] += value
            else:
                task_args[key] = value

    changes = any((x.changes for x in states))

    if update_cluster_name(cluster_obj, name):
        changes = True

    if update_cluster_labels(cluster_obj, labels):
        changes = True  # dataproc assigns labels to real VMs, so it cannot be

    if changes:
        validate_cluster_is_alive(cluster_obj['cid'])
        time_limit = timedelta(hours=1)
        if decommission_timeout:
            time_limit = timedelta(seconds=min(decommission_timeout + 3600, 86400))
        check_cluster_not_in_status(ClusterInfo.make(cluster_obj), ClusterStatus.stopped)
        return create_operation(
            task_type=HadoopTasks.modify,
            operation_type=HadoopOperations.modify,
            metadata=ModifyClusterMetadata(),
            time_limit=time_limit,
            cid=cluster_obj['cid'],
            task_args=task_args,
        )

    raise NoChangesError()


@register_request_handler(MY_CLUSTER_TYPE, Resource.SUBCLUSTER, DbaasOperation.MODIFY)
def modify_hadoop_subcluster(
    cluster_obj: dict,
    subcid: str,
    name: str = None,
    hosts_count: int = None,
    resources: dict = None,
    decommission_timeout: int = None,
    autoscaling_config: dict = None,
    **_,
) -> Operation:
    """
    Modifies Hadoop subcluster. Return Operation
    """
    validate_cluster_is_alive(cluster_obj['cid'])
    pillar = get_cluster_pillar(cluster_obj)
    cid = cluster_obj['cid']
    image_version = semver.VersionInfo.parse(pillar.semantic_version)
    if decommission_timeout and image_version < semver.VersionInfo(1, 2):
        raise DbaasClientError('Decommission is supported for clusters with version >= 1.2')
    task_args = {'subcid': subcid, 'image_id': pillar.image, 'decommission_timeout': decommission_timeout}  # type: dict
    states, desc_changes, _ = _modify_subcluster(  # type: ignore
        cid=cid,
        subcid=subcid,
        pillar=pillar,
        requested_name=name,
        requested_hosts_count=hosts_count,
        requested_resources=resources,
        requested_autoscaling_config=autoscaling_config,
    )

    changes = False
    for state in states:
        task_args.update(state.task_args)
        changes = changes or state.changes

    metadb.update_cluster_pillar(cid, pillar)

    if changes or desc_changes:
        time_limit = timedelta(hours=1)
        if decommission_timeout:
            time_limit = timedelta(seconds=min(decommission_timeout + 3600, 86400))
        return create_operation(
            task_type=HadoopTasks.subcluster_modify,
            operation_type=HadoopOperations.subcluster_modify,
            metadata=ModifySubclusterMetadata(subcid=subcid),
            time_limit=time_limit,
            cid=cid,
            task_args=task_args,
        )

    raise NoChangesError()
