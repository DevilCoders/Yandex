import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.deploy.models as mod_models
import cloud.mdb.backstage.apps.deploy.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class GroupSkelView(django.views.generic.View):
    def get(self, request, group_id, section='common'):
        group = mod_models.Group.objects.get(pk=group_id)
        ctx = {
            'menu': 'Deploy',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': group,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class GroupSectionView(django.views.generic.View):
    def get(self, request, group_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(group_id)
        response.html = mod_helpers.render_html(f'deploy/groups/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, group_id):
        return {'obj': mod_models.Group.objects.get(pk=group_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class GroupListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Deploy',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_groups_filters(request),
            'onload_url': '/ui/deploy/ajax/groups',
            'title': 'Groups',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class GroupListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_groups_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'deploy_groups',
                'array_name': 'deploy_groups_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Group.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('deploy/groups/groups.html', ctx, request)
        return http.JsonResponse(response)
