# -*- coding: utf-8 -*-
"""
DBaaS Internal API ClickHouse cluster creation
"""

from copy import deepcopy
from datetime import timedelta
from itertools import groupby, cycle
from typing import Any, List, Optional, Tuple

from flask import current_app

from ...core.exceptions import (
    DbaasClientError,
    ParseConfigError,
)
from ...core.types import Operation
from ...core.base_pillar import set_e2e_cluster
from ...utils import metadb, pillar
from ...utils.backups import get_backup_schedule
from ...utils.cluster import create as clusterutil
from ...utils.cluster.create import create_shard, create_subcluster
from ...utils.config import get_bucket_name
from ...utils.e2e import is_cluster_for_e2e
from ...utils.helpers import merge_dict
from ...utils.metadata import CreateClusterMetadata
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.feature_flags import ensure_feature_flag, ensure_no_feature_flag, has_feature_flag
from ...utils.types import ClusterDescription, ExistedHostResources, Host, RequestedHostResources, VTYPE_COMPUTE
from ...utils.validation import (
    QuotaValidator,
    get_flavor_by_name,
    validate_hosts,
    validate_service_account,
)
from ...utils.version import Version, ensure_version_allowed, version_validator, get_default_version, get_full_version
from .constants import CH_SUBCLUSTER, DEFAULT_SHARD_NAME, MY_CLUSTER_TYPE, CH_PACKAGE_NAME
from .pillar import ClickhousePillar, ClickhouseShardPillar
from .traits import ClickhouseOperations, ClickhouseRoles, ClickhouseTasks
from .utils import (
    ClickhouseConfigSpec,
    create_operation,
    ensure_version_supported_for_clickhouse_keeper,
    ensure_version_supported_for_cloud_storage,
    get_zk_zones,
    process_user_spec,
    split_host_specs,
    validate_db_name,
    validate_user_quotas,
    validate_zk_flavor,
)
from .zookeeper import create_zk_subcluster, get_default_zk_resources, ensure_zk_hosts_configured


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
    shard_name: str = None,
    service_account_id: str = None,
    maintenance_window: dict = None,
    security_group_ids: List[str] = None,
    deletion_protection: bool = False,
    **_,
) -> Operation:
    # pylint: disable=too-many-arguments,too-many-locals
    """
    Handler for create ClickHouse cluster requests.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_CREATE_CLUSTER_API')

    try:
        spec = ClickhouseConfigSpec(config_spec)
    except ValueError as exc:
        raise ParseConfigError(exc)

    ch_resources, zk_resources = spec.resources
    if service_account_id is not None:
        if get_flavor_by_name(ch_resources.resource_preset_id)['vtype'] != VTYPE_COMPUTE:
            raise DbaasClientError('Service Accounts not supported in porto clusters')
        else:
            validate_service_account(service_account_id)

    version = spec.version or get_default_version(MY_CLUSTER_TYPE)

    if not has_feature_flag('MDB_CLICKHOUSE_TESTING_VERSIONS'):
        ensure_version_allowed(MY_CLUSTER_TYPE, version)
    else:
        try:
            get_full_version(MY_CLUSTER_TYPE, version)
        except RuntimeError:
            version_validator().ensure_version_exists(CH_PACKAGE_NAME, version)

    backup_schedule = get_backup_schedule(MY_CLUSTER_TYPE, spec.backup_start)
    if 'use_backup_service' not in backup_schedule:
        backup_schedule['use_backup_service'] = False

    cluster, s3_buckets = create_cluster(
        description=ClusterDescription(name, environment, description, labels),
        ch_resources=ch_resources,
        zk_resources=zk_resources,
        host_specs=host_specs,
        network_id=network_id,
        shard_name=shard_name,
        backup_schedule=backup_schedule,
        maintenance_window=maintenance_window,
        ch_pillar=_make_pillar(
            config_spec=spec,
            database_specs=database_specs,
            user_specs=user_specs,
            version=version,
            backup_start=backup_schedule['start'],
            service_account_id=service_account_id,
        ),
        security_group_ids=security_group_ids,
        deletion_protection=deletion_protection,
    )

    return _create_task(
        cluster, s3_buckets, service_account_id, security_group_ids, bool(spec.config.get('geobase_uri'))
    )


def create_cluster(
    description: ClusterDescription,
    ch_pillar: ClickhousePillar,
    host_specs: List[Host],
    network_id: str,
    ch_resources: RequestedHostResources,
    zk_resources: RequestedHostResources,
    shard_name: Optional[str],
    backup_schedule: dict,
    maintenance_window: dict = None,
    security_group_ids: List[str] = None,
    deletion_protection: bool = False,
) -> Tuple[dict, dict]:
    # pylint: disable=too-many-locals
    """
    Create ClickHouse cluster.
    """
    ch_host_specs, zk_host_specs = split_host_specs(host_specs)
    ch_flavor = get_flavor_by_name(ch_resources.resource_preset_id)
    ch_cores = ch_flavor['cpu_limit'] * len(ch_host_specs)
    zk_zones = get_zk_zones(zk_host_specs)

    merged_zk_resources = get_default_zk_resources(ch_cores, zk_zones).update(zk_resources)

    zk_flavor = get_flavor_by_name(merged_zk_resources.resource_preset_id)

    ch_host_specs, zk_host_specs = process_host_specs(
        ch_host_specs, zk_host_specs, ch_resources, merged_zk_resources, ch_pillar.embedded_keeper
    )

    if ch_pillar.cloud_storage_enabled:
        ensure_version_supported_for_cloud_storage(ch_pillar.version)

    if ch_pillar.embedded_keeper:
        ensure_version_supported_for_clickhouse_keeper(ch_pillar.version)

    validator = QuotaValidator()
    validator.add(ch_flavor, len(ch_host_specs), ch_resources.disk_size, ch_resources.disk_type_id)
    if zk_host_specs:
        validator.add(zk_flavor, len(zk_host_specs), merged_zk_resources.disk_size, merged_zk_resources.disk_type_id)
    validator.validate()

    cluster, subnets, s3_buckets = _create_cluster(
        network_id,
        description,
        maintenance_window=maintenance_window,
        security_group_ids=security_group_ids,
        deletion_protection=deletion_protection,
    )

    if ch_pillar.cloud_storage_enabled:
        cloud_storage_bucket = 'cloud-storage-' + cluster['cid']
        ch_pillar.set_cloud_storage_bucket(cloud_storage_bucket)
        s3_buckets['cloud_storage'] = cloud_storage_bucket

    metadb.add_backup_schedule(cluster['cid'], backup_schedule)

    if zk_host_specs:
        validate_zk_flavor(ch_cores, zk_flavor)
        create_zk_subcluster(
            cluster=cluster,
            host_specs=zk_host_specs,
            flavor=zk_flavor,
            subnets=subnets,
            disk_size=merged_zk_resources.disk_size,
            disk_type_id=merged_zk_resources.disk_type_id,
        )

    _create_ch_subcluster(
        cluster=cluster,
        host_specs=ch_host_specs,
        flavor=ch_flavor,
        subnets=subnets,
        disk_size=ch_resources.disk_size,
        disk_type_id=ch_resources.disk_type_id,
        shard_name=shard_name,
        ch_pillar=ch_pillar,
    )

    return cluster, s3_buckets


def process_host_specs(
    ch_host_specs: List[dict],
    zk_host_specs: List[dict],
    ch_resources: RequestedHostResources,
    zk_resources: ExistedHostResources,
    embedded_keeper: bool,
) -> Tuple[List[dict], List[dict]]:
    """
    Process ClickHouse and ZooKeeper host specs: validation and substitution of default values.
    """
    if ch_resources.disk_size is None:
        raise DbaasClientError('Disk size of ClickHouse hosts is not specified')
    validate_hosts(ch_host_specs, ch_resources, ClickhouseRoles.clickhouse.value, MY_CLUSTER_TYPE)
    if embedded_keeper:
        ensure_feature_flag('MDB_CLICKHOUSE_KEEPER')
        if zk_host_specs:
            raise DbaasClientError('You cannot use Zookeper with enabled Embedded Keeper')
        if len(ch_host_specs) == 2:
            raise DbaasClientError('You cannot use 2-node setup with enabled Embedded Keeper')

    elif len(ch_host_specs) > 1:
        preferred_geo = set(h['zone_id'] for h in ch_host_specs)
        zk_host_specs = ensure_zk_hosts_configured(zk_host_specs, preferred_geo)

    if zk_host_specs:
        validate_hosts(
            zk_host_specs, zk_resources.to_requested_host_resources(), ClickhouseRoles.zookeeper.value, MY_CLUSTER_TYPE
        )
    return ch_host_specs, zk_host_specs


def _create_cluster(
    network_id,
    description: ClusterDescription,
    maintenance_window: dict = None,
    security_group_ids: List[str] = None,
    deletion_protection: bool = False,
):
    cluster, subnets, private_key = clusterutil.create_cluster(
        cluster_type=MY_CLUSTER_TYPE,
        network_id=network_id,
        description=description,
        maintenance_window=maintenance_window,
        security_group_ids=security_group_ids,
        deletion_protection=deletion_protection,
    )

    bucket_name = get_bucket_name(cluster['cid'])

    cluster_pillar = deepcopy(current_app.config['DEFAULT_PILLAR_TEMPLATE'][MY_CLUSTER_TYPE])
    pillar.set_s3_bucket(cluster_pillar, bucket_name)
    pillar.add_cluster_private_key(cluster_pillar, private_key)
    merge_dict(cluster_pillar, {'data': {'unmanaged': {'enable_zk_tls': True}}})
    if is_cluster_for_e2e(description.name):
        set_e2e_cluster(cluster_pillar)
    metadb.add_cluster_pillar(cluster['cid'], cluster_pillar)

    return cluster, subnets, {"backup": bucket_name}


def _create_ch_subcluster(
    cluster: dict,
    host_specs: List[dict],
    flavor: dict,
    subnets: dict,
    disk_size: Optional[int],
    disk_type_id: Optional[str],
    shard_name: Optional[str],
    ch_pillar: ClickhousePillar,
) -> None:
    """
    Create ClickHouse subcluster
    """
    subcluster, *_ = create_subcluster(
        cluster_id=cluster['cid'], name=CH_SUBCLUSTER, roles=[ClickhouseRoles.clickhouse]
    )

    shard_pillar = ClickhouseShardPillar.from_config(ch_pillar.config)
    shard, hosts = create_shard(
        cid=cluster['cid'],
        subcid=subcluster['subcid'],
        name=(shard_name or DEFAULT_SHARD_NAME),
        flavor=flavor,
        subnets=subnets,
        volume_size=disk_size,  # type: ignore
        host_specs=host_specs,
        disk_type_id=disk_type_id,
        pillar=shard_pillar,
    )

    if ch_pillar.embedded_keeper:
        """
        Try to pick ZK hosts from different locations
        """
        zk_host_count = 3 if len(hosts) >= 3 else 1

        def geo_grouper(h):
            return h['geo']

        grouped_by_geo = {
            key: iter(list(value)) for key, value in groupby(sorted(hosts, key=geo_grouper), key=geo_grouper)
        }
        zk_hosts = {}
        server_id = 1
        for key in cycle(grouped_by_geo.keys()):
            host = next(grouped_by_geo[key], None)
            if host is not None:
                zk_hosts[host['fqdn']] = server_id
                server_id += 1
            if len(zk_hosts) >= zk_host_count:
                break
        ch_pillar.keeper_hosts = zk_hosts

    ch_pillar.add_shard(shard['shard_id'])
    metadb.add_subcluster_pillar(cluster['cid'], subcluster['subcid'], ch_pillar)


def _make_pillar(
    config_spec: ClickhouseConfigSpec,
    database_specs: List[dict],
    user_specs: List[dict],
    version: Version,
    backup_start: dict,
    service_account_id: Optional[str],
) -> ClickhousePillar:
    db_names = [x['name'] for x in database_specs]
    for db_name in db_names:
        validate_db_name(db_name)

    if config_spec.sql_user_management:
        if version < Version(20, 6):
            raise DbaasClientError('SQL user management is not supported in versions lower than 20.6.')
        if not config_spec.admin_password:
            raise DbaasClientError('Admin password must be specified in order to enable SQL user management.')
        if len(user_specs) != 0:
            raise DbaasClientError('Users to create cannot be specified when SQL user management is enabled.')
    else:
        if len(user_specs) == 0:
            raise DbaasClientError('userSpecs: Shorter than minimum length 1.')

    for user in user_specs:
        process_user_spec(user, db_names)
        validate_user_quotas(user.get('quotas'))

    if config_spec.sql_database_management:
        if not config_spec.sql_user_management:
            raise DbaasClientError('SQL database management is not supported without SQL user management.')
        if len(db_names) > 0:
            raise DbaasClientError('Databases to create cannot be specified when SQL database management is enabled.')
    else:
        if len(db_names) == 0:
            raise DbaasClientError('databaseSpecs: Shorter than minimum length 1.')

    ch_pillar = ClickhousePillar.make()
    ch_pillar.add_databases(database_specs)
    ch_pillar.add_users(user_specs)
    ch_pillar.set_version(version)
    ch_pillar.add_zk_users()
    ch_pillar.add_system_users()
    ch_pillar.update_config(config_spec.config)
    ch_pillar.update_backup_start(backup_start)
    ch_pillar.update_access(config_spec.access)
    ch_pillar.update_service_account_id(service_account_id)  # type: ignore
    ch_pillar.update_cloud_storage(config_spec.cloud_storage)
    ch_pillar.update_embedded_keeper(bool(config_spec.embedded_keeper))
    ch_pillar.update_sql_user_management(bool(config_spec.sql_user_management))
    ch_pillar.update_sql_database_management(bool(config_spec.sql_database_management))
    ch_pillar.update_mysql_protocol(bool(config_spec.mysql_protocol))
    ch_pillar.update_postgresql_protocol(bool(config_spec.postgresql_protocol))
    ch_pillar.update_admin_password(config_spec.admin_password)
    ch_pillar.set_user_management_v2(version >= Version(20, 6))
    ch_pillar.set_testing_repos(has_feature_flag('MDB_CLICKHOUSE_TESTING_VERSIONS'))
    return ch_pillar


def _create_task(
    cluster, s3_buckets: dict, service_account_id: Optional[str], security_group_ids, update_geobase: bool
):
    task_args: dict[str, Any] = {
        's3_buckets': s3_buckets,
        'service_account_id': service_account_id,
        'update-geobase': update_geobase,
    }
    if security_group_ids is not None:
        task_args['security_group_ids'] = security_group_ids

    return create_operation(
        task_type=ClickhouseTasks.create,
        operation_type=ClickhouseOperations.create,
        metadata=CreateClusterMetadata(),
        cid=cluster['cid'],
        task_args=task_args,
        time_limit=timedelta(hours=3),
    )
