import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.meta.models as mod_models
import cloud.mdb.backstage.apps.meta.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class MaintenanceTaskSkelView(django.views.generic.View):
    def get(self, request, task_id, section='common'):
        maintenance_task = mod_models.MaintenanceTask.objects.get(task_id=task_id)
        ctx = {
            'menu': 'Meta',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': maintenance_task,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class MaintenanceTaskSectionView(django.views.generic.View):
    def get(self, request, task_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(task_id)
        response.html = mod_helpers.render_html(f'meta/maintenance_tasks/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, task_id):
        return {'obj': mod_models.MaintenanceTask.objects.get(task_id=task_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class MaintenanceTaskListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Meta',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_maintenance_tasks_filters(request),
            'onload_url': '/ui/meta/ajax/maintenance_tasks',
            'title': 'MaintenanceTasks',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class MaintenanceTaskListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_maintenance_tasks_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'meta_maintenance_tasks',
                'array_name': 'meta_maintenance_tasks_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.MaintenanceTask.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('meta/maintenance_tasks/maintenance_tasks.html', ctx, request)
        return http.JsonResponse(response)
