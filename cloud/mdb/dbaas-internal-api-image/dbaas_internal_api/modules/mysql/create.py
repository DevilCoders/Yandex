# -*- coding: utf-8 -*-
"""
DBaaS  Internal API MySQL cluster creation
"""

from datetime import timedelta
from typing import cast, List, Tuple, Dict, Any

from dbaas_internal_api.utils.alert_group import create_default_alert_group

from . import utils
from ...core.exceptions import ParseConfigError, ReplicationSourceOnCreateCluster
from ...core.types import CID, Operation
from ...utils import config, metadb, validation
from ...utils.backup_id import generate_backup_id
from ...utils.backups import get_backup_schedule
from ...utils.cluster import create as clusterutil
from ...utils.cluster.get import get_subcluster
from ...utils.config import get_environment_config
from ...utils.compute import validate_host_groups
from ...utils.e2e import is_cluster_for_e2e
from ...utils.feature_flags import ensure_feature_flag
from ...utils.host import collect_zones
from ...utils.metadata import CreateClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ENV_PROD, VTYPE_PORTO, ClusterDescription, RequestedHostResources
from ...utils.version import ensure_version_allowed
from ...utils.zk import choice_zk_hosts
from .constants import MY_CLUSTER_TYPE
from .info import get_cluster_config_set
from .host_pillar import MysqlHostPillar
from .pillar import MySQLPillar, make_new_pillar
from .traits import MySQLOperations, MySQLRoles, MySQLTasks
from .types import MysqlConfigSpec


def create_cluster(
    description: ClusterDescription,
    host_specs: List[dict],
    database_specs: List[dict],
    user_specs: List[dict],
    my_config_spec: MysqlConfigSpec,
    resources: RequestedHostResources,
    flavor: dict,
    network_id: str,
    backup_schedule: dict,
    maintenance_window: dict = None,
    security_group_ids: List[str] = None,
    deletion_protection: bool = False,
    host_group_ids: List[str] = None,
) -> Tuple[CID, MySQLPillar]:
    # pylint: disable=too-many-locals
    """
    Create MySQL cluster database structures
    """

    # Check quota
    num_nodes = len(host_specs)
    validation.validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=MySQLRoles.mysql.value,  # pylint: disable=no-member
        resource_preset_id=resources.resource_preset_id,
        disk_type_id=resources.disk_type_id,
        hosts_count=num_nodes,
    )

    for node in host_specs:
        validation.validate_host_create_resources(
            cluster_type=MY_CLUSTER_TYPE,
            role=MY_CLUSTER_TYPE,
            resource_preset_id=resources.resource_preset_id,
            geo=node['zone_id'],
            disk_type_id=resources.disk_type_id,
            disk_size=resources.disk_size,
        )
        if node.get('replication_source'):
            raise ReplicationSourceOnCreateCluster

    validation.check_quota(flavor, num_nodes, resources.disk_size, resources.disk_type_id, new_cluster=True)

    if host_group_ids:
        ensure_feature_flag("MDB_DEDICATED_HOSTS")
        validate_host_groups(host_group_ids, collect_zones(host_specs), flavor, resources)

    sox_audit = my_config_spec.key(
        'sox_audit',
        flavor['vtype'].lower() == VTYPE_PORTO.lower() and description.environment.lower() == ENV_PROD.lower(),
    )

    validation.validate_sox_flag(flavor['vtype'], description.environment, sox_audit)

    # Create cluster and default pillar
    cluster, subnets, private_key = clusterutil.create_cluster(
        cluster_type=MY_CLUSTER_TYPE,
        network_id=network_id,
        description=description,
        maintenance_window=maintenance_window,
        security_group_ids=security_group_ids,
        deletion_protection=deletion_protection,
        host_group_ids=host_group_ids,
    )
    _, hosts = clusterutil.create_subcluster(
        cluster_id=cluster['cid'],
        name=description.name,
        flavor=flavor,
        volume_size=resources.disk_size,
        host_specs=host_specs,
        roles=[MY_CLUSTER_TYPE],
        subnets=subnets,
        disk_type_id=resources.disk_type_id,
    )

    pillar = make_new_pillar(description.environment)

    mysql_config = my_config_spec.get_config()
    if my_config_spec.version is None:
        raise RuntimeError('Version not specified in spec')
    ensure_version_allowed(MY_CLUSTER_TYPE, my_config_spec.version)
    utils.validate_options(pillar.config.get_config(), mysql_config, my_config_spec.version, flavor, len(hosts))

    pillar.mysql_version = my_config_spec.version.to_string()
    pillar.s3_bucket = config.get_bucket_name(cluster['cid'])
    pillar.set_cluster_private_key(private_key)
    if is_cluster_for_e2e(description.name):
        pillar.set_e2e_cluster()
    pillar.config.merge_config(mysql_config)
    pillar.update_backup_start(backup_schedule['start'])
    pillar.update_access(my_config_spec.access)
    pillar.update_perf_diag(my_config_spec.perf_diag)

    env_conf = get_environment_config(cluster_type=MY_CLUSTER_TYPE, env=description.environment)
    _, zk_hosts = choice_zk_hosts(env_conf['zk'], metadb.get_zk_hosts_usage())
    pillar.zk_hosts = zk_hosts

    config_set = get_cluster_config_set(cluster=cluster, pillar=pillar, flavor=flavor, version=None)
    lower_case_table_names = config_set.effective.get('lower_case_table_names')

    # fill databases
    for db_spec in database_specs:
        pillar.add_database(db_spec, cast(int, lower_case_table_names))

    # Set passwords for system users
    for user_name in pillar.user_names:
        pillar.set_random_password(user_name)

    # fill users
    for user_spec in user_specs:
        pillar.add_user(user_spec, my_config_spec.version)

    pillar.sox_audit = sox_audit

    utils.populate_idm_system_users_if_need(pillar, sox_audit, my_config_spec.version)

    # fill version from default_versions
    metadb.set_default_versions(
        cid=cluster['cid'],
        subcid=None,
        shard_id=None,
        env=description.environment,
        major_version=str(my_config_spec.version),
        edition=my_config_spec.version.edition,
        ctype=MY_CLUSTER_TYPE,
    )

    hosts_with_pillar = [(host, host_specs[i]) for i, host in enumerate(hosts)]

    # host pillars
    hosts_with_pillar.sort(key=lambda h: h[0]['fqdn'])
    pillar.max_server_id = len(hosts)

    for i, (host, host_spec) in enumerate(hosts_with_pillar):
        host_pillar = _create_host_pillar(host_spec, i + 1)
        metadb.add_host_pillar(cluster['cid'], host['fqdn'], host_pillar.as_dict())

    metadb.add_backup_schedule(cluster['cid'], backup_schedule)
    metadb.add_cluster_pillar(cluster['cid'], pillar)

    return cluster['cid'], pillar


