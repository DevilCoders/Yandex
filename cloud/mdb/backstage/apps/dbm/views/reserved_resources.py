import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.dbm.models as mod_models
import cloud.mdb.backstage.apps.dbm.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ReservedResourceListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'DBM',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_reserved_resources_filters(request),
            'onload_url': '/ui/dbm/ajax/reserved_resources',
            'title': 'ReservedResources',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ReservedResourceListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_reserved_resources_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'dbm_reserved_resources',
                'array_name': 'dbm_reserved_resources_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.ReservedResource.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('dbm/reserved_resources/reserved_resources.html', ctx, request)
        return http.JsonResponse(response)
