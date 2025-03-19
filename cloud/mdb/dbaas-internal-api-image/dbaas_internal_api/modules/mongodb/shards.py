# -*- coding: utf-8 -*-
"""
API for MongoDB shards management.
"""

from collections import defaultdict
from copy import deepcopy
from datetime import timedelta
from typing import Dict, List

from ...core.exceptions import (
    DbaasClientError,
    PreconditionFailedError,
    ShardExistsError,
    ShardNameCollisionError,
    ShardNotExistsError,
    MongoDBMixedShardingInfraConfigurationError,
)
from ...utils import metadb
from ...utils.cluster import create as clusterutil
from ...utils.cluster.delete import delete_shard_with_task
from ...utils.cluster.get import find_shard_by_name, get_shards
from ...utils.config import cluster_type_config
from ...utils.feature_flags import has_feature_flag
from ...utils.helpers import first_value
from ...utils.host import filter_host_map, get_hosts
from ...utils.infra import get_shard_resources
from ...utils.metadata import CreateShardMetadata, DeleteShardMetadata, Metadata
from ...utils.network import get_network, get_subnets
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import Host, RequestedHostResources
from ...utils.validation import (
    QuotaValidator,
    check_quota,
    get_flavor_by_name,
    validate_host_create_resources,
    validate_hosts_count,
)
from .constants import (
    MAX_SHARDS_COUNT,
    MY_CLUSTER_TYPE,
    SHARDING_INFRA_HOST_TYPES,
    UNLIMITED_SHARD_COUNT_FEATURE_FLAG,
    VERSIONS_SUPPORTS_SHARDING,
    MIXED_SHARDING_CONFIG_FEATURE_FLAG,
    MONGOCFG_HOST_TYPE,
    MONGOINFRA_HOST_TYPE,
    DEPLOY_API_RETRIES_COUNT,
    GBYTES,
)
from .pillar import get_cluster_pillar
from .traits import MongoDBOperations, MongoDBRoles, MongoDBServices, MongoDBTasks
from .utils import validate_infra_hosts_count, create_mongodb_sharding_infra_subcluster, get_cluster_version


class EnableShardingMetadata(Metadata):
    """
    Create MongoDB sharding metadata
    """

    def _asdict(self) -> dict:
        return {}


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.CREATE)
def create_mongodb_shard(cluster, shard_name, host_specs, **_):
    # pylint: disable=too-many-locals
    """
    Adds MongoDB shard.
    """

    cluster_pillar = get_cluster_pillar(cluster)
    if not cluster_pillar.sharding_enabled:
        raise PreconditionFailedError('Sharding must be enabled')

    shards = metadb.get_shards(cid=cluster['cid'], role=MongoDBRoles.mongod)
    if find_shard_by_name(shards, shard_name):
        raise ShardExistsError(shard_name)

    collision = find_shard_by_name(shards, shard_name, ignore_case=True)
    if collision:
        raise ShardNameCollisionError(shard_name, collision['name'])

    if not has_feature_flag(UNLIMITED_SHARD_COUNT_FEATURE_FLAG):
        shard_count_limit = cluster_type_config(MY_CLUSTER_TYPE).get('shard_count_limit', MAX_SHARDS_COUNT)
        if len(shards) >= shard_count_limit:
            raise PreconditionFailedError(
                'Cluster can have up to a maximum of {0} MongoDB shards'.format(shard_count_limit)
            )

    for host_spec in host_specs:
        if host_spec.get('type', MongoDBRoles.mongod) != MongoDBRoles.mongod:
            raise DbaasClientError('Shard must have MONGOD hosts only')
        if host_spec.get('shard_name', shard_name) != shard_name:
            raise DbaasClientError('Shard hosts must have single shard name')

    # all mongod shards&hosts are equal for now
    first_shard = shards[0]
    first_shard_resources = get_shard_resources(first_shard['shard_id'])
    first_shard_flavor = get_flavor_by_name(first_shard_resources.resource_preset_id)

    host_count = len(host_specs)
    validate_hosts_count(
        MY_CLUSTER_TYPE,
        MongoDBRoles.mongod.value,  # pylint: disable=no-member
        first_shard_resources.resource_preset_id,
        first_shard_resources.disk_type_id,
        host_count,
    )
    check_quota(first_shard_flavor, host_count, first_shard_resources.disk_size, first_shard_resources.disk_type_id)

    for host in host_specs:
        validate_host_create_resources(
            cluster_type=MY_CLUSTER_TYPE,
            role=MongoDBRoles.mongod,
            resource_preset_id=first_shard_resources.resource_preset_id,
            geo=host['zone_id'],
            disk_type_id=first_shard_resources.disk_type_id,
            disk_size=first_shard_resources.disk_size,
        )

    network = get_network(cluster['network_id'])
    subnets = get_subnets(network)

    shard, _ = clusterutil.create_shard(
        cid=cluster['cid'],
        subcid=first_shard['subcid'],
        name=shard_name,
        flavor=first_shard_flavor,
        subnets=subnets,
        volume_size=first_shard_resources.disk_size,
        host_specs=host_specs,
        disk_type_id=first_shard_resources.disk_type_id,
    )

    return create_operation(
        MongoDBTasks.shard_create,
        cid=cluster['cid'],
        operation_type=MongoDBOperations.shard_create,
        metadata=CreateShardMetadata(shard_name),
        task_args={'shard_id': shard['shard_id']},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.DELETE)
