import json
import toml
import yaml
import zoneinfo
from django.conf import settings

import django.db
import django.http as http
import django.utils.timezone as timezone
import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import metrika.pylib.utils as mpu

import cloud.mdb.backstage.lib.gore as mod_gore
import cloud.mdb.backstage.lib.health as mod_health
import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.meta.models as meta_models

import cloud.mdb.backstage.lib.search as mod_search

import cloud.mdb.backstage.apps.main.models as mod_models
import cloud.mdb.backstage.apps.main.profile as mod_profile
import cloud.mdb.backstage.apps.main.filters as mod_filters


HEALTH = mod_health.HealthApi()


STATS_VERSIONS_QUERY = """
    SELECT
        component, major_version, edition, minor_version, count(*)
    FROM
        dbaas.versions
    JOIN
        dbaas.clusters USING (cid)
    WHERE
        dbaas.active_cluster_status(clusters.status)
    GROUP BY
        major_version, minor_version, component, edition
    ORDER BY
        component, major_version, minor_version, edition
"""

STATS_MAINTENANCE_STATUS_QUERY = """
    SELECT
        v.component, env, mt.status, config_id, count(*)
    FROM
        dbaas.versions v
    JOIN
        dbaas.clusters c using (cid)
    JOIN
        dbaas.default_versions dv using (type, env, major_version, edition)
    LEFT JOIN
        (SELECT * FROM dbaas.maintenance_tasks) mt
    ON
        mt.cid = c.cid
    WHERE
        dv.component = v.component
    AND
        dbaas.visible_cluster_status(c.status)
    AND
        (v.minor_version != dv.minor_version or mt.status = 'COMPLETED')
    GROUP BY
        env, mt.status, v.component, config_id
    ORDER BY 1, 2, 3
"""

GORE = mod_gore.GoreApi()
GORE_SERVICES = [
    'mdb-postgres',
    'mdb-elasticsearch',
    'mdb-mysql',
    'mdb-greenplum',
    'mdb-clickhouse',
    'mdb-mongodb',
    'mdb-redis',
    'mdb-sqlserver',
    'mdb-core',
]


@iam_decorators.auth_required
def user_profile(request):
    response = mod_response.Response()

    if request.method == 'GET':
        ctx = {
            'no_database': settings.CONFIG.no_database,
            'now': timezone.now(),
            'timezones': sorted(zoneinfo.available_timezones()),
        }
        response.html = mod_helpers.render_html('main/ajax/user_profile.html', ctx, request)
    elif request.method == 'POST':
        post_data = json.loads(request.POST.get('data'))['params']
        profile = mod_profile.get_profile_model_instance(request.iam_user)
        profile.settings.update(post_data['settings'])
        profile.save()

    return http.JsonResponse(response)


@iam_decorators.auth_required
def failed_tasks(request):
    response = mod_response.Response()
    show_all_failed_tasks = mpu.str_to_bool(request.GET.get('show_all_failed_tasks'))

    tasks = meta_models.WorkerTask.get_failed_tasks()
    total = len(tasks)
    tasks = meta_models.WorkerTask.ui_list.filter(pk__in=[task.pk for task in tasks])
    if not show_all_failed_tasks:
        tasks = tasks[:10]
    count = len(tasks)
    ctx = {
        'total': total,
        'count': count,
        'tasks': tasks,
    }
    response.html = mod_helpers.render_html('main/ajax/failed_tasks.html', ctx, request)
    return http.JsonResponse(response)


@iam_decorators.auth_required
def duties(request):
    response = mod_response.Response()
    services_map = {}

    for service in GORE.services():
        if service['service'] in GORE_SERVICES and service['active']:
            services_map[service['id']] = {
                'name': service['service'],
                'lead': service['teamowners']['lead'],
                'primary': None,
                'backup': None,
                'duties': [],
                'error': None,
            }

    for duty in GORE.duty():
        if duty['serviceid'] in services_map:
            services_map[duty['serviceid']]['duties'].append(duty)
            service = services_map[duty['serviceid']]

    services = []
    for _, data in services_map.items():
        try:
            now = int(timezone.now().timestamp())
            data['duties'] = sorted(data['duties'], key=lambda i: i['dateend'])
            data['duties'] = list(filter(lambda i: i['datestart'] < now < i['dateend'], data['duties']))

            if len(data['duties']) < 2:
                data['error'] = 'found more than 2 duty'

            for duty in data['duties']:
                if duty['resp']['order'] == 0:
                    data['primary'] = duty['resp']['username']
                elif duty['resp']['order'] == 1:
                    data['backup'] = duty['resp']['username']
        except Exception as err:
            data['error'] = f'failed to get duties: {err}'
        services.append(data)

    ctx = {
        'services': sorted(services, key=lambda s: s['name']),
    }
    response.html = mod_helpers.render_html('main/ajax/duties.html', ctx, request)
    return http.JsonResponse(response)


