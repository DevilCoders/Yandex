import cloud.mdb.backstage.lib.params as mod_params
import metrika.pylib.utils as utils

import cloud.mdb.backstage.apps.meta.models as mod_models


CLUSTERS_PARAMS = [
    mod_params.QuerysetListParam(
        key='cid',
        name='CID',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetRegexParam(
        key='name',
    ),
    mod_params.QuerysetRegexParam(
        key='env',
        name='Environment',
    ),
    mod_params.QuerysetIntListParam(
        key='folder_id',
        queryset_key='folder__folder_id__in',
    ),
    mod_params.QuerysetListParam(
        key='folder_ext_id',
        queryset_key='folder__folder_ext_id__in',
    ),
    mod_params.QuerysetMultipleChoicesParam(
        'type',
        choices=mod_models.ClusterType.all,
    ),
    mod_params.QuerysetMultipleChoicesParam(
        key='status',
        choices=mod_models.ClusterStatus.all,
        css_class_prefix='backstage-meta-cluster-status',
    ),
    mod_params.QuerysetListParam(
        key='component',
        queryset_key='versions__component__in',
    ),
    mod_params.QuerysetListParam(
        key='major_version',
        queryset_key='versions__major_version__in',
    ),
    mod_params.QuerysetListParam(
        key='minor_version',
        queryset_key='versions__minor_version__in',
    ),
    mod_params.QuerysetRegexParam(
        key='edition',
        queryset_key='versions__edition__regex',
    ),
]

WORKER_TASKS_PARAMS = [
    mod_params.QuerysetListParam(
        key='task_id',
        name='ID',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetParam(
        key='cid',
        name='Cluster ID',
        queryset_key='cluster__cid',
    ),
    mod_params.QuerysetRegexParam(
        key='task_type',
        name='Type',
    ),
    mod_params.QuerysetRegexParam(
        key='created_by',
        name='Created By',
    ),
    mod_params.QuerysetRegexParam(
        key='worker_id',
        name='Worker ID',
    ),
    mod_params.QuerysetMultipleChoicesParam(
        key='result',
        choices=mod_models.BooleanChoices.all,
        to=utils.str_to_bool,
    ),
]

CLOUDS_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='cloud_id',
        name='ID',
    ),
    mod_params.QuerysetListParam(
        key='ext_id',
        name='External ID',
        queryset_key='cloud_ext_id__in',
        is_opened=True,
        is_focused=True,
    ),
]

HOSTS_PARAMS = [
    mod_params.QuerysetListParam(
        key='fqdn',
        name='FQDN',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetListParam(
        key='subcluster_id',
        name='Subcluster ID',
        queryset_key='subcluster__subcid__in',
    ),
    mod_params.QuerysetListParam(
        key='cluster_id',
        name='Cluster ID',
        queryset_key='subcluster__cluster__cid__in',
    ),
    mod_params.QuerysetListParam(
        key='vtype_id',
        name='VType ID',
    ),
]

FLAVORS_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='id',
        name='ID',
    ),
    mod_params.QuerysetRegexParam(
        key='name',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetRegexParam(
        key='type',
    ),
    mod_params.QuerysetRegexParam(
        key='vtype',
        name='VType',
    ),
]

FOLDERS_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='folder_id',
        name='ID',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetListParam(
        key='ext_id',
        name='External ID',
        queryset_key='folder_ext_id__in',
    ),
    mod_params.QuerysetListParam(
        key='cloud_id',
        name='Cloud ID',
        queryset_key='cloud__cloud_ext_id__in',
    ),
]

BACKUPS_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='backup_id',
        name='ID',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetMultipleChoicesParam(
        key='status',
        choices=mod_models.BackupStatus.all,
    ),
]

MAINTENANCE_TASKS_PARAMS = [
    mod_params.QuerysetListParam(
        key='task_id',
        name='ID',
    ),
    mod_params.QuerysetRegexParam(
        key='config_id',
        name='Config ID',
    ),
    mod_params.QuerysetRegexParam(
        key='info',
    ),
    mod_params.QuerysetListParam(
        key='cluster_id',
        name='Cluster ID',
        queryset_key='cluster_id__in',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetListParam(
        key='cluster_env',
        queryset_key='cluster__env__in'
    ),
    mod_params.QuerysetListParam(
        key='cluster_component',
        queryset_key='cluster__versions__component__in'
    ),
]

SHARDS_PARAMS = [
    mod_params.QuerysetListParam(
        key='shard_id',
        name='ID',
    ),
    mod_params.QuerysetRegexParam(
        key='name',
    ),
    mod_params.QuerysetListParam(
        key='subcluster_id',
        name='Subcluster ID',
        queryset_key='subcluster__subcid__in',
        is_opened=True,
        is_focused=True,
    ),
]

SUBCLUSTERS_PARAMS = [
    mod_params.QuerysetListParam(
        key='subcid_id',
        name='ID',
    ),
    mod_params.QuerysetListParam(
        key='cluster_id',
        name='Cluster ID',
        queryset_key='cluster__cid__in',
        is_opened=True,
        is_focused=True,
    ),
]

VALID_RESOURCES_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='id',
        name='ID',
    ),
    mod_params.QuerysetRegexParam(
        key='cluster_type',
        name='Cluster type',
    ),
    mod_params.QuerysetRegexParam(
        key='role',
    ),
    mod_params.QuerysetRegexParam(
        key='flavor',
        queryset_key='flavor__name__regex'
    ),
    mod_params.QuerysetRegexParam(
        key='disk_type',
        queryset_key='disk_type__disk_type_ext_id__regex',
    ),
    mod_params.QuerysetRegexParam(
        key='geo',
        queryset_key='geo__name__regex',
    ),
]

