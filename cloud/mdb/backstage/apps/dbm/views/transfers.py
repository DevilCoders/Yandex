import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.dbm.models as mod_models
import cloud.mdb.backstage.apps.dbm.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class TransferSkelView(django.views.generic.View):
    def get(self, request, id, section='common'):
        transfer = mod_models.Transfer.ui_obj.get(pk=id)
        ctx = {
            'menu': 'DBM',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': transfer,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class TransferSectionView(django.views.generic.View):
    def get(self, request, id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(id)
        response.html = mod_helpers.render_html(f'dbm/transfers/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, id):
        return {'obj': mod_models.Transfer.ui_obj.get(pk=id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class TransferListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'DBM',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_transfers_filters(request),
            'onload_url': '/ui/dbm/ajax/transfers',
            'title': 'Transfers',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class TransferListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_transfers_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'dbm_transfers',
                'array_name': 'dbm_transfers_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Transfer.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('dbm/transfers/transfers.html', ctx, request)
        return http.JsonResponse(response)
