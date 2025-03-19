import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.katan.models as mod_models
import cloud.mdb.backstage.apps.katan.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ScheduleSkelView(django.views.generic.View):
    def get(self, request, schedule_id, section='common'):
        schedule = mod_models.Schedule.objects.get(pk=schedule_id)
        ctx = {
            'menu': 'Katan',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': schedule,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ScheduleSectionView(django.views.generic.View):
    def get(self, request, schedule_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(schedule_id)
        response.html = mod_helpers.render_html(f'katan/schedules/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, schedule_id):
        return {'obj': mod_models.Schedule.objects.get(pk=schedule_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ScheduleBlockView(django.views.generic.View):
    def get(self, request, schedule_id, block):
        BLOCKS = {
            'rollouts': self.get_rollouts_context,
            'convergence': self.get_convergence_context,
            'dependencies': self.get_dependencies_context,
        }
        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(schedule_id)
        response.html = mod_helpers.render_html(f'katan/schedules/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_rollouts_context(self, schedule_id):
        rollouts = mod_models.Rollout.ui_list.filter(schedule_id=schedule_id)
        return {'rollouts': rollouts}

    def get_dependencies_context(self, schedule_id):
        depends_on = mod_models.ScheduleDependency.objects\
            .filter(schedule_id=schedule_id)\
            .values_list('depends_on', flat=True)

        schedules = mod_models.Schedule.ui_list.filter(pk__in=depends_on)
        return {'schedules': schedules}

    def get_convergence_context(self, schedule_id):
        data = []
        for row in mod_models.Schedule.objects.get(pk=schedule_id).get_convergence():
            cluster_ids = row[3]
            if cluster_ids:
                data.append({
                    'min_age': row[0].days if row[0] else None,
                    'max_age': row[1].days if row[1] else None,
                    'count': row[2],
                    'cluster_ids': cluster_ids,
                })
        return {'data': data}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ScheduleListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Katan',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_schedules_filters(request),
            'onload_url': '/ui/katan/ajax/schedules',
            'title': 'Schedules',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ScheduleListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_schedules_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'katan_schedules',
                'array_name': 'katan_schedules_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Schedule.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('katan/schedules/schedules.html', ctx, request)
        return http.JsonResponse(response)
