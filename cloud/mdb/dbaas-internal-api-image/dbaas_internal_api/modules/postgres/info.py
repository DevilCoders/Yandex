# -*- coding: utf-8 -*-
"""
DBaaS Internal API postgresql cluster options getter
"""
from ...utils import metadb
from ...utils.infra import filter_cluster_resources, get_resources
from ...utils.register import DbaasOperation, Resource, get_config_schema, register_request_handler
from ...utils.renderers import render_config_set
from ...utils.version import format_versioned_key
from .constants import MY_CLUSTER_TYPE, EDITION_1C
from .pillar import PostgresqlClusterPillar
from .utils import get_cluster_version
from ...utils.version import Version


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.INFO)
def postgresql_cluster_config(cluster, flavors=None):
    """
    Assemble postgresql config object.
    """
    if flavors:
        resources = filter_cluster_resources(cluster)
        flavor = flavors.get_flavor_by_name(resources.resource_preset_id)
    else:
        # slow requests to Meta DB
        resources = get_resources(cluster['cid'])
        flavor = metadb.get_flavor_by_name(resources.resource_preset_id)
    if 'cluster_pillar_default' in cluster:
        default_data = cluster['cluster_pillar_default']
    else:
        default_data = metadb.get_cluster_type_pillar(MY_CLUSTER_TYPE)['data']
    if 'versions' in cluster:
        version = cluster['versions']
        pg_version = Version.load(version['major_version'])
        if version['edition'] == EDITION_1C:
            pg_version.edition = EDITION_1C
    else:
        pg_version = get_cluster_version(cluster['cid'])

    # Try don't call 'metadb.*' in this handler.
    # If you need 'new' meta, make sure that it is no longer lying nearby.
    # Keep in mind that this function is used in LIST clusters.
    # Your handler will be called 'cluster-count-in-folder' times.
    # https://st.yandex-team.ru/MDB-7841

    pillar = PostgresqlClusterPillar(cluster['value'])
    user_config = pillar.config.get_config()

    default_config = default_data['config']
    config_set = render_config_set(
        default_config=default_config,
        user_config=user_config,
        schema=get_config_schema(MY_CLUSTER_TYPE, pg_version.to_string()),
        instance_type_obj=flavor,
        version=pg_version,
        disk_size=resources.disk_size,
    )

    result = {
        format_versioned_key('postgresql_config', pg_version): config_set.as_dict(),
        'autofailover': pillar.pgsync.get_autofailover(),
        'backup_window_start': cluster['backup_schedule'].get('start', {}),
        'retain_period': cluster['backup_schedule'].get('retain_period', 7),
        'pooler_config': {
            'pool_mode': user_config.get('pool_mode'),
        },
        'resources': resources,
        'version': pg_version.to_string(),
        'access': pillar.access,
        'sox_audit': pillar.sox_audit,
        'perf_diag': render_config_set(
            default_config={},
            user_config=pillar.perf_diag.get(),
            schema=get_config_schema('pg_perf_diag', 'any'),
            instance_type_obj=flavor,
            version='any',
        ).as_dict()['effective_config'],
    }

    if 'monitoring_cloud_id' in cluster:
        result['monitoring_cloud_id'] = cluster['monitoring_cloud_id']

    if 'server_reset_query_always' in user_config:
        result['pooler_config']['server_reset_query_always'] = user_config['server_reset_query_always']
    return result
