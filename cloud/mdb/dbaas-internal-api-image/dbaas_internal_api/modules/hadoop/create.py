# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop cluster creation
"""

import semver
from flask import g, current_app
from datetime import timedelta

from typing import List, Optional
from ...core.exceptions import DbaasClientError, ParseConfigError
from ...core.types import Operation
from ...core.id_generators import gen_id
from ...utils import metadb, validation
from ...utils.cluster import create as clusterutil
from ...utils.compute import validate_host_groups
from ...utils.e2e import is_cluster_for_e2e
from ...utils.feature_flags import has_feature_flag
from ...utils.metadata import CreateClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterDescription
from ...utils.network import get_provider
from .constants import (
    FQDN_MARKS,
    MY_CLUSTER_TYPE,
    COMPUTE_SUBCLUSTER_TYPE,
    MASTER_SUBCLUSTER_TYPE,
)
from .compute_quota import check_subclusters_compute_quota
from .metadata import CreateSubclusterMetadata
from .pillar import HadoopPillar, get_cluster_pillar
from .traits import HadoopOperations, HadoopTasks
from .types import HadoopConfigSpec, HadoopSubclusterSpec
from .utils import (
    fetch_subnet,
    get_raw_resources_by_resource_preset,
    get_role_services,
    get_subcluster_raw_resources,
    set_pillar_labels,
    get_default_autoscaling_rule,
    generate_subcluster_name,
    guess_subnet_id,
)
from .version import (
    get_latest_dataproc_version_by_prefix,
    get_dataproc_image_config_by_version,
    get_dataproc_default_version_prefix,
)
from .validation import (
    validate_cluster,
    validate_cluster_is_alive,
    validate_log_group,
    validate_pillar,
    validate_service_account,
    validate_subcluster,
    validate_service_account_for_instance_groups,
    validate_dataproc_security_groups,
    validate_service_account_for_folder,
    check_version_is_allowed,
    validate_initialization_actions,
)


def set_instance_group_config(subcluster, subcid, autoscaling_config, resources, hosts_count, image_version):
    subcluster['subcid'] = subcid
    subcluster['hosts'] = []
    flavor = validation.get_flavor_by_name(resources['resource_preset_id'])
    subcluster['resources']['platform_id'] = flavor['platform_id'].replace('mdb-', 'standard-')

    autoscaling_config = autoscaling_config or {}
    max_hosts_count = autoscaling_config.get('max_hosts_count', 1)
    cpu_utilization_target = autoscaling_config['cpu_utilization_target']
    custom_rules = []

    if cpu_utilization_target <= 0 and not custom_rules:
        custom_rules = [get_default_autoscaling_rule(cid=subcluster['cid'], image_version=image_version)]
    elif cpu_utilization_target < 10:
        # min valid value in IG service. is checked here because adapter sets it to 0 by default.
        # temporary hack until custom metrics are not added to the spec
        cpu_utilization_target = 10

    if max_hosts_count >= hosts_count:
        scale_policy = {
            'auto_scale': {
                'initial_size': hosts_count,
                'max_size': max_hosts_count,
                'measurement_duration': f'{autoscaling_config["measurement_duration"]}s',
                'min_zone_size': hosts_count,
                # time for instance to start and enter cluster
                'warmup_duration': f'{autoscaling_config["warmup_duration"]}s',
                'stabilization_duration': f'{autoscaling_config["stabilization_duration"]}s',
            },
        }
        if cpu_utilization_target > 0:
            scale_policy['auto_scale']['cpu_utilization_rule'] = {'utilization_target': cpu_utilization_target}
        else:
            scale_policy['auto_scale']['custom_rules'] = custom_rules

    else:
        raise DbaasClientError('Maximum hosts count can not be less than (minimum) hosts count.')

    subcluster['instance_group_config'] = {
        'instance_template': {'scheduling_policy': {'preemptible': autoscaling_config.get('preemptible', True)}},
        'scale_policy': scale_policy,
    }
    subcluster['decommission_timeout'] = autoscaling_config['decommission_timeout']
    return subcluster


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CREATE)
def create_cluster_handler(
    name: str,
    environment: str,
    config_spec: dict,
    zone_id: str,
    service_account_id: str,
    folder_id: str,
    bucket: str = None,
    description: str = None,
    labels: dict = None,
    ui_proxy: bool = False,
    security_group_ids: List[str] = None,
    host_group_ids: List[str] = None,
    deletion_protection: bool = False,
    log_group_id: Optional[str] = None,
    **_,
) -> Operation:
    # pylint: disable=too-many-arguments,unused-argument,too-many-locals
    """
    Handler for create Hadoop cluster requests.
    """

    cluster_description = ClusterDescription(name, environment, description, labels)
    validate_service_account(service_account_id, folder_id)

    version_prefix = config_spec.get('version_id')
    if not version_prefix:
        version_prefix = get_dataproc_default_version_prefix()
    image_version, image_id, image_config = get_latest_dataproc_version_by_prefix(version_prefix=version_prefix)
    check_version_is_allowed(image_config, version_prefix)
    default_resources = current_app.config['HADOOP_DEFAULT_RESOURCES']

    all_subclusters = config_spec.get('subclusters', [])

    # set default values from intapi config if some resources parts are omitted in user request
    for index, subcluster in enumerate(all_subclusters):
        for resource_type, default_value in default_resources.items():
            if not subcluster['resources'].get(resource_type):
                config_spec['subclusters'][index]['resources'][resource_type] = default_value

    validate_cluster(config_spec, zone_id, image_config, str(image_version))

    check_subclusters_compute_quota(all_subclusters)

    subcluster_first = all_subclusters[0]
    subnet_id = subcluster_first.get('subnet_id')

    vpc = get_provider()
    if not subnet_id:
        subnet_id = guess_subnet_id(vpc, zone_id, folder_id)

    subnet = fetch_subnet(subnet_id, subcluster_first.get('assign_public_ip', False), zone_id)
    network_id = subnet['networkId']

    if host_group_ids:
        validate_host_groups(host_group_ids, [zone_id])

    cluster, _, private_key = clusterutil.create_cluster(
        cluster_type=MY_CLUSTER_TYPE,
        network_id=network_id,
        description=cluster_description,
        update_used_resources=False,
        security_group_ids=security_group_ids,
        host_group_ids=host_group_ids,
        deletion_protection=deletion_protection,
    )

    pillar = HadoopPillar.make(image_config=image_config)
    pillar.image = config_spec.get('image_id', image_id)  # image_id can be overriden in private api request
    pillar.version = str(image_version)
    pillar.version_prefix = version_prefix

    if log_group_id:
        validate_log_group(
            log_group_id=log_group_id,
            service_account_id=service_account_id,
            cluster_folder_id=folder_id,
        )
        pillar.log_group_id = log_group_id

    pillar.service_account_id = service_account_id
    pillar.zone_id = zone_id
    hadoop_cfg = config_spec.get('hadoop', {})
    if hadoop_cfg.get('services'):
        pillar.services = hadoop_cfg['services']
    if hadoop_cfg.get('initialization_actions'):
        validate_initialization_actions(version=pillar.semantic_version)
        pillar.initialization_actions = hadoop_cfg['initialization_actions']
    pillar.user_properties = hadoop_cfg.get('properties', {})
    if bucket:
        pillar.user_s3_bucket = bucket
    pillar.ssh_public_keys = hadoop_cfg['ssh_public_keys']
    pillar.set_cluster_private_key(private_key)
    if is_cluster_for_e2e(name):
        pillar.set_e2e_cluster()
    pillar.agent_cid = cluster['cid']
    pillar.ui_proxy = ui_proxy
    pillar.set_network_id(network_id)

    nat_enabled = False
    got_assign_public_ip = False
    for subcluster in all_subclusters:
        if not subcluster['name']:
            subcluster['name'] = generate_subcluster_name(subcluster['role'], pillar)
        hosts_count = subcluster.get('hosts_count', 1)
        subnet_id = subcluster.get('subnet_id')
        if not subnet_id:
            subcluster['subnet_id'] = subnet_id = guess_subnet_id(vpc, zone_id, folder_id=folder_id)
        is_assign_public_ip = subcluster.get('assign_public_ip', False)
        if is_assign_public_ip and not got_assign_public_ip:
            # at least one subcluster has assign_public_ip == True
            got_assign_public_ip = True
        subnet = fetch_subnet(subnet_id, is_assign_public_ip, zone_id, network_id)
        if subcluster['role'] == MASTER_SUBCLUSTER_TYPE:
            if subnet.get('egressNatEnable', False):
                nat_enabled = True
            elif has_feature_flag('MDB_DATAPROC_MANAGER'):
                raise DbaasClientError('NAT should be enabled on the subnet of the main subcluster.')
        subcluster['cid'] = cluster['cid']
        subcluster['services'] = get_role_services(pillar.services, image_config, subcluster['role'])
        # Masternode should know a resources of all cluster members.
        # For correct calculation of yarn schedule properties we should forward memory sizes of all nodes
        subcluster['resources'].update(get_subcluster_raw_resources(config_spec, subcluster['name']))
        role = subcluster['role']
        autoscaling_config = subcluster.get('autoscaling_config')
        if role == COMPUTE_SUBCLUSTER_TYPE and autoscaling_config:
            if subcluster['resources']['resource_preset_id'].startswith('b'):
                raise DbaasClientError(
                    'Burstable instances are not supported for instance groups. Choose other preset.'
                )
            validate_service_account_for_instance_groups(service_account_id, folder_id)

            # subnet from another folder may be used for the subcluster
            cluster_subnet_folder_id = vpc.get_subnet(subnet_id, is_assign_public_ip)['folderId']
            if autoscaling_config:
                validate_service_account_for_folder(pillar.service_account_id, cluster_subnet_folder_id)

            subcid = gen_id('subcid')
            del subcluster['autoscaling_config']
            subcluster = set_instance_group_config(
                subcluster,
                subcid,
                autoscaling_config,
                subcluster['resources'],
                hosts_count,
                image_version=pillar.semantic_version,
            )
            metadb.add_instance_group_subcluster(
                cluster_id=cluster['cid'],
                subcid=subcid,
                name=subcluster['name'],
                roles=[role],
            )
        else:
            subcid, hosts = create_subcluster(cluster, subcluster, subnet_id, zone_id, role, hosts_count)
            subcluster['hosts'] = [h['fqdn'] for h in hosts]
            subcluster['subcid'] = subcid

        pillar.set_subcluster(subcluster)

    set_pillar_labels(pillar, labels)
    validate_pillar(pillar)
    validate_dataproc_security_groups(pillar, nat_enabled, got_assign_public_ip, security_group_ids)
    metadb.add_cluster_pillar(cluster['cid'], pillar)
    task_args = {
        'image_id': pillar.image,  # using task args to override the image during tests/debugging
    }
    if security_group_ids:
        task_args['security_group_ids'] = security_group_ids

    time_limit = timedelta(hours=1)
    for init_act in pillar.initialization_actions:
        time_limit += timedelta(seconds=init_act.get('timeout', 600))

    return create_operation(
        task_type=HadoopTasks.create,
        operation_type=HadoopOperations.create,
        metadata=CreateClusterMetadata(),
        cid=cluster['cid'],
        task_args=task_args,
        time_limit=time_limit,
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.SUBCLUSTER, DbaasOperation.CREATE)
def create_subcluster_handler(
    cluster: dict,
    name: str,
    role: str,
    hosts_count: int,
    resources: dict,
    subnet_id: str = '',
    autoscaling_config: dict = None,
    **_,
):
    # pylint: disable=too-many-arguments,unused-argument,too-many-locals
    """
    Handler for create Hadoop subcluster requests.
    """

    # set default values from intapi config if some resources parts are omitted in user request
    default_resources = current_app.config['HADOOP_DEFAULT_RESOURCES']
    for resource_type, default_value in default_resources.items():
        if not resources.get(resource_type):
            resources[resource_type] = default_value

    validate_cluster_is_alive(cluster['cid'])
    pillar = get_cluster_pillar(cluster)
    if not pillar.subcluster_main:
        raise DbaasClientError('You need to set up main subcluster')
    if not name:
        name = generate_subcluster_name(role, pillar)
    if name in pillar.subclusters_names:
        raise DbaasClientError('Subclusters names must be unique')

    vpc = get_provider()
    if not subnet_id:
        if not subnet_id:
            subnet_id = guess_subnet_id(vpc, zone_id=pillar.zone_id, network_id=pillar.network_id)
    public_ip_used = False
    main_subnet = fetch_subnet(pillar.subcluster_main.get('subnet_id', subnet_id), public_ip_used)
    network_id = main_subnet.get('networkId', '')
    # subnet from another folder may be used for the subcluster
    cluster_subnet_folder_id = vpc.get_subnet(subnet_id, public_ip_used)['folderId']
    if autoscaling_config:
        validate_service_account_for_folder(pillar.service_account_id, cluster_subnet_folder_id)
    zone_id = main_subnet['zoneId']
    fetch_subnet(subnet_id, public_ip_used, zone_id, network_id)
    image_config = get_dataproc_image_config_by_version(semver.VersionInfo.parse(pillar.semantic_version))
    subcluster = {
        'cid': cluster['cid'],
        'name': name,
        'role': role,
        'hosts_count': hosts_count,
        'resources': resources,
        'subnet_id': subnet_id,
        'services': get_role_services(pillar.services, image_config, role),  # type: ignore
    }
    subcluster['resources'].update(get_raw_resources_by_resource_preset(resources['resource_preset_id']))
    if role == COMPUTE_SUBCLUSTER_TYPE and autoscaling_config:
        if [subcluster for subcluster in pillar.subclusters if subcluster.get('instance_group_config')]:
            raise DbaasClientError('Only one autoscaling subcluster per cluster is supported in this cluster version.')
        if resources['resource_preset_id'].startswith('b'):
            raise DbaasClientError('Burstable instances are not supported for instance groups. Choose other preset.')
        # create instance group based subcluster
        validate_service_account_for_instance_groups(
            pillar.service_account_id,
            g.folder['folder_ext_id'],
        )
        subcid = gen_id('subcid')
        subcluster['subcid'] = subcid
        subcluster['hosts'] = []
        subcluster = set_instance_group_config(
            subcluster,
            subcid,
            autoscaling_config,
            resources,
            hosts_count,
            pillar.semantic_version,
        )
        subcluster_spec = HadoopSubclusterSpec(subcluster)
        validate_subcluster(subcluster_spec, zone_id, image_config, pillar.version)  # type: ignore
        check_subclusters_compute_quota([subcluster_spec])
        metadb.add_instance_group_subcluster(cluster_id=cluster['cid'], subcid=subcid, name=name, roles=[role])
    else:
        subcluster_spec = HadoopSubclusterSpec(subcluster)
        validate_subcluster(subcluster_spec, zone_id, image_config, pillar.version)  # type: ignore
        check_subclusters_compute_quota([subcluster_spec])
        subcid, hosts = create_subcluster(cluster, subcluster, subnet_id, zone_id, role, hosts_count)
        subcluster['subcid'] = subcid
        subcluster['hosts'] = [host['fqdn'] for host in hosts]

    # Forward raw resource to pillar for masternode, because she should know sizes for correct settings
    pillar.set_subcluster(subcluster)
    set_pillar_labels(pillar, cluster.get('labels', {}))
    validate_pillar(pillar)
    metadb.update_cluster_pillar(cluster['cid'], pillar)

    return create_operation(
        task_type=HadoopTasks.subcluster_create,
        operation_type=HadoopOperations.subcluster_create,
        metadata=CreateSubclusterMetadata(subcid),
        cid=cluster['cid'],
        task_args={
            'subcid': subcid,
            'image_id': pillar.image,  # using task args to override the image during tests/debugging
        },
    )


def create_subcluster(cluster: dict, subcluster_config: dict, subnet_id: str, zone_id: str, role: str, host_count: int):
    """
    Create Hadoop subcluster
    """

    try:
        spec = HadoopConfigSpec(subcluster_config)
    except ValueError as err:
        raise ParseConfigError(err)

    resources = spec.get_resources()
    flavor = validation.get_flavor_by_name(resources.resource_preset_id)

    assign_public_ip = subcluster_config.get('assign_public_ip', False)

    name = subcluster_config['name']
    host_specs = [{'zone_id': zone_id, 'assign_public_ip': assign_public_ip, 'subnet_id': subnet_id}] * host_count
    subcluster, hosts = clusterutil.create_subcluster_unmanaged(
        cluster_id=cluster['cid'],
        name=name,
        subnet_id=subnet_id,
        roles=[role],
        host_specs=host_specs,
        flavor=flavor,
        volume_size=resources.disk_size,
        disk_type_id=resources.disk_type_id,
        fqdn_mark=FQDN_MARKS[role],
    )
    return subcluster['subcid'], hosts
