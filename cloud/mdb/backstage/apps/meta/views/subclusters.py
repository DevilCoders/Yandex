import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.meta.models as mod_models
import cloud.mdb.backstage.apps.meta.filters as mod_filters
import cloud.mdb.backstage.apps.meta.helpers as meta_helpers


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class SubclusterSkelView(django.views.generic.View):
    def get(self, request, subcid_id, section='common'):
        subcluster = mod_models.Subcluster.objects.get(pk=subcid_id)
        ctx = {
            'menu': 'Meta',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': subcluster,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class SubclusterSectionView(django.views.generic.View):
    def get(self, request, subcid_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(subcid_id)
        response.html = mod_helpers.render_html(f'meta/subclusters/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, subcid_id):
        return {'obj': mod_models.Subcluster.objects.get(pk=subcid_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class SubclusterBlockView(django.views.generic.View):
    def get(self, request, subcid_id, block):
        BLOCKS = {
            'pillar': self.get_pillar_context,
            'pillar_revs': self.get_pillar_revs_context,
        }
        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(subcid_id)
        response.html = mod_helpers.render_html(f'meta/subclusters/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_pillar_revs_context(self, subcid_id):
        return meta_helpers.get_pillar_revs_context(subcid=subcid_id)

    def get_pillar_context(self, subcid_id):
        return meta_helpers.get_pillar_context(subcid=subcid_id)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class SubclusterListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Meta',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_subclusters_filters(request),
            'onload_url': '/ui/meta/ajax/subclusters',
            'title': 'Subclusters',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class SubclusterListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_subclusters_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'meta_subclusters',
                'array_name': 'meta_subclusters_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Subcluster.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('meta/subclusters/subclusters.html', ctx, request)
        return http.JsonResponse(response)
