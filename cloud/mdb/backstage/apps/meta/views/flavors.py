import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.meta.models as mod_models
import cloud.mdb.backstage.apps.meta.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class FlavorSkelView(django.views.generic.View):
    def get(self, request, id, section='common'):
        flavor = mod_models.Flavor.objects.get(pk=id)
        ctx = {
            'menu': 'Meta',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': flavor,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class FlavorSectionView(django.views.generic.View):
    def get(self, request, id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(id)
        response.html = mod_helpers.render_html(f'meta/flavors/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, id):
        return {'obj': mod_models.Flavor.objects.get(pk=id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class FlavorListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Meta',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_flavors_filters(request),
            'onload_url': '/ui/meta/ajax/flavors',
            'title': 'Flavors',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class FlavorListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_flavors_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'meta_flavors',
                'array_name': 'meta_flavors_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Flavor.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
                items_per_page=200,
            )

        response.html = mod_helpers.render_html('meta/flavors/flavors.html', ctx, request)
        return http.JsonResponse(response)
