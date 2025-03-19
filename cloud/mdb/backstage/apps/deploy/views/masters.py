import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.deploy.models as mod_models
import cloud.mdb.backstage.apps.deploy.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class MasterSkelView(django.views.generic.View):
    def get(self, request, master_id, section='common'):
        master = mod_models.Master.objects.get(pk=master_id)
        ctx = {
            'menu': 'Deploy',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': master,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class MasterSectionView(django.views.generic.View):
    def get(self, request, master_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(master_id)
        response.html = mod_helpers.render_html(f'deploy/masters/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, master_id):
        return {'obj': mod_models.Master.objects.get(pk=master_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class MasterListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Deploy',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_masters_filters(request),
            'onload_url': '/ui/deploy/ajax/masters',
            'title': 'Masters',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class MasterListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_masters_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'deploy_masters',
                'array_name': 'deploy_masters_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Master.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('deploy/masters/masters.html', ctx, request)
        return http.JsonResponse(response)
