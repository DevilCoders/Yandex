"""
DBaaS Internal API MySQL cluster options getter
"""
from dbaas_internal_api.utils.types import ConfigSet
from ...utils import metadb
from ...utils.infra import filter_cluster_resources, get_resources
from ...utils.register import DbaasOperation, Resource, get_config_schema, register_request_handler
from ...utils.renderers import render_config_set
from ...utils.version import format_versioned_key, Version
from .constants import MY_CLUSTER_TYPE
from .pillar import MySQLPillar, get_cluster_pillar
from .utils import get_cluster_version


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.INFO)
def mysql_cluster_config(cluster, flavors=None):
    """
    Assemble MySQL config object.
    """
    if flavors:
        resources = filter_cluster_resources(cluster)
        flavor = flavors.get_flavor_by_name(resources.resource_preset_id)
    else:
        resources = get_resources(cluster['cid'])
        flavor = metadb.get_flavor_by_name(resources.resource_preset_id)

    # Try don't call 'metadb.*' in this handler.
    # If you need 'new' meta, make sure that it is no longer lying nearby.
    # Keep in mind that this function is used in LIST clusters.
    # Your handler will be called 'cluster-count-in-folder' times.
    # https://st.yandex-team.ru/MDB-7841

    pillar = get_cluster_pillar(cluster)

    perf_diag_config_set = render_config_set(
        default_config={},
        user_config=pillar.perf_diag,
        schema=get_config_schema('my_perf_diag', 'any'),
        instance_type_obj=flavor,
        version='any',
    )

    version = evaluate_cluster_version(cluster, pillar)
    config_set = get_cluster_config_set(cluster, pillar, flavor, version)

    return {
        format_versioned_key('mysql_config', version): config_set.as_dict(),
        'backup_window_start': cluster['backup_schedule'].get('start', {}),
        'retain_period': cluster['backup_schedule'].get('retain_period', 7),
        'resources': resources,
        'version': version.to_string(),
        'access': pillar.access,
        'sox_audit': pillar.sox_audit,
        'perf_diag': perf_diag_config_set.as_dict()['effective_config'],
    }


def evaluate_cluster_version(cluster, pillar):
    """
    Evaluates cluster version from cluster or loads from metadb
    """
    if 'versions' in cluster:
        version = cluster['versions']
        version = Version.load(version['major_version'])
    else:
        version = get_cluster_version(cluster['cid'], pillar)

        return version


def get_cluster_config_set(cluster: dict, pillar: MySQLPillar, flavor: dict, version: Version = None) -> ConfigSet:
    """
    Renders cluster config set
    """
    if version is None:
        version = evaluate_cluster_version(cluster, pillar)

    if 'cluster_pillar_default' in cluster:
        default_data = cluster['cluster_pillar_default']
    else:
        default_data = metadb.get_cluster_type_pillar(MY_CLUSTER_TYPE)['data']
    default_config = default_data.get('mysql', {}).get('config', {})

    return render_config_set(
        default_config=default_config,
        user_config=pillar.config.get_config(),
        schema=get_config_schema(MY_CLUSTER_TYPE, version.to_string()),
        instance_type_obj=flavor,
        version=version,
    )
