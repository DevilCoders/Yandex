import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.deploy.models as mod_models
import cloud.mdb.backstage.apps.deploy.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShipmentCommandSkelView(django.views.generic.View):
    def get(self, request, shipment_command_id, section='common'):
        shipment_command = mod_models.ShipmentCommand.objects.get(pk=shipment_command_id)
        ctx = {
            'menu': 'Deploy',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': shipment_command,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShipmentCommandSectionView(django.views.generic.View):
    def get(self, request, shipment_command_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(shipment_command_id)
        response.html = mod_helpers.render_html(f'deploy/shipment_commands/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, shipment_command_id):
        return {'obj': mod_models.ShipmentCommand.objects.get(pk=shipment_command_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShipmentCommandBlockView(django.views.generic.View):
    def get(self, request, shipment_command_id, block):
        BLOCKS = {
            'job_results': self.get_job_results_context,
            'commands': self.get_commands_context,
        }
        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(shipment_command_id)
        response.html = mod_helpers.render_html(f'deploy/shipment_commands/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_job_results_context(self, shipment_command_id):
        shipment_command = mod_models.ShipmentCommand.objects.get(pk=shipment_command_id)
        return {'job_results': shipment_command.job_results}

    def get_commands_context(self, shipment_command_id):
        commands = mod_models.Command.ui_list\
            .filter(shipment_command__pk=shipment_command_id)\
            .prefetch_related('jobs')

        job_results_map = {}
        ext_job_ids = []

        for command in commands:
            for job in command.jobs.all():
                ext_job_ids.append(job.ext_job_id)

        for job_result in mod_models.JobResult.objects.filter(ext_job_id__in=ext_job_ids):
            job_results_map[job_result.ext_job_id] = job_result

        data = []
        for command in commands:
            jobs = []
            for job in command.jobs.all():
                jobs.append({
                    'obj': job,
                    'job_result': job_results_map.get(job.ext_job_id)
                })
            data.append({
                'obj': command,
                'jobs': jobs,
            })
        return {'commands': data}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShipmentCommandListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Deploy',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_shipment_commands_filters(request),
            'onload_url': '/ui/deploy/ajax/shipment_commands',
            'title': 'ShipmentCommands',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShipmentCommandListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_shipment_commands_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'deploy_shipment_commands',
                'array_name': 'deploy_shipment_commands_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.ShipmentCommand.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('deploy/shipment_commands/shipment_commands.html', ctx, request)
        return http.JsonResponse(response)
