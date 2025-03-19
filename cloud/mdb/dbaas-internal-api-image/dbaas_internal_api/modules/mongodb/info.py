# -*- coding: utf-8 -*-
"""
DBaaS Internal API MongoDB cluster options getter
"""
from ...utils import metadb
from ...utils.backups import get_backup_schedule
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.version import format_versioned_key
from .constants import MY_CLUSTER_TYPE
from .pillar import MongoDBPillar
from .utils import build_cluster_config_set, get_cluster_version


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.INFO)
def mongodb_cluster_config(cluster, flavors=None):
    """
    Assemble MongoDB cluster info.
    """
    pillar = MongoDBPillar.load(cluster['value'])
    if 'cluster_pillar_default' in cluster:
        default_pillar = cluster['cluster_pillar_default']
    else:
        default_pillar = metadb.get_cluster_type_pillar(MY_CLUSTER_TYPE)['data']

    # Try don't call 'metadb.*' in this handler.
    # If you need 'new' meta, make sure that it is no longer lying nearby.
    # Keep in mind that this function is used in LIST clusters.
    # Your handler will be called 'cluster-count-in-folder' times.
    # https://st.yandex-team.ru/MDB-7841

    version = get_cluster_version(cluster['cid'], pillar)
    cluster_config = build_cluster_config_set(cluster, pillar, default_pillar, flavors, version)
    backup_schedule = get_backup_schedule(
        MY_CLUSTER_TYPE,
        cluster['backup_schedule'].get('start', {}),
        cluster['backup_schedule'].get('retain_period', None),
    )

    return {
        format_versioned_key('mongodb', version): cluster_config,
        'backup_window_start': backup_schedule.get('start', {}),
        'backup_retain_period_days': backup_schedule.get('retain_period', None),
        'version': version.to_string(),
        'feature_compatibility_version': pillar.feature_compatibility_version,
        'performance_diagnostics': pillar.performance_diagnostics,
        'access': pillar.access,
    }


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.EXTRA_INFO)
def mongodb_cluster_extra_info(cluster: dict) -> dict:
    """
    Returns cluster additional fields
    """
    pillar = MongoDBPillar.load(cluster['value'])
    return {'sharded': pillar.sharding_enabled}
