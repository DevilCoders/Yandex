# -*- coding: utf-8 -*-
"""
DBaaS Internal API MongoDB cluster utils
"""
import json
import math
from copy import deepcopy
from datetime import timedelta
from typing import Any, List, Optional, Tuple

from flask import current_app
from flask_restful import abort

from ...core.exceptions import (
    DatabasePermissionsError,
    DbaasClientError,
    DbaasNotImplementedError,
    ParseConfigError,
    PreconditionFailedError,
)
from ...utils import metadb
from ...utils.cluster import create as clusterutil
from ...utils.feature_flags import ensure_feature_flag
from ...utils.helpers import get_value_by_path
from ...utils.infra import filter_cluster_resources, get_resources
from ...utils.network import get_network, get_subnets
from ...utils.register import get_cluster_traits, get_config_schema
from ...utils.renderers import render_config_set
from ...utils.types import Host, Pillar, ShardedBackup, Version
from ...utils.version import ensure_version_allowed
from .constants import (
    DB_ROLE_MAP,
    DEFAULT_ROLES,
    EDITION_ENTERPRISE,
    MIN_MONGOCFG_HOSTS_COUNT,
    MIN_MONGOS_HOSTS_COUNT,
    MONGOCFG_HOST_TYPE,
    MONGOINFRA_HOST_TYPE,
    MONGOS_HOST_TYPE,
    MY_CLUSTER_TYPE,
    SUBCLUSTER_NAMES,
    SYSTEM_DATABASES,
    UPGRADE_PATHS,
    VERSIONS_SUPPORTS_PITR,
)
from .pillar import MongoDBPillar
from .traits import MongoDBRoles

RECOVERY_TIMESTAMP_ROUND = timedelta(seconds=1)


def validate_db_roles(data: dict) -> None:
    """Validates combination of DB and roles"""
    dbname = data.get('database_name')
    roles = data.get('roles')
    # Roles were not provided: later in code we set default perms to readWrite.
    # Check if this DB is not among a list of system databases to prevent an elevated access.
    if roles is None:
        if dbname in SYSTEM_DATABASES:
            raise DatabasePermissionsError(dbname, ','.join(DEFAULT_ROLES))
        return

    # Some system DBs can be assigned a read-only role, e.g. mdbMonitor.
    if dbname in DB_ROLE_MAP:
        for role in roles:
            if role not in DB_ROLE_MAP[dbname]:  # type: ignore
                raise DatabasePermissionsError(dbname, role)
            elif DB_ROLE_MAP[dbname][role] is not True:  # type: ignore
                ensure_feature_flag(DB_ROLE_MAP[dbname][role])  # type: ignore
    # `*` stands for "any other database".
    else:
        for role in roles:
            if role not in DB_ROLE_MAP['*']:  # type: ignore
                raise DatabasePermissionsError(dbname, role)
            elif DB_ROLE_MAP['*'][role] is not True:  # type: ignore
                ensure_feature_flag(DB_ROLE_MAP['*'][role])  # type: ignore


def pillar_get_version(pillar: Pillar) -> str:
    """
    Get Mongo version (in human readable format) from pillar
    """
    return pillar['data']['mongodb']['version']['major_human']


def validate_mongod_wt_cache_size(wt_cache_size, flavor):
    """
    Validate wired tiger engine cache size setting
    """
    min_size = 0.25
    max_size = 0.9 * flavor['memory_guarantee'] / math.pow(1024, 3)

    if max_size < wt_cache_size or wt_cache_size < min_size:
        abort(
            422,
            message='Invalid mongod setting for you instance type: '
            '\'storage.wiredTiger.engineConfig.cacheSizeGB\' '
            'must be between {0} and {1}'.format(min_size, max_size),
        )


def validate_mongod_max_incoming_connections(max_inc_conn, flavor):
    # pylint: disable=invalid-name
    """
    Validate max incoming connections setting
    """
    min_count = 10
    max_count = int(flavor['memory_guarantee'] / math.pow(1024, 2))

    if max_count < max_inc_conn or max_inc_conn < min_count:
        abort(
            422,
            message='Invalid mongod setting for your instance type: '
            '\'net.maxIncomingConnections\' must be between {0} and {1}'.format(min_count, max_count),
        )