def _create_host_pillar(host_spec, server_id) -> MysqlHostPillar:
    host_pillar = MysqlHostPillar({})
    host_pillar.server_id = server_id
    host_pillar.backup_priority = host_spec.get('backup_priority') or 0
    host_pillar.priority = host_spec.get('priority') or 0
    return host_pillar


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CREATE)
def create_mysql_cluster(
    name: str,
    environment: str,
    config_spec: dict,
    database_specs: List[dict],
    user_specs: List[dict],
    host_specs: List[dict],
    network_id: str,
    description: str = None,
    labels: dict = None,  # TODO: handle labels
    maintenance_window: dict = None,
    security_group_ids: List[str] = None,
    deletion_protection: bool = False,
    host_group_ids: List[str] = None,
    alert_group_spec: dict = None,
    **_
) -> Operation:
    # pylint: disable=too-many-arguments,unused-argument
    """
    Create MySQL cluster
    """
    try:
        # Load and validate version
        my_config_spec = MysqlConfigSpec(config_spec, fill_default=True)
    except ValueError as err:
        # `raise` to explicitly tell linters that processing stops here.
        raise ParseConfigError(err)

    resources = my_config_spec.get_resources()
    flavor = validation.get_flavor_by_name(resources.resource_preset_id)

    cid, pillar = create_cluster(
        description=ClusterDescription(
            name=name,
            environment=environment,
            labels=labels,
            description=description,
        ),
        host_specs=host_specs,
        user_specs=user_specs,
        database_specs=database_specs,
        my_config_spec=my_config_spec,
        resources=resources,
        flavor=flavor,
        network_id=network_id,
        backup_schedule=get_backup_schedule(
            cluster_type=MY_CLUSTER_TYPE,
            backup_window_start=my_config_spec.backup_start,
            backup_retain_period=my_config_spec.retain_period,
            use_backup_service=True,
        ),
        maintenance_window=maintenance_window,
        security_group_ids=security_group_ids,
        deletion_protection=deletion_protection,
        host_group_ids=host_group_ids,
    )

    if flavor['cpu_fraction'] != 100:
        time_limit = timedelta(hours=4)
    else:
        time_limit = timedelta(hours=1)

    task_args: Dict[Any, Any] = {
        's3_buckets': {
            'backup': pillar.s3_bucket,
        },
        'zk_hosts': pillar.zk_hosts,
    }

    if alert_group_spec:
        ag_id = create_default_alert_group(cid=cid, alert_group_spec=alert_group_spec)
        task_args["default_alert_group_id"] = ag_id

    if security_group_ids is not None:
        task_args['security_group_ids'] = security_group_ids

    subcid = get_subcluster(role=MySQLRoles.mysql, cluster_id=cid)['subcid']

    backups = [
        {
            "backup_id": generate_backup_id(),
            "subcid": subcid,
            "shard_id": None,
        }
    ]

    task_args['initial_backup_info'] = backups
    task_args['use_backup_service'] = True

    return create_operation(
        task_type=MySQLTasks.create,
        operation_type=MySQLOperations.create,
        metadata=CreateClusterMetadata(),
        time_limit=time_limit,
        cid=cid,
        task_args=task_args,
    )