VERSIONS_PARAMS = [
    mod_params.QuerysetRegexParam(
        key='component',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetRegexParam(
        key='package_version',
    ),
    mod_params.QuerysetRegexParam(
        key='major_version',
    ),
    mod_params.QuerysetRegexParam(
        key='minor_version',
    ),
    mod_params.QuerysetListParam(
        key='cluster_id',
        name='Cluster ID',
        queryset_key='cluster_id__in',
    ),
    mod_params.QuerysetListParam(
        key='subcluster_id',
        name='Subcluster ID',
        queryset_key='subcluster_id__in',
    ),
    mod_params.QuerysetListParam(
        key='sahrd_id',
        name='Shard ID',
        queryset_key='shard_id__in',
    ),
]

DEFAULT_VERSIONS_PARAMS = [
    mod_params.QuerysetRegexParam(
        key='component',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetRegexParam(
        key='type',
    ),
    mod_params.QuerysetRegexParam(
        key='package_version',
    ),
    mod_params.QuerysetRegexParam(
        key='major_version',
    ),
    mod_params.QuerysetRegexParam(
        key='minor_version',
    ),
    mod_params.QuerysetRegexParam(
        key='env',
    ),
    mod_params.QuerysetRegexParam(
        key='name',
    ),
    mod_params.QuerysetRegexParam(
        key='edition',
    ),
]


def get_clusters_filters(request):
    return mod_params.Filter(
        CLUSTERS_PARAMS,
        request,
        url='/ui/meta/ajax/clusters',
        href='/ui/meta/clusters',
        js_object='meta_clusters_filter_object',
    ).parse()


def get_worker_tasks_filters(request):
    return mod_params.Filter(
        WORKER_TASKS_PARAMS,
        request,
        url='/ui/meta/ajax/worker_tasks',
        href='/ui/meta/worker_tasks',
        js_object='meta_worker_tasks_filter_object',
    ).parse()


def get_clouds_filters(request):
    return mod_params.Filter(
        CLOUDS_PARAMS,
        request,
        url='/ui/meta/ajax/clouds',
        href='/ui/meta/clouds',
        js_object='meta_clouds_filter_object',
    ).parse()


def get_hosts_filters(request):
    return mod_params.Filter(
        HOSTS_PARAMS,
        request,
        url='/ui/meta/ajax/hosts',
        href='/ui/meta/hosts',
        js_object='meta_hosts_filter_object',
    ).parse()


def get_valid_resources_filters(request):
    return mod_params.Filter(
        VALID_RESOURCES_PARAMS,
        request,
        url='/ui/meta/ajax/valid_resources',
        href='/ui/meta/valid_resources',
        js_object='meta_valid_resources_filter_object',
    ).parse()


def get_subclusters_filters(request):
    return mod_params.Filter(
        SUBCLUSTERS_PARAMS,
        request,
        url='/ui/meta/ajax/subclusters',
        href='/ui/meta/subclusters',
        js_object='meta_subclusters_filter_object',
    ).parse()


def get_shards_filters(request):
    return mod_params.Filter(
        SHARDS_PARAMS,
        request,
        url='/ui/meta/ajax/shards',
        href='/ui/meta/shards',
        js_object='meta_shards_filter_object',
    ).parse()


def get_maintenance_tasks_filters(request):
    return mod_params.Filter(
        MAINTENANCE_TASKS_PARAMS,
        request,
        url='/ui/meta/ajax/maintenance_tasks',
        href='/ui/meta/maintenance_tasks',
        js_object='meta_maintenance_tasks_filter_object',
    ).parse()


def get_folders_filters(request):
    return mod_params.Filter(
        FOLDERS_PARAMS,
        request,
        url='/ui/meta/ajax/folders',
        href='/ui/meta/folders',
        js_object='meta_folders_filter_object',
    ).parse()


def get_flavors_filters(request):
    return mod_params.Filter(
        FLAVORS_PARAMS,
        request,
        url='/ui/meta/ajax/flavors',
        href='/ui/meta/flavors',
        js_object='meta_flavors_filter_object',
    ).parse()


def get_backups_filters(request):
    return mod_params.Filter(
        BACKUPS_PARAMS,
        request,
        url='/ui/meta/ajax/backups',
        href='/ui/meta/backups',
        js_object='meta_backups_filter_object',
    ).parse()


def get_versions_filters(request):
    return mod_params.Filter(
        VERSIONS_PARAMS,
        request,
        url='/ui/meta/ajax/versions',
        href='/ui/meta/versions',
        js_object='meta_versions_filter_object',
    ).parse()


def get_default_versions_filters(request):
    return mod_params.Filter(
        DEFAULT_VERSIONS_PARAMS,
        request,
        url='/ui/meta/ajax/versions',
        href='/ui/meta/versions',
        js_object='meta_versions_filter_object',
    ).parse()
