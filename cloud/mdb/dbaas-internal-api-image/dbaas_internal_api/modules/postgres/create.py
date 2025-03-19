# -*- coding: utf-8 -*-
"""
DBaaS Internal API postgresql cluster creation
"""

from datetime import timedelta

import psycopg2
import psycopg2.errorcodes as pgcode
from flask import current_app
from typing import Any, List, Tuple, cast

from . import utils
from .api.convertors import database_from_spec, user_from_spec
from .constants import MY_CLUSTER_TYPE, EDITION_1C
from .pillar import PostgresqlClusterPillar, make_new_pillar
from .traits import PostgresqlOperations, PostgresqlRoles, PostgresqlTasks
from .types import Database, PostgresqlConfigSpec, UserWithPassword, ClusterConfig
from .validation import validate_database, validate_databases, validate_user, validate_ha_host_count
from ...core import exceptions as errors
from ...core.types import CID, Operation
from ...utils import config, metadb, validation
from ...utils.alert_group import create_default_alert_group
from ...utils.backup_id import generate_backup_id
from ...utils.backups import get_backup_schedule
from ...utils.cluster import create as clusterutil
from ...utils.cluster.get import get_subcluster
from ...utils.compute import validate_host_groups
from ...utils.config import get_environment_config
from ...utils.e2e import is_cluster_for_e2e
from ...utils.feature_flags import ensure_feature_flag
from ...utils.host import collect_zones
from ...utils.metadata import CreateClusterMetadata
from ...utils.operation_creator import (
    create_operation,
)
from ...utils.register import DbaasOperation
from ...utils.register import Resource as ResourceEnum
from ...utils.register import register_request_handler
from ...utils.types import ClusterDescription, RequestedHostResources, VTYPE_PORTO, ENV_PROD
from ...utils.version import Version, ensure_version_allowed_metadb
from ...utils.zk import choice_zk_hosts

HostSpecs = List[dict]
Users = List[UserWithPassword]
Databases = List[Database]


def _validate_resources(resources: RequestedHostResources, host_specs: HostSpecs) -> None:
    """
    Validate resources
    """

    validation.validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=PostgresqlRoles.postgresql.value,  # pylint: disable=no-member
        resource_preset_id=resources.resource_preset_id,
        disk_type_id=resources.disk_type_id,
        hosts_count=len(host_specs),
    )

    validate_ha_host_count({}, len(host_specs))

    for node in host_specs:
        validation.validate_host_create_resources(
            cluster_type=MY_CLUSTER_TYPE,
            role=MY_CLUSTER_TYPE,
            resource_preset_id=resources.resource_preset_id,
            geo=node['zone_id'],
            disk_type_id=resources.disk_type_id,
            disk_size=resources.disk_size,
        )


def _validate_quota(resources: RequestedHostResources, host_specs: HostSpecs, flavor: dict) -> None:
    """
    Validate quota
    """
    num_nodes = len(host_specs)
    volume_size = resources.disk_size

    utils.validate_nodes_number(num_nodes)

    # Check cloud quota
    validation.check_quota(flavor, num_nodes, volume_size, resources.disk_type_id, new_cluster=True)


def _validate_options(
    pg_config: ClusterConfig,
    databases: Databases,
    users: Users,
    flavor: dict,
    resources: RequestedHostResources,
    is_restore: bool,
    metadb_default_versions: dict,
) -> None:
    """
    Validate version, databases, users and options
    """
    if not is_restore:
        # restore from deprecated version should works
        ensure_version_allowed_metadb(MY_CLUSTER_TYPE, pg_config[0], metadb_default_versions)
    for database in databases:
        validate_database(database, users, pg_config)
    for user in users:
        validate_user(user, databases)
    validate_databases(databases)

    users_conns = utils.get_sum_user_conns(users)
    system_conns = utils.get_system_conn_limit()

    utils.validate_mem_options(
        options=pg_config.db_options,
        sum_user_conns=users_conns + system_conns,
        flavor=flavor,
    )
    if resources.disk_size is None:
        # That case shouldn't happen, but MYPY doesn't know about it.
        # Probably, here we should create another HostResources type `disk_size: int` (without Optional)
        raise RuntimeError('Unexpected: resources.disk_size is None')
    utils.validate_disk_options(options=pg_config.db_options, disk_size=resources.disk_size)