def get_wt_cache_size(mongod_options):
    """
    Extract wt cache size from options
    """
    return mongod_options.get('storage', {}).get('wiredTiger', {}).get('engineConfig', {}).get('cacheSizeGB')


def get_max_conns(mongod_options):
    """
    Extract max incoming connection from options
    """
    return mongod_options.get('net', {}).get('maxIncomingConnections')


def get_audit_filter(mongod_options):
    """
    Extract audit log filter from options
    """
    return mongod_options.get('auditLog', {}).get('filter')


def get_audit_runtime_configuration(mongod_options):
    """
    Extract audit runtimeConfiguration from options
    """
    return mongod_options.get('auditLog', {}).get('runtimeConfiguration')


def validate_options(options, version, flavor):
    # pylint: disable=unused-argument
    """
    Validate MongoDB cluster options (mongod, mongos, mongocfg)

    # TODO: move to schema validate
    """
    if 'mongod' in options:
        mongod_validate_options(options['mongod'], flavor)


def mongod_validate_options(options, flavor):
    """
    Validate mongod options

    # TODO: move to schema validate (mb jsonschema)
    """
    wt_cache_size = get_wt_cache_size(options)
    if wt_cache_size is not None:
        validate_mongod_wt_cache_size(wt_cache_size, flavor)

    max_incoming_connections = get_max_conns(options)
    if max_incoming_connections is not None:
        validate_mongod_max_incoming_connections(max_incoming_connections, flavor)

    audit_filter = get_audit_filter(options)
    if audit_filter is not None:
        validate_audit_filter(audit_filter)

    if get_audit_runtime_configuration(options) is not None:
        raise DbaasNotImplementedError('Runtime audit configuration is not supported yet')


def validate_audit_filter(audit_filter: str) -> None:
    """
    Validate audit filter
    """
    try:
        json.loads(audit_filter)
    except json.JSONDecodeError:
        raise ParseConfigError('Invalid auditLog.filter value, json expected: {0}'.format(audit_filter))


def validate_fcv(fcv: str, version: Version) -> None:
    """
    Validate featureCompatibilityVersion
    """
    traits_fcv = getattr(get_cluster_traits(MY_CLUSTER_TYPE), 'feature_compatibility_versions')
    valid_fcv = traits_fcv.get(version.to_string())
    if fcv not in valid_fcv:
        raise ParseConfigError(
            'Invalid feature compatibility version for version {0}, allowed values: {1}'.format(
                version, ', '.join(valid_fcv)
            )
        )


def update_fcvs(console_data: dict):
    """
    Get featureCompatibilityVersion options for console
    """
    traits_fcv = getattr(get_cluster_traits(MY_CLUSTER_TYPE), 'feature_compatibility_versions')
    console_data['available_fcvs'] = [{'id': _id, 'fcvs': fcvs} for _id, fcvs in traits_fcv.items()]


def validate_version_change(cid: str, pillar: MongoDBPillar, new_version: Version, current_version: Version) -> None:
    """
    Validate version change
    """

    if current_version > new_version:
        raise PreconditionFailedError('Version downgrade detected')

    version_path = UPGRADE_PATHS.get(new_version.to_string())
    if not version_path:
        raise PreconditionFailedError('Invalid version to upgrade to: {new}'.format(new=new_version))

    if current_version.to_string() not in version_path['from']:
        raise PreconditionFailedError(
            'Upgrade from {old} to {new} is not allowed'.format(old=current_version, new=new_version)
        )

    feature_flag = version_path.get('feature_flag')
    if feature_flag is not None:
        ensure_feature_flag(feature_flag)

    fcv = pillar.feature_compatibility_version
    if fcv != current_version.to_string(with_suffix=False):
        raise PreconditionFailedError(
            'Current feature compatibility version {0} is invalid for upgrade to version {1}, expected: {2}'.format(
                fcv, new_version, current_version
            )
        )

    assert_deprecated_config(pillar.config, version_path.get('deprecated_config_values'))


