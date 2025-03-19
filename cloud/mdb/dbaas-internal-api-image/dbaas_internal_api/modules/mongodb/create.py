# -*- coding: utf-8 -*-
"""
DBaaS Internal API MongoDB cluster creation
"""
from copy import deepcopy
from datetime import timedelta
from typing import Any, Dict, List, Optional, Tuple

from flask import current_app

from . import utils
from ...core.exceptions import DbaasClientError, ParseConfigError
from ...core.types import Operation
from ...utils import metadb, validation
from ...utils.backup_id import generate_backup_id
from ...utils.backups import get_backup_schedule
from ...utils.cluster import create as clusterutil
from ...utils.cluster.get import get_subcluster, get_shards
from ...utils.config import get_bucket_name, get_environment_config
from ...utils.metadata import CreateClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.e2e import is_cluster_for_e2e
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterDescription, Host, RequestedHostResources, Version
from ...utils.version import ensure_version_allowed
from .constants import DEFAULT_SHARD_NAME, MONGOD_HOST_TYPE, MY_CLUSTER_TYPE, SUBCLUSTER_NAMES
from .pillar import MongoDBPillar
from .traits import MongoDBOperations, MongoDBRoles, MongoDBTasks
from .types import MongodbConfigSpec


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CREATE)
def create_cluster_handler(
    name: str,
    environment: str,
    config_spec: dict,
    database_specs: List[dict],
    user_specs: List[dict],
    host_specs: List[dict],
    network_id: str,
    description: str = None,
    labels: dict = None,
    maintenance_window: dict = None,
    security_group_ids: List[str] = None,
    deletion_protection: bool = False,
    **_
) -> Operation:
    # pylint: disable=too-many-arguments,unused-argument,too-many-locals
    """
    Handler for create MongoDB cluster requests.
    """
    config_role = MONGOD_HOST_TYPE
    try:
        spec = MongodbConfigSpec(config_spec)
    except ValueError as err:
        raise ParseConfigError(err)

    resources = spec.get_resources(config_role)
    flavor = validation.get_flavor_by_name(resources.resource_preset_id)

    config = {}  # type: Dict
    if spec.has_config(config_role):
        config = {config_role: spec.get_config(config_role)}
    utils.validate_options(config, spec.version, flavor)

    backup_schedule = get_backup_schedule(MY_CLUSTER_TYPE, spec.backup_start, spec.backup_retain_period)

    base_pillar = MongoDBPillar.make()
    if spec.version is None:
        raise RuntimeError('Version not specified in spec')
    ensure_version_allowed(MY_CLUSTER_TYPE, spec.version)

    base_pillar.feature_compatibility_version = spec.version.to_string(with_suffix=False)
    base_pillar.update_access(spec.access)
    base_pillar.update_config(config)
    base_pillar.add_databases(database_specs)
    base_pillar.add_users(user_specs)
    base_pillar.update_performance_diagnostics(spec.performance_diagnostics)

    cluster, bucket_name = create_cluster(
        description=ClusterDescription(name, environment, description, labels),
        base_pillar=base_pillar,
        host_specs=host_specs,
        resources=resources,
        network_id=network_id,
        flavor=flavor,
        backup_schedule=backup_schedule,
        maintenance_window=maintenance_window,
        security_group_ids=security_group_ids,
        deletion_protection=deletion_protection,
        version=spec.version,
    )

    task_args: dict[str, Any] = {
        's3_buckets': {
            'backup': bucket_name,
        }
    }

    if security_group_ids is not None:
        task_args['security_group_ids'] = security_group_ids

    subcid = get_subcluster(role=MongoDBRoles.mongod, cluster_id=cluster['cid'])['subcid']

    backups = []

    try:
        backups.append(
            {
                "backup_id": generate_backup_id(),
                "subcid": get_subcluster(role=MongoDBRoles.mongocfg, cluster_id=cluster['cid'])['subcid'],
                "shard_id": None,
            }
        )
    except RuntimeError:
        pass

    backups += [
        {
            "backup_id": generate_backup_id(),
            "subcid": subcid,
            "shard_id": shard['shard_id'],
        }
        for shard in get_shards({'cid': cluster['cid']}, role=MongoDBRoles.mongod)
    ]

    task_args['initial_backup_info'] = backups
    task_args['use_backup_service'] = True

    return create_operation(
        task_type=MongoDBTasks.create,
        operation_type=MongoDBOperations.create,
        metadata=CreateClusterMetadata(),
        cid=cluster['cid'],
        task_args=task_args,
        time_limit=timedelta(hours=4),
    )


