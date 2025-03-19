import sys
import traceback

from django.conf import settings
import django.http as http

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers

import cloud.mdb.backstage.apps.meta.models as meta_models
import cloud.mdb.backstage.apps.main.filters as mod_filters


@iam_decorators.auth_required
def main(request):
    return http.HttpResponseRedirect('/ui/main/dashboard')


@iam_decorators.auth_required
def dashboard(request):
    search_query = request.GET.get('q')
    show_all_failed_tasks = request.GET.get('show_all_failed_tasks')
    ctx = {
        'menu': 'Dashboard',
        'gore_enabled': settings.CONFIG.gore.enabled,
        'installations': settings.INSTALLATIONS,
        'links': settings.LINKS,
        'current_installation': settings.INSTALLATION,
        'search_query': search_query,
        'show_all_failed_tasks': show_all_failed_tasks,
    }
    html = mod_helpers.render_html('main/views/dashboard.html', ctx, request)
    return http.HttpResponse(html)


@iam_decorators.auth_required
def user_profile(request):
    ctx = {
        'menu': 'main',
    }
    html = mod_helpers.render_html('main/views/user_profile.html', ctx, request)
    return http.HttpResponse(html)


@iam_decorators.auth_required
def server_404_handler(request, exception, **kwargs):
    ctx = {
        'menu': '',
        'exception': exception,
        'installations': settings.INSTALLATIONS,
        'current_installation': settings.INSTALLATION,
    }
    html = mod_helpers.render_html('main/views/404.html', ctx, request)
    return http.HttpResponse(html, status=404)


@iam_decorators.auth_required
def server_500_handler(request, **kwargs):
    type_, value, tb = sys.exc_info()

    ctx = {
        'menu': '',
        'installations': settings.INSTALLATIONS,
        'error': value,
        'traceback': traceback.format_exception(type_, value, tb),
        'current_installation': settings.INSTALLATION,
    }
    html = mod_helpers.render_html('main/views/500.html', ctx, request)
    return http.HttpResponse(html, status=500)


@iam_decorators.auth_required
def stats_versions(request):
    ctx = {
        'menu': 'Stats',
        'get_params': request.GET.urlencode(),
        'filters': mod_filters.get_stats_versions_filters(request),
        'onload_url': '/ui/main/ajax/stats/versions',
        'title': 'Stats: versions',
    }
    html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
    return http.HttpResponse(html)


@iam_decorators.auth_required
def stats_maintenance_tasks(request):
    ctx = {
        'menu': 'Stats',
        'get_params': request.GET.urlencode(),
        'filters': mod_filters.get_stats_maintenance_tasks_filters(request),
        'onload_url': '/ui/main/ajax/stats/maintenance_tasks',
        'title': 'Stats: maintenance tasks',
    }
    html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
    return http.HttpResponse(html)


@iam_decorators.auth_required
def audit(request):
    ctx = {
        'menu': 'Audit',
        'get_params': request.GET.urlencode(),
        'filters': mod_filters.get_audit_filters(request),
        'onload_url': '/ui/main/ajax/audit',
        'title': 'Audit',
    }
    html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
    return http.HttpResponse(html)


@iam_decorators.auth_required
def vector_config(request):
    ctx = {
        'menu': 'Tools',
    }
    html = mod_helpers.render_html('main/views/tools/vector_config.html', ctx, request)
    return http.HttpResponse(html)


@iam_decorators.auth_required
def health_ua(request):
    selected_agg_type = request.GET.get('agg_type', 'clusters')

    env_types = meta_models.EnvType.all
    cluster_types = [
        meta_models.ClusterType.POSTGRESQL_CLUSTER,
        meta_models.ClusterType.CLICKHOUSE_CLUSTER,
        meta_models.ClusterType.MONGODB_CLUSTER,
        meta_models.ClusterType.REDIS_CLUSTER,
        meta_models.ClusterType.MYSQL_CLUSTER,
        meta_models.ClusterType.GREENPLUM_CLUSTER,
        meta_models.ClusterType.SQLSERVER_CLUSTER,
        meta_models.ClusterType.HADOOP_CLUSTER,
        meta_models.ClusterType.KAFKA_CLUSTER,
        meta_models.ClusterType.ELASTICSEARCH_CLUSTER,
    ]

    if settings.INSTALLATION.is_porto() and settings.INSTALLATION.name == 'test':
        env_types = [
            meta_models.EnvType.DEV,
            meta_models.EnvType.QA,
        ]
    elif settings.INSTALLATION.is_porto() and settings.INSTALLATION.name == 'prod':
        env_types = [
            meta_models.EnvType.PROD,
            meta_models.EnvType.QA,
        ]
    elif settings.INSTALLATION.is_compute() and settings.INSTALLATION.name == 'preprod':
        env_types = [
            meta_models.EnvType.DEV,
            meta_models.EnvType.QA,
        ]
    elif settings.INSTALLATION.is_compute() and settings.INSTALLATION.name == 'prod':
        env_types = [
            meta_models.EnvType.COMPUTE_PROD,
            meta_models.EnvType.QA,
        ]
    elif settings.INSTALLATION.is_cloudil():
        env_types = [
            meta_models.EnvType.COMPUTE_PROD,
            meta_models.EnvType.DEV,
        ]
    elif settings.INSTALLATION.is_dc():
        env_types = [
            meta_models.EnvType.DEV,
        ]
        cluster_types = [
            meta_models.ClusterType.CLICKHOUSE_CLUSTER,
            meta_models.ClusterType.KAFKA_CLUSTER,
        ]

    selected_c_type = request.GET.get('c_type', cluster_types[0][0])
    selected_env = request.GET.get('env', env_types[0][0])

    ctx = {
        'menu': 'Tools',
        'selected_agg_type': selected_agg_type,
        'selected_c_type': selected_c_type,
        'selected_env': selected_env,
        'agg_types': [
            'clusters',
            'shards',
        ],
        'env_types': env_types,
        'cluster_types': cluster_types,
    }
    html = mod_helpers.render_html('main/views/tools/health_ua.html', ctx, request)
    return http.HttpResponse(html)