def assert_deprecated_config(config, deprecated_config):
    """
    Check config for deprecated items
    """
    if not deprecated_config:
        return

    deprecated_items = []
    for opt, deprecated_values in deprecated_config.items():
        current_value = get_value_by_path(config, opt.split('.'))
        if current_value in deprecated_values:
            deprecated_items.append('{opt} = {value}'.format(opt=opt, value=current_value))
    if deprecated_items:
        raise PreconditionFailedError(
            'Version upgrade is not possible due to conflicting options: {options}'.format(
                options=', '.join(deprecated_items)
            )
        )


def build_cluster_config_set(cluster: dict, pillar: MongoDBPillar, default_pillar: dict, flavors, version=None) -> dict:
    """
    Assemble MongoDB config object.
    """

    if version is None:
        version = get_cluster_version(cluster['cid'], pillar)

    config: dict[str, Any] = {}
    for srv, subcluster_name in SUBCLUSTER_NAMES.items():
        if flavors:
            resources = filter_cluster_resources(cluster, MongoDBRoles[srv])
        else:
            resources = get_resources(cluster['cid'], MongoDBRoles[srv])
        if not resources:
            continue
        if flavors:
            flavor = flavors.get_flavor_by_name(resources.resource_preset_id)
        else:
            flavor = metadb.get_flavor_by_name(resources.resource_preset_id)

        config.update(
            {
                srv: {
                    'resources': resources,
                },
            }
        )

        srvs = []
        if srv != MONGOINFRA_HOST_TYPE:
            srvs.append(srv)
        else:
            srvs.append(MONGOS_HOST_TYPE)
            srvs.append(MONGOCFG_HOST_TYPE)

        for _srv in srvs:
            default_config = default_pillar.get('mongodb', {}).get('config', {}).get(_srv, {})

            schema = get_config_schema(SUBCLUSTER_NAMES[_srv], version.to_string())
            config_set = render_config_set(
                default_config=default_config,
                user_config=pillar.config.get(_srv, {}),
                schema=schema,
                instance_type_obj=flavor,
                version=version,
            )

            if srv != MONGOINFRA_HOST_TYPE:
                config[srv].update(
                    {
                        'config': config_set.as_dict(),
                    }
                )
            else:
                config[srv].update(
                    {
                        'config_{srv}'.format(srv=_srv): config_set.as_dict(),
                    }
                )

    return config


def validate_pitr_versions(src_version: str, dst_version: str):
    """
    Check if pitr is supported by source and destination clusters
    """
    if src_version not in VERSIONS_SUPPORTS_PITR:
        raise PreconditionFailedError(
            'Recovery target is not supported by source cluster version: {0}'.format(src_version)
        )

    if dst_version not in VERSIONS_SUPPORTS_PITR:
        raise PreconditionFailedError(
            'Recovery target is not supported by new cluster version: {0}'.format(dst_version)
        )


def validate_infra_hosts_count(mongocfg=0, mongos=0, mongoinfra=0):
    """
    Check if, in total we have at least
    MIN_MONGOS_HOST_COUNT of mongos (+ mongoinfra)
    and
    MIN_MONGOCFG_HOSTS_COUNT on mongocfg (+ mongoinfra)

    """
    mongos_count = mongos + mongoinfra
    mongocfg_count = mongocfg + mongoinfra

    if mongos_count < MIN_MONGOS_HOSTS_COUNT:
        raise PreconditionFailedError(
            "Only {} MONGOS + MONGOINFRA hosts provided, but at least {} needed".format(
                mongos_count,
                MIN_MONGOS_HOSTS_COUNT,
            )
        )

    if mongocfg_count < MIN_MONGOCFG_HOSTS_COUNT:
        raise PreconditionFailedError(
            "Only {} MONGOCFG + MONGOINFRA hosts provided, but at least {} needed".format(
                mongocfg_count,
                MIN_MONGOCFG_HOSTS_COUNT,
            )
        )