def create_cluster(
    description: ClusterDescription,
    base_pillar: MongoDBPillar,
    host_specs: List[Host],
    resources: RequestedHostResources,
    flavor: Dict,
    network_id: str,
    backup_schedule: dict,
    version: Version,
    maintenance_window: dict = None,
    security_group_ids: List[str] = None,
    deletion_protection: bool = False,
) -> Tuple[Dict, str]:
    """
    Create MongoDB cluster.
    """
    disk_size = resources.disk_size
    if disk_size is None:
        raise DbaasClientError('Disk size of hosts is not specified')

    host_count = len(host_specs)
    validation.validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=MongoDBRoles.mongod.value,  # pylint: disable=no-member
        resource_preset_id=resources.resource_preset_id,
        disk_type_id=resources.disk_type_id,
        hosts_count=host_count,
    )

    for node in host_specs:
        validation.validate_host_create_resources(
            cluster_type=MY_CLUSTER_TYPE,
            role=MongoDBRoles.mongod,
            resource_preset_id=resources.resource_preset_id,
            geo=node['zone_id'],
            disk_type_id=resources.disk_type_id,
            disk_size=disk_size,
        )

    validation.check_quota(flavor, host_count, disk_size, resources.disk_type_id, new_cluster=True)

    cluster, subnets, private_key = clusterutil.create_cluster(
        cluster_type=MY_CLUSTER_TYPE,
        network_id=network_id,
        description=description,
        maintenance_window=maintenance_window,
        security_group_ids=security_group_ids,
        deletion_protection=deletion_protection,
    )

    env_config = get_environment_config(MY_CLUSTER_TYPE, description.environment)

    bucket_name = get_bucket_name(cluster['cid'])

    pillar = base_pillar
    pillar.cluster_name = description.name
    pillar.zk_hosts = env_config['zk_hosts']
    pillar.s3_bucket = bucket_name
    pillar.set_cluster_private_key(private_key)
    if is_cluster_for_e2e(description.name):
        pillar.set_e2e_cluster()

    metadb.add_cluster_pillar(cluster['cid'], pillar)
    metadb.set_default_versions(
        cid=cluster['cid'],
        subcid=None,
        shard_id=None,
        env=description.environment,
        major_version=version.to_string(with_suffix=False),
        edition=version.edition,
        ctype=MY_CLUSTER_TYPE,
    )

    backup_schedule['use_backup_service'] = True
    metadb.add_backup_schedule(cid=cluster['cid'], backup_schedule=backup_schedule)

    _create_mongod_subcluster_shard(
        cluster=cluster,
        host_specs=host_specs,
        flavor=flavor,
        subnets=subnets,
        disk_size=disk_size,
        disk_type_id=resources.disk_type_id,
    )

    return cluster, bucket_name


def _create_mongod_subcluster_shard(
    cluster: dict,
    host_specs: List[dict],
    flavor: dict,
    subnets: dict,
    disk_size: Optional[int],
    disk_type_id: Optional[str],
) -> None:
    """
    Create MongoDB mongod subcluster
    """
    if disk_size is None:
        raise DbaasClientError('Disk size of MongoDB hosts is not specified')

    mongod_subcluster_name = SUBCLUSTER_NAMES[MONGOD_HOST_TYPE]
    subcluster, _ = clusterutil.create_subcluster(
        cluster_id=cluster['cid'], name=mongod_subcluster_name, subnets=subnets, roles=[MongoDBRoles.mongod]
    )

    pillar = deepcopy(current_app.config['DEFAULT_SUBCLUSTER_PILLAR_TEMPLATE'][mongod_subcluster_name])
    metadb.add_subcluster_pillar(cluster['cid'], subcluster['subcid'], pillar)

    clusterutil.create_shard(
        cid=cluster['cid'],
        subcid=subcluster['subcid'],
        name=DEFAULT_SHARD_NAME,
        flavor=flavor,
        subnets=subnets,
        volume_size=disk_size,
        host_specs=host_specs,
        disk_type_id=disk_type_id,
    )