def delete_mongodb_shard(cluster, shard_name, **_):
    """
    Deletes MongoDB shard.
    """

    cluster_pillar = get_cluster_pillar(cluster)
    if not cluster_pillar.sharding_enabled:
        raise PreconditionFailedError('Sharding must be enabled')

    shards = metadb.get_shards(cid=cluster['cid'], role=MongoDBRoles.mongod)

    delete_shard = find_shard_by_name(shards, shard_name)
    if not delete_shard:
        raise ShardNotExistsError(shard_name)

    if len(shards) <= 1:
        raise PreconditionFailedError('Last shard in cluster cannot be removed')

    hosts = get_hosts(cluster['cid'])
    template_hosts = filter_host_map(hosts, MongoDBRoles.mongod)
    opts = first_value(template_hosts)
    space_limit = opts['space_limit']

    return delete_shard_with_task(
        cluster=cluster,
        task_type=MongoDBTasks.shard_delete,
        operation_type=MongoDBOperations.shard_delete,
        metadata=DeleteShardMetadata(shard_name),
        shard_id=delete_shard['shard_id'],
        time_limit=timedelta(
            hours=max(
                1,
                # We assume that mongodb could move at least 10GB/Hour from deleteing shard
                ((space_limit / GBYTES) / 10) * DEPLOY_API_RETRIES_COUNT,
            )
        ),
        args={
            'shard_id': delete_shard['shard_id'],
            'shard_name': delete_shard['name'],
        },
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.LIST)
def list_mongodb_shards(cluster, **_):
    """
    Returns list of MongoDB shards.
    """
    return {'shards': get_shards(cluster, MongoDBRoles.mongod)}


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.INFO)
def get_mongodb_shard(cluster, shard_name, **_):
    """
    Returns MongoDB shard.
    """
    shards = metadb.get_shards(cid=cluster['cid'], role=MongoDBRoles.mongod)
    shard = find_shard_by_name(shards, shard_name)
    if not shard:
        raise ShardNotExistsError(shard_name)

    return {'name': shard['name'], 'cluster_id': cluster['cid']}


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.ENABLE_SHARDING)
def enable_mongodb_sharding(cluster, mongos=None, mongocfg=None, host_specs=None, mongoinfra=None, **_):
    # pylint: disable=too-many-locals
    """
    Enables mongodb sharding

    Creates mongos+mongocfg subclusters
    """

    cluster_pillar = get_cluster_pillar(cluster)
    if host_specs is None:
        raise DbaasClientError('host_specs were not provided')

    version = get_cluster_version(cluster['cid'], cluster_pillar)
    if version.to_string() not in VERSIONS_SUPPORTS_SHARDING:
        raise PreconditionFailedError(
            'Sharding requires one of following mongodb versions: {0}'.format(', '.join(VERSIONS_SUPPORTS_SHARDING))
        )

    if cluster_pillar.sharding_enabled:
        raise PreconditionFailedError('Sharding has been already enabled')

    if mongoinfra is None and (mongos is None or mongocfg is None):
        raise DbaasClientError('Mongos and mongocfg resources must be specified if mongoinfra is not used')

    # Fill unspecified resources with values from mongoinfra\mongocfg
    # So we'll have resources for later, when we'll want to add such hosts
    if mongoinfra is None:
        mongoinfra = deepcopy(mongocfg)
    if mongos is None:
        mongos = deepcopy(mongoinfra)
    if mongocfg is None:
        mongocfg = deepcopy(mongoinfra)

    mongos_resources, mongocfg_resources, mongoinfra_resources = [
        RequestedHostResources(**s.get('resources')) for s in (mongos, mongocfg, mongoinfra)
    ]

    resources = dict(mongos=mongos_resources, mongocfg=mongocfg_resources, mongoinfra=mongoinfra_resources)
    flavors = {k: get_flavor_by_name(resources[k].resource_preset_id) for k in resources}

    mongo_host_specs = _process_host_specs(host_specs, resources)

    mixed_conditions = (
        not has_feature_flag(MIXED_SHARDING_CONFIG_FEATURE_FLAG),
        mongo_host_specs[MONGOCFG_HOST_TYPE],
        mongo_host_specs[MONGOINFRA_HOST_TYPE],
    )
    if all(mixed_conditions):
        raise MongoDBMixedShardingInfraConfigurationError()

    validator = QuotaValidator()
    for host_type in SHARDING_INFRA_HOST_TYPES:
        if mongo_host_specs[host_type]:
            validator.add(
                flavors[host_type],
                len(mongo_host_specs[host_type]),
                resources[host_type].disk_size,
                resources[host_type].disk_type_id,
            )
    validator.validate()

    for host_type in SHARDING_INFRA_HOST_TYPES:
        if mongo_host_specs[host_type]:
            create_mongodb_sharding_infra_subcluster(
                cluster=cluster,
                host_specs=mongo_host_specs[host_type],
                flavor=flavors[host_type],
                subnets=None,
                disk_size=resources[host_type].disk_size,
                disk_type_id=resources[host_type].disk_type_id,
                host_type=host_type,
            )

    cluster_pillar.sharding_enabled = True
    metadb.update_cluster_pillar(cluster['cid'], cluster_pillar)

    shard = metadb.get_shards(cid=cluster['cid'], role=MongoDBRoles.mongod)[0]
    return create_operation(
        MongoDBTasks.enable_sharding,
        cid=cluster['cid'],
        operation_type=MongoDBOperations.enable_sharding,
        metadata=EnableShardingMetadata(),
        task_args={'shard_id': shard['shard_id']},
    )