@iam_decorators.auth_required
def search(request):
    search_query = request.GET.get('q')
    response = mod_response.Response()

    ctx = {
        'result': mod_search.search(search_query)
    }
    response.html = mod_helpers.render_html('main/ajax/search.html', ctx, request)
    return http.JsonResponse(response)


@iam_decorators.auth_required
def stats_versions(request):
    response = mod_response.Response()
    versions = []
    filters = mod_filters.get_stats_versions_filters(request)
    ctx = {
        'filters': filters,
    }

    if not filters.errors:
        with django.db.connections['meta_db'].cursor() as cursor:
            cursor.execute(STATS_VERSIONS_QUERY)
            rows = cursor.fetchall()
            fields = [i[0] for i in cursor.description]

            for row in rows:
                row_dict = dict(zip(fields, row))
                versions.append(row_dict)

        for name in ['component', 'major_version', 'minor_version', 'edition']:
            regex = filters.regex.get(name)
            if regex:
                versions = filter(lambda v: regex.search(v[name]), versions)
                versions = list(versions)
    ctx = {
        'versions': versions,
        'cluster_active_statuses': [s[0] for s in meta_models.ClusterStatus.active],
    }
    response.html = mod_helpers.render_html('main/ajax/stats_versions.html', ctx, request)
    return http.JsonResponse(response)


@iam_decorators.auth_required
def stats_maintenance_tasks(request):
    response = mod_response.Response()
    maintenance_tasks = []
    filters = mod_filters.get_stats_maintenance_tasks_filters(request)
    ctx = {
        'filters': filters,
    }

    if not filters.errors:
        with django.db.connections['meta_db'].cursor() as cursor:
            cursor.execute(STATS_MAINTENANCE_STATUS_QUERY)
            rows = cursor.fetchall()
            fields = [i[0] for i in cursor.description]

            for row in rows:
                row_dict = dict(zip(fields, row))
                maintenance_tasks.append(row_dict)

        for name in ['component', 'env', 'status', 'config_id']:
            regex = filters.regex.get(name)
            if regex:
                maintenance_tasks = filter(lambda mt: regex.search(mt[name]), maintenance_tasks)
                maintenance_tasks = list(maintenance_tasks)
    ctx = {
        'maintenance_tasks': maintenance_tasks,
    }
    response.html = mod_helpers.render_html('main/ajax/stats_maintenance_tasks.html', ctx, request)
    return http.JsonResponse(response)


@iam_decorators.auth_required
def related_links(request, fqdn):
    response = mod_response.Response()
    ctx = {
        'links': mod_search.search_by_fqdn_links(fqdn),
    }
    response.html = mod_helpers.render_html('main/ajax/related_links.html', ctx, request)
    return http.JsonResponse(response)


@iam_decorators.auth_required
def audit(request):
    response = mod_response.Response()
    filters = mod_filters.get_audit_filters(request)

    ctx = {
        'no_database': settings.CONFIG.no_database,
        'filters': filters,
    }

    if not settings.CONFIG.no_database:
        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.AuditLog.objects.filter(**filters.queryset).order_by('-datetime'),
                request=request,
                context=ctx,
            )

    response.html = mod_helpers.render_html('main/ajax/audit.html', ctx, request)
    return http.JsonResponse(response)


@iam_decorators.auth_required
def tools_vector_schema(request):
    response = mod_response.Response()
    post_data = mod_helpers.get_post_data(request)
    config_text = post_data['config_text']
    config = None
    errors = []

    try:
        config = yaml.load(config_text, Loader=yaml.SafeLoader)
    except Exception as err:
        errors.append(f'YAML: failed to load config: {err}')
        try:
            config = toml.loads(config_text)
        except Exception as err:
            errors.append(f'TOML: failed to load config: {err}')
        else:
            errors = []

    ctx = {
        'config': config,
        'errors': errors,
    }
    response.html = mod_helpers.render_html('main/ajax/tools/vector_schema.html', ctx, request)
    return http.JsonResponse(response)


@iam_decorators.auth_required
def health_ua(request):
    response = mod_response.Response()

    post_data = mod_helpers.get_post_data(request)

    data, _ = HEALTH.unhealthyaggregatedinfo(
        agg_type=post_data['agg_type'],
        c_type=post_data['c_type'],
        env=post_data['env']
    )
    ctx = {
        'data': data,
        'agg_type': post_data['agg_type'],
    }
    response.html = mod_helpers.render_html('main/ajax/tools/health_ua.html', ctx, request)
    return http.JsonResponse(response)