def _fill_db_options_pillar(
    cluster_pillar: PostgresqlClusterPillar, description: ClusterDescription, pg_config: ClusterConfig
) -> None:
    """
    Fill db options.
    """

    env_conf = get_environment_config(cluster_type=MY_CLUSTER_TYPE, env=description.environment)
    zk_id, zk_hosts = choice_zk_hosts(env_conf['zk'], metadb.get_zk_hosts_usage())
    cluster_pillar.pgsync.set_zk_id_and_hosts(zk_id, zk_hosts)
    cluster_pillar.pgsync.set_autofailover(pg_config.autofailover)
    cluster_pillar.update_backup_start(pg_config.backup_schedule['start'])
    cluster_pillar.update_access(pg_config.access)

    # Fill postgre options
    cluster_pillar.config.update_config(pg_config.db_options)
    cluster_pillar.config.update_config(pg_config.pooler_options)
    cluster_pillar.perf_diag.update(pg_config.perf_diag)

    # Set some pillar options for 1C version. Need to be removed when we can
    # have default pillar per major version.
    if pg_config.version.edition == EDITION_1C:
        cluster_pillar.config.update_config(
            {
                'join_collapse_limit': 20,
                'from_collapse_limit': 20,
                'max_locks_per_transaction': 150,
            }
        )


def _fill_unmanaged_dbs_pillar(cluster_pillar: PostgresqlClusterPillar, databases: Databases) -> None:
    """
    Fill unmanaged dbs pillar
    """
    for db in databases:
        cluster_pillar.databases.add_database_initial(db)


def _fill_system_users_in_pillar(cluster_pillar: PostgresqlClusterPillar) -> None:
    """
    Fill passwords for system users in pillar
    """

    # Create passwords for system users
    for name in cluster_pillar.pgusers.get_names():
        cluster_pillar.pgusers.set_random_password(name)


def _fill_requested_users_in_pillar(cluster_pillar: PostgresqlClusterPillar, users: Users) -> None:
    """
    Fill users in pillar
    """
    name_to_grants = {u.name: u.grants for u in users}

    def _get_user_order(username):
        return sum([_get_user_order(x) for x in name_to_grants.get(username, [])]) + 1

    for user in sorted(users, key=lambda x: _get_user_order(x.name)):
        cluster_pillar.pgusers.add_user(user)


