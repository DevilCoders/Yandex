"""
DBaaS Internal API Redis cluster options getter
"""
from ...utils import metadb
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.infra import filter_cluster_resources, get_resources
from ...utils.register import DbaasOperation, Resource, get_config_schema, register_request_handler
from ...utils.renderers import render_config_set
from ...utils.version import format_versioned_key
from .constants import MY_CLUSTER_TYPE
from .pillar import RedisPillar
from .utils import get_cluster_version


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.INFO)
def redis_cluster_config(cluster, flavors=None):
    """
    Assemble Redis config object.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CLUSTER_CRUD_API')
    if flavors:
        resources = filter_cluster_resources(cluster)
        flavor = flavors.get_flavor_by_name(resources.resource_preset_id)
    else:
        resources = get_resources(cluster['cid'])
        flavor = metadb.get_flavor_by_name(resources.resource_preset_id)
    if 'cluster_pillar_default' in cluster:
        default_data = cluster['cluster_pillar_default']
    else:
        default_data = metadb.get_cluster_type_pillar(MY_CLUSTER_TYPE)['data']
    default_config = default_data.get('redis', {}).get('config', {})

    # Try don't call 'metadb.*' in this handler.
    # If you need 'new' meta, make sure that it is no longer lying nearby.
    # Keep in mind that this function is used in LIST clusters.
    # Your handler will be called 'cluster-count-in-folder' times.
    # https://st.yandex-team.ru/MDB-7841

    pillar = RedisPillar(cluster['value'])
    version = get_cluster_version(cluster['cid'])

    config_set = render_config_set(
        default_config=default_config,
        user_config=pillar.get_info_config(),
        schema=get_config_schema(MY_CLUSTER_TYPE, version.to_string()),
        instance_type_obj=flavor,
        version=version,
    )

    return {
        format_versioned_key('redis_config', version): config_set.as_dict(),
        'backup_window_start': cluster['backup_schedule'].get('start', {}),
        'resources': resources,
        'version': version.to_string(),
        'access': pillar.access,
    }


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.EXTRA_INFO)
def redis_cluster_extra_info(cluster: dict) -> dict:
    """
    Returns cluster's additional fields
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CONSOLE_API')
    pillar = RedisPillar(cluster['value'])
    return {
        'sharded': pillar.is_cluster_enabled(),
        'tls_enabled': pillar.tls_enabled,
        'persistence_mode': pillar.get_persistence_mode_for_info(),
    }
