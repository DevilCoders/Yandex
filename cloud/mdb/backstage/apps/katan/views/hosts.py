import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.katan.models as mod_models
import cloud.mdb.backstage.apps.katan.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class HostSkelView(django.views.generic.View):
    def get(self, request, fqdn, section='common'):
        host = mod_models.Host.objects.get(pk=fqdn)
        ctx = {
            'menu': 'Katan',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': host,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class HostSectionView(django.views.generic.View):
    def get(self, request, fqdn, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(fqdn)
        response.html = mod_helpers.render_html(f'katan/hosts/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, fqdn):
        return {'obj': mod_models.Host.objects.get(pk=fqdn)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class HostBlockView(django.views.generic.View):
    def get(self, request, fqdn, block):
        BLOCKS = {
            'shipments': self.get_shipments_context,
        }
        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(fqdn)
        response.html = mod_helpers.render_html(f'katan/hosts/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_shipments_context(self, fqdn):
        shipments = mod_models.RolloutShipment.objects\
            .select_related('rollout')\
            .filter(fqdn=fqdn)\
            .order_by('shipment_id')
        return {'shipments': shipments}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class HostListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Katan',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_hosts_filters(request),
            'onload_url': '/ui/katan/ajax/hosts',
            'title': 'Hosts',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class HostListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_hosts_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'katan_hosts',
                'array_name': 'katan_hosts_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Host.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('katan/hosts/hosts.html', ctx, request)
        return http.JsonResponse(response)