def create_mongodb_sharding_infra_subcluster(
    cluster: dict,
    host_specs: List[dict],
    flavor: dict,
    subnets: Optional[dict],
    disk_size: Optional[int],
    disk_type_id: Optional[str],
    host_type: str,
    pillar: Optional[dict] = None,
) -> Tuple[dict, List[Host]]:
    """
    Create MongoDB subcluster
    """
    if subnets is None:
        network = get_network(cluster['network_id'])
        subnets = get_subnets(network)

    if pillar is None:
        subcluster_name = SUBCLUSTER_NAMES[host_type]
        pillar = deepcopy(current_app.config['DEFAULT_SUBCLUSTER_PILLAR_TEMPLATE'][subcluster_name])
    if disk_size is None:
        raise DbaasClientError('Disk size of MongoDB hosts is not specified')
    subcluster_name = SUBCLUSTER_NAMES[host_type]
    roles = [MongoDBRoles[host_type]]
    subcluster, hosts = clusterutil.create_subcluster(
        cluster_id=cluster['cid'],
        name=subcluster_name,
        subnets=subnets,
        roles=roles,
        flavor=flavor,
        volume_size=disk_size,
        host_specs=host_specs,
        disk_type_id=disk_type_id,
    )

    metadb.add_subcluster_pillar(cluster['cid'], subcluster['subcid'], pillar)
    return subcluster, hosts


def get_allowed_versions_to_restore(source_version: Version) -> List[Version]:
    possible_versions = [source_version.string]
    for version_str, path in UPGRADE_PATHS.items():
        if source_version.to_string() in path['from']:
            possible_versions.append(version_str)

    versions = []
    for version_str in possible_versions:
        try:
            version = Version.load(version_str)
            ensure_version_allowed(MY_CLUSTER_TYPE, version)
            versions.append(version)
        except PreconditionFailedError:
            pass
    return versions


def get_cluster_version(cid, pillar) -> Version:
    """
    Lookup dbaas.versions entry
    :param cid: cluster id of desired cluster
    :return: actual version of cluster
    """

    # (maj, min, edition) or None
    traits = get_cluster_traits(MY_CLUSTER_TYPE)
    cluster = metadb.get_clusters_versions(traits.versions_component, [cid]).get(cid, None)
    if cluster:
        version = Version.load(cluster['major_version'])
        if cluster['edition'] == EDITION_ENTERPRISE:
            version.edition = EDITION_ENTERPRISE
    else:
        # TODO: for backward compatibility - remove after migration of all old clusters to dbaas.versions
        version = pillar.version
    return version


def get_cluster_version_at_time(cid, timestamp, pillar):
    """
    Lookup dbaas.versions_revs entry
    :param cid: cluster id of desired cluster
    :param timestamp: timestamp to restore
    :param pillar: cluster pillar of given timestamp
    :return: version of mongodb cluster at timestamp
    """

    # (maj, min, edition) or None
    traits = get_cluster_traits(MY_CLUSTER_TYPE)
    rev = metadb.get_cluster_rev_by_timestamp(cid=cid, timestamp=timestamp)
    versions_at_rev = metadb.get_cluster_version_at_rev(traits.versions_component, cid, rev)
    if versions_at_rev:
        version_at_rev = versions_at_rev[0]
        version = Version.load(version_at_rev['major_version'])
        if version_at_rev['edition'] == EDITION_ENTERPRISE:
            version.edition = EDITION_ENTERPRISE
    else:
        # TODO: for backward compatibility - remove after migration of all old clusters to dbaas.versions
        version = pillar.version
    return version


def backup_id_from_model(backup: ShardedBackup) -> str:
    """
    Returns backup_id part from api model (cid:backup_id)
    """
    return backup.id.split(':')[-1]


def get_user_database_task_timeout(cluster_pillar: MongoDBPillar) -> timedelta:
    """
    Compute dynamic task timeout based on users/databases count
    """
    value = min(24 * 3600, max(len(cluster_pillar.users('')) * len(cluster_pillar.databases('')), 3600))
    return timedelta(seconds=value)
