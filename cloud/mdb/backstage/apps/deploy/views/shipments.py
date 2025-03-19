import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.deploy.models as mod_models
import cloud.mdb.backstage.apps.deploy.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShipmentSkelView(django.views.generic.View):
    def get(self, request, shipment_id, section='common'):
        shipment = mod_models.Shipment.objects.get(pk=shipment_id)
        ctx = {
            'menu': 'Deploy',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': shipment,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShipmentSectionView(django.views.generic.View):
    def get(self, request, shipment_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(shipment_id)
        response.html = mod_helpers.render_html(f'deploy/shipments/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, shipment_id):
        return {'obj': mod_models.Shipment.objects.get(pk=shipment_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShipmentBlockView(django.views.generic.View):
    def get(self, request, shipment_id, block):
        BLOCKS = {
            'progress': self.get_progress_context,
            'minions': self.get_minions_context,
        }
        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(shipment_id)
        response.html = mod_helpers.render_html(f'deploy/shipments/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_progress_context(self, shipment_id):
        shipment = mod_models.Shipment.objects.prefetch_related('shipmentcommand_set').get(shipment_id=shipment_id)
        shipment_commands = shipment.shipmentcommand_set.all()
        commands = mod_models.Command.objects\
            .select_related('minion')\
            .select_related('shipment_command')\
            .prefetch_related('jobs')\
            .filter(shipment_command__in=shipment_commands)

        job_results_map = {}
        ext_job_ids = []

        for command in commands:
            for job in command.jobs.all():
                ext_job_ids.append(job.ext_job_id)

        for job_result in mod_models.JobResult.objects.filter(ext_job_id__in=ext_job_ids):
            job_results_map[job_result.ext_job_id] = job_result

        data = []
        minions = set([c.minion for c in commands])

        for minion in minions:
            minion_data = {
                'minion': minion,
                'commands': [],
                'stats': {},
            }
            for status in mod_models.CommandStatus.all:
                minion_data['stats'][status[0]] = 0

            for shipment_command in shipment_commands:
                minion_commands = filter(
                    lambda c: c.minion == minion and c.shipment_command == shipment_command,
                    commands,
                )
                for command in minion_commands:
                    command_data = {
                        'command': command,
                        'jobs': [],
                    }
                    for job in command.jobs.all():
                        command_data['jobs'].append({
                            'obj': job,
                            'job_result': job_results_map.get(job.ext_job_id)
                        })
                    minion_data['commands'].append(command_data)
                    minion_data['stats'][command.status] += 1

            minion_data['commands'] = sorted(minion_data['commands'], key=lambda i: i['command'].updated_at)
            data.append(minion_data)

        data = sorted(data, key=lambda i: i['minion'].fqdn)

        return {'data': data}

    def get_minions_context(self, shipment_id):
        shipment = mod_models.Shipment.objects.get(pk=shipment_id)
        minions = mod_models.Minion.objects.filter(fqdn__in=shipment.fqdns).order_by('fqdn')
        return {'minions': minions}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShipmentListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Deploy',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_shipments_filters(request),
            'onload_url': '/ui/deploy/ajax/shipments',
            'title': 'Shipments',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShipmentListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_shipments_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'deploy_shipments',
                'array_name': 'deploy_shipments_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Shipment.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('deploy/shipments/shipments.html', ctx, request)
        return http.JsonResponse(response)