def _process_host_specs(
    host_specs: List[Host],
    resources: dict,
) -> Dict:

    mongo_host_specs = defaultdict(list)  # type: Dict
    for host in host_specs:
        host_type = host.get('type')
        if host_type is None:
            raise DbaasClientError('Type of MongoDB host is not specified')

        if host_type not in (MongoDBRoles.mongos, MongoDBRoles.mongocfg, MongoDBRoles.mongoinfra):
            raise DbaasClientError('MONGOS, MONGOCFG or MONGOINFRA host type expected')

        host_type = MongoDBServices[host['type'].value]
        mongo_host_specs[host_type].append(host)

    infra_hosts_count = {}

    for host_type in SHARDING_INFRA_HOST_TYPES:
        hosts = mongo_host_specs.get(host_type, [])
        infra_hosts_count[host_type] = len(hosts)

        validate_hosts_count(
            cluster_type=MY_CLUSTER_TYPE,
            role=MongoDBRoles[host_type].value,  # pylint: disable=no-member
            resource_preset_id=resources[host_type].resource_preset_id,
            disk_type_id=resources[host_type].disk_type_id,
            hosts_count=len(hosts),
        )

    validate_infra_hosts_count(**infra_hosts_count)

    for host_type in resources:
        for host in mongo_host_specs[host_type]:
            validate_host_create_resources(
                cluster_type=MY_CLUSTER_TYPE,
                role=MongoDBRoles[host_type],
                resource_preset_id=resources[host_type].resource_preset_id,
                geo=host['zone_id'],
                disk_size=resources[host_type].disk_size,
                disk_type_id=resources[host_type].disk_type_id,
            )

    return mongo_host_specs
