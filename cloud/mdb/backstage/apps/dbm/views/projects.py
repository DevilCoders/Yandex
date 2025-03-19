import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.dbm.models as mod_models
import cloud.mdb.backstage.apps.dbm.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ProjectSkelView(django.views.generic.View):
    def get(self, request, name, section='common'):
        project = mod_models.Project.ui_obj.get(pk=name)
        ctx = {
            'menu': 'DBM',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': project,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ProjectSectionView(django.views.generic.View):
    def get(self, request, name, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(name)
        response.html = mod_helpers.render_html(f'dbm/projects/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, name):
        return {'obj': mod_models.Project.ui_obj.get(pk=name)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ProjectBlockView(django.views.generic.View):
    def get(self, request, name, block):
        BLOCKS = {
            'dom0_hosts': self.get_dom0_hosts_context,
        }
        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(name)
        response.html = mod_helpers.render_html(f'dbm/projects/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_dom0_hosts_context(self, name):
        return {'hosts': mod_models.Dom0Host.ui_list.filter(project__name=name)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ProjectListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'DBM',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_projects_filters(request),
            'onload_url': '/ui/dbm/ajax/projects',
            'title': 'Projects',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ProjectListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_projects_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'dbm_projects',
                'array_name': 'dbm_projects_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Project.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('dbm/projects/projects.html', ctx, request)
        return http.JsonResponse(response)
