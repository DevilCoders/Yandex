import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.deploy.models as mod_models
import cloud.mdb.backstage.apps.deploy.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class CommandSkelView(django.views.generic.View):
    def get(self, request, command_id, section='common'):
        command = mod_models.Command.objects.get(pk=command_id)
        ctx = {
            'menu': 'Deploy',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': command,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class CommandSectionView(django.views.generic.View):
    def get(self, request, command_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(command_id)
        response.html = mod_helpers.render_html(f'deploy/commands/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, command_id):
        return {'obj': mod_models.Command.ui_obj.get(pk=command_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class CommandBlockView(django.views.generic.View):
    def get(self, request, command_id, block):
        BLOCKS = {
            'jobs': self.get_jobs_context,
        }
        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(command_id)
        response.html = mod_helpers.render_html(f'deploy/commands/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_jobs_context(self, command_id):
        command = mod_models.Command.ui_obj.get(pk=command_id)
        return {'jobs': command.jobs.order_by('-created_at').all()}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class CommandListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Deploy',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_commands_filters(request),
            'onload_url': '/ui/deploy/ajax/commands',
            'title': 'Commands',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class CommandListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_commands_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'deploy_commands',
                'array_name': 'deploy_commands_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Command.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('deploy/commands/commands.html', ctx, request)
        return http.JsonResponse(response)
