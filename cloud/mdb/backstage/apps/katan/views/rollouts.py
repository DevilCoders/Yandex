import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.katan.models as mod_models
import cloud.mdb.backstage.apps.katan.filters as mod_filters

import cloud.mdb.backstage.apps.deploy.models as deploy_models


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class RolloutSkelView(django.views.generic.View):
    def get(self, request, rollout_id, section='common'):
        rollout = mod_models.Rollout.objects.get(pk=rollout_id)
        ctx = {
            'menu': 'Katan',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': rollout,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class RolloutSectionView(django.views.generic.View):
    def get(self, request, rollout_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(rollout_id)
        response.html = mod_helpers.render_html(f'katan/rollouts/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, rollout_id):
        return {'obj': mod_models.Rollout.objects.get(pk=rollout_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class RolloutBlockView(django.views.generic.View):
    def get(self, request, rollout_id, block):
        BLOCKS = {
            'cluster_rollouts': self.get_cluster_rollouts_context,
            'dependencies': self.get_dependencies_context,
            'shipments': self.get_shipments_context,
        }
        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(rollout_id)
        response.html = mod_helpers.render_html(f'katan/rollouts/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_cluster_rollouts_context(self, rollout_id):
        return {'cluster_rollouts': mod_models.ClusterRollout.ui_list.filter(rollout_id=rollout_id)}

    def get_dependencies_context(self, rollout_id):
        depends_on = mod_models.RolloutsDependency.objects\
            .filter(rollout_id=rollout_id)\
            .values_list('depends_on', flat=True)
        rollouts = mod_models.Rollout.ui_list.filter(pk__in=depends_on)
        return {'rollouts': rollouts}

    def get_shipments_context(self, rollout_id):
        rollout_shipments = mod_models.RolloutShipment.objects\
            .select_related('rollout')\
            .filter(rollout_id=rollout_id)

        shipments_ids = set([r.shipment_id for r in rollout_shipments])
        shipments_map = {}
        for shipment in deploy_models.Shipment.ui_list.filter(pk__in=shipments_ids):
            shipments_map[shipment.pk] = shipment

        data = []
        for rs in rollout_shipments:
            if rs.shipment_id not in shipments_map:
                continue
            data.append({
                'obj': rs,
                'shipment': shipments_map[rs.shipment_id],
            })
        data = sorted(data, key=lambda i: i['obj'].fqdn)
        return {'data': data}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class RolloutListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Katan',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_rollouts_filters(request),
            'onload_url': '/ui/katan/ajax/rollouts',
            'title': 'Rollouts',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class RolloutListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_rollouts_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'katan_rollouts',
                'array_name': 'katan_rollouts_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Rollout.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('katan/rollouts/rollouts.html', ctx, request)
        return http.JsonResponse(response)