def _create_in_metadb(
    cluster_pillar: PostgresqlClusterPillar,
    description: ClusterDescription,
    flavor: dict,
    host_specs: HostSpecs,
    resources: RequestedHostResources,
    network_id: str,
    backup_schedule: dict,
    pg_version: Version,
    maintenance_window: dict = None,
    monitoring_cloud_id: str = None,
    security_group_ids: List[str] = None,
    deletion_protection: bool = False,
    host_group_ids: List[str] = None,
) -> CID:
    # pylint: disable=too-many-locals
    """
    Create cluster in metadb

    ... and update pillar
    """

    cluster_dict, subnets, private_key = clusterutil.create_cluster(
        cluster_type=MY_CLUSTER_TYPE,
        network_id=network_id,
        description=description,
        maintenance_window=maintenance_window,
        monitoring_cloud_id=monitoring_cloud_id,
        security_group_ids=security_group_ids,
        deletion_protection=deletion_protection,
        host_group_ids=host_group_ids,
    )
    cid = cluster_dict['cid']
    _, hosts = clusterutil.create_subcluster(
        cluster_id=cid,
        name=description.name,
        flavor=flavor,
        volume_size=cast(int, resources.disk_size),
        host_specs=host_specs,
        subnets=subnets,
        roles=[MY_CLUSTER_TYPE],
        disk_type_id=resources.disk_type_id,
    )

    # Here we need to redefine data:s3_bucket in pillar
    # because only here we know cluster id
    cluster_pillar.s3_bucket = config.get_bucket_name(cid)
    cluster_pillar.set_cluster_private_key(private_key)
    if is_cluster_for_e2e(description.name):
        cluster_pillar.set_e2e_cluster()

    try:
        metadb.add_cluster_pillar(cid, cluster_pillar)
    except psycopg2.IntegrityError as error:
        # We could fail here on integrity error
        # Due to non-unique database name
        if error.pgcode == pgcode.UNIQUE_VIOLATION:
            db_list = [x.name for x in cluster_pillar.databases.get_databases()]
            raise errors.PgDatabasesAlreadyExistError(db_list) from error
        raise error

    metadb.add_backup_schedule(cid, backup_schedule)

    metadb.set_default_versions(
        cid=cid,
        subcid=None,
        shard_id=None,
        env=description.environment,
        major_version=str(pg_version.major),
        edition=pg_version.edition,
        ctype=MY_CLUSTER_TYPE,
    )

    for index, host_spec in enumerate(host_specs):
        host_name = hosts[index]['fqdn']
        if host_spec.get('replication_source'):
            raise errors.ReplicationSourceOnCreateCluster

        if 'priority' not in host_spec:
            host_spec['priority'] = current_app.config['ZONE_ID_TO_PRIORITY'].get(host_spec['zone_id'], 0)

        host_pillar = utils.host_spec_make_pillar(host_spec).as_dict()

        metadb.add_host_pillar(cid, host_name, host_pillar)

    return cid


