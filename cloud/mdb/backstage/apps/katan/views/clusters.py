import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.katan.models as mod_models
import cloud.mdb.backstage.apps.katan.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ClusterSkelView(django.views.generic.View):
    def get(self, request, cluster_id, section='common'):
        cluster = mod_models.Cluster.objects.get(pk=cluster_id)
        ctx = {
            'menu': 'Katan',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': cluster,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ClusterSectionView(django.views.generic.View):
    def get(self, request, cluster_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(cluster_id)
        response.html = mod_helpers.render_html(f'katan/clusters/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, cluster_id):
        return {'obj': mod_models.Cluster.objects.get(pk=cluster_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ClusterBlockView(django.views.generic.View):
    def get(self, request, cluster_id, block):
        BLOCKS = {
            'cluster_rollouts': self.get_cluster_rollouts_context,
        }
        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(cluster_id)
        response.html = mod_helpers.render_html(f'katan/clusters/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_cluster_rollouts_context(self, cluster_id):
        return {'cluster_rollouts': mod_models.ClusterRollout.ui_list.filter(cluster_id=cluster_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ClusterListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Katan',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_clusters_filters(request),
            'onload_url': '/ui/katan/ajax/clusters',
            'title': 'Clusters',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ClusterListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_clusters_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'katan_clusters',
                'array_name': 'katan_clusters_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Cluster.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('katan/clusters/clusters.html', ctx, request)
        return http.JsonResponse(response)
