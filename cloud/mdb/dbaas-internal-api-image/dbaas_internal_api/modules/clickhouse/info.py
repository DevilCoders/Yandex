# -*- coding: utf-8 -*-
"""
DBaaS Internal API ClickHouse cluster options getter
"""
from ...utils import metadb
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.infra import (
    filter_cluster_resources,
    filter_shard_resources,
    get_resources,
    get_shard_resources,
    oldest_shard_id,
)
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.renderers import render_config_set
from .constants import MY_CLUSTER_TYPE
from .pillar import get_pillar
from .schemas.clusters import ClickhouseConfigSchemaV1
from .traits import ClickhouseRoles


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.INFO)
def clickhouse_cluster_config(cluster, flavors=None):
    """
    Assemble ClickHouse config object.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_GET_CLUSTER_API')

    if flavors:
        shard_id = oldest_shard_id(cluster, ClickhouseRoles.clickhouse)
        ch_resources = filter_shard_resources(cluster, shard_id)
        zk_resources = filter_cluster_resources(cluster, ClickhouseRoles.zookeeper)
        flavor = flavors.get_flavor_by_name(ch_resources.resource_preset_id)
    else:
        # slow requests to Meta DB
        shard = metadb.get_oldest_shard(cluster['cid'], ClickhouseRoles.clickhouse)
        ch_resources = get_shard_resources(shard['shard_id'])
        zk_resources = get_resources(cluster['cid'], ClickhouseRoles.zookeeper)
        flavor = metadb.get_flavor_by_name(ch_resources.resource_preset_id)
    if 'cluster_pillar_default' in cluster:
        default_data = cluster['cluster_pillar_default']
    else:
        default_data = metadb.get_cluster_type_pillar(MY_CLUSTER_TYPE)['data']

    # Try don't call 'metadb.*' in this handler.
    # If you need 'new' meta, make sure that it is no longer lying nearby.
    # Keep in mind that this function is used in LIST clusters.
    # Your handler will be called 'cluster-count-in-folder' times.
    # https://st.yandex-team.ru/MDB-7841

    # TODO: request to DB
    pillar = get_pillar(cluster['cid'])
    default_config = default_data.get('clickhouse', {}).get('config', {})
    # TODO: precache flavor requests
    config_set = render_config_set(
        default_config=default_config,
        user_config=pillar.config,
        schema=ClickhouseConfigSchemaV1,
        instance_type_obj=flavor,
    )

    return {
        'backup_window_start': cluster['backup_schedule'].get('start', {}),
        'version': pillar.version.to_string(),
        'mysql_protocol': pillar.mysql_protocol,
        'postgresql_protocol': pillar.postgresql_protocol,
        'sql_user_management': pillar.sql_user_management,
        'sql_database_management': pillar.sql_database_management,
        'embedded_keeper': pillar.embedded_keeper,
        'clickhouse': {
            'config': config_set.as_dict(),
            'resources': ch_resources,
        },
        'zookeeper': {
            'resources': zk_resources,
        },
        'access': pillar.access,
        'service_account_id': pillar.service_account_id(),
        'cloud_storage': pillar.cloud_storage,
    }


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.EXTRA_INFO)
def clickhouse_cluster_extra_info(cluster: dict) -> dict:
    """
    Returns cluster's additional fields
    """
    pillar = get_pillar(cluster['cid'])
    return {'service_account_id': pillar.service_account_id()}