def create_cluster(
    description: ClusterDescription,
    host_specs: List[dict],
    databases: List[Database],
    users: List[UserWithPassword],
    pg_config: ClusterConfig,
    resources: RequestedHostResources,
    flavor: dict,
    network_id: str,
    maintenance_window: dict = None,
    security_group_ids: List[str] = None,
    monitoring_cloud_id: str = None,
    deletion_protection: bool = False,
    host_group_ids: List[str] = None,
    sox_audit: bool = False,
    walg_options: dict = None,
    is_restore: bool = False,
) -> Tuple[CID, PostgresqlClusterPillar]:
    """
    Create postgresql sql cluster
    """

    _validate_resources(resources=resources, host_specs=host_specs)
    _validate_quota(resources=resources, host_specs=host_specs, flavor=flavor)
    metadb_default_versions = metadb.get_default_versions(
        component='postgres', env=description.environment, type=MY_CLUSTER_TYPE
    )
    _validate_options(
        pg_config=pg_config,
        databases=databases,
        users=users,
        flavor=flavor,
        resources=resources,
        is_restore=is_restore,
        metadb_default_versions=metadb_default_versions,
    )
    validation.validate_sox_flag(flavor['vtype'], description.environment, sox_audit)

    if host_group_ids:
        ensure_feature_flag("MDB_DEDICATED_HOSTS")
        validate_host_groups(host_group_ids, collect_zones(host_specs), flavor, resources)

    cluster_pillar = make_new_pillar()
    _fill_db_options_pillar(cluster_pillar, description, pg_config)
    _fill_unmanaged_dbs_pillar(cluster_pillar, databases)
    _fill_system_users_in_pillar(cluster_pillar)
    _fill_requested_users_in_pillar(cluster_pillar, users)
    if walg_options:
        cluster_pillar.walg.update(walg_options)
    if sox_audit:
        cluster_pillar.sox_audit = sox_audit

    cid = _create_in_metadb(
        cluster_pillar=cluster_pillar,
        description=description,
        flavor=flavor,
        host_specs=host_specs,
        network_id=network_id,
        resources=resources,
        backup_schedule=pg_config.backup_schedule,
        pg_version=pg_config.version,
        maintenance_window=maintenance_window,
        monitoring_cloud_id=monitoring_cloud_id,
        security_group_ids=security_group_ids,
        deletion_protection=deletion_protection,
        host_group_ids=host_group_ids,
    )
    return cid, cluster_pillar


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.CLUSTER, DbaasOperation.CREATE)
def create_postgresql_cluster(
    # pylint: disable=unused-argument
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
    monitoring_cloud_id: str = None,
    monitoring_stub_request_id: str = None,
    deletion_protection: bool = False,
    host_group_ids: List[str] = None,
    alert_group_spec: dict = None,
    **_,
) -> Operation:
    # pylint: disable=too-many-arguments,too-many-locals
    """
    Creates postgresql cluster. Returns task for worker
    """
    try:
        # Load and validate version
        pg_config_spec = PostgresqlConfigSpec(config_spec)
    except ValueError as err:
        raise errors.ParseConfigError(err)

    databases = [database_from_spec(s) for s in database_specs]

    users = [user_from_spec(u, databases) for u in user_specs]

    resources = pg_config_spec.get_resources()

    flavor = validation.get_flavor_by_name(resources.resource_preset_id)

    config_options = pg_config_spec.get_config()

    if config_options.get('wal_level') is not None:
        raise errors.DbaasClientError('walLevel is deprecated and no longer supported')

    cid, cluster_pillar = create_cluster(
        description=ClusterDescription(name=name, environment=environment, description=description, labels=labels),
        host_specs=host_specs,
        databases=databases,
        users=users,
        pg_config=ClusterConfig(
            version=pg_config_spec.get_version_strict(),
            pooler_options=pg_config_spec.key('pooler_config'),
            db_options=config_options,
            autofailover=pg_config_spec.key('autofailover', True),
            backup_schedule=get_backup_schedule(
                cluster_type=MY_CLUSTER_TYPE,
                backup_window_start=pg_config_spec.backup_start,
                backup_retain_period=pg_config_spec.retain_period,
                use_backup_service=True,
                max_incremental_steps=pg_config_spec.max_incremental_steps,
            ),
            access=pg_config_spec.key('access'),
            perf_diag=pg_config_spec.key('perf_diag'),
        ),
        resources=resources,
        flavor=flavor,
        network_id=network_id,
        maintenance_window=maintenance_window,
        security_group_ids=security_group_ids,
        monitoring_cloud_id=monitoring_cloud_id,
        deletion_protection=deletion_protection,
        host_group_ids=host_group_ids,
        sox_audit=pg_config_spec.key(
            'sox_audit',
            flavor['vtype'].lower() == VTYPE_PORTO.lower() and environment.lower() == ENV_PROD.lower(),
        ),
    )

    task_args: dict[str, Any] = {'s3_buckets': {'backup': cluster_pillar.s3_bucket}}
    if security_group_ids is not None:
        task_args['security_group_ids'] = security_group_ids

    if flavor['cpu_fraction'] != 100:
        time_limit = timedelta(hours=4)
    else:
        time_limit = timedelta(hours=1)

    if alert_group_spec:
        ag_id = create_default_alert_group(cid=cid, alert_group_spec=alert_group_spec)
        task_args["default_alert_group_id"] = ag_id

    subcid = get_subcluster(role=PostgresqlRoles.postgresql, cluster_id=cid)['subcid']

    backups = [
        {
            "backup_id": generate_backup_id(),
            "subcid": subcid,
            "shard_id": None,
        }
    ]

    task_args['initial_backup_info'] = backups
    task_args['use_backup_service'] = True

    if monitoring_stub_request_id is not None:
        task_args['monitoring_stub_request_id'] = monitoring_stub_request_id

    return create_operation(
        task_type=PostgresqlTasks.create,
        operation_type=PostgresqlOperations.create,
        metadata=CreateClusterMetadata(),
        time_limit=time_limit,
        cid=cid,
        task_args=task_args,
    )
