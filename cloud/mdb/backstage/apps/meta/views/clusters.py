import json

import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.health as mod_health
import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.meta.models as mod_models
import cloud.mdb.backstage.apps.meta.filters as mod_filters
import cloud.mdb.backstage.apps.meta.helpers as meta_helpers


HEALTH = mod_health.HealthApi()


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ClusterSkelView(django.views.generic.View):
    def get(self, request, cluster_id, section='common'):
        cluster = mod_models.Cluster.ui_obj.get(pk=cluster_id)
        ctx = {
            'menu': 'Meta',
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
        response.html = mod_helpers.render_html(f'meta/clusters/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, cluster_id):
        return {'obj': mod_models.Cluster.ui_obj.prefetch_related('subclusters').get(pk=cluster_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ClusterBlockView(django.views.generic.View):
    WORKER_TASKS_LIMIT = 100

    def get(self, request, cluster_id, block):
        BLOCKS = {
            'versions': self.get_versions_context,
            'revs': self.get_revs_context,
            'maintenance_tasks': self.get_maintenance_tasks_context,
            'worker_tasks': self.get_worker_tasks_context,
            'pillar': self.get_pillar_context,
            'pillar_revs': self.get_pillar_revs_context,
            'hosts_health': self.get_hosts_health_context,
            'cluster_health': self.get_cluster_health_context,
            'subclusters': self.get_subclusters_context,
        }

        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(cluster_id)
        response.html = mod_helpers.render_html(f'meta/clusters/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_versions_context(self, cluster_id):
        return {'versions': mod_models.Version.objects.filter(cluster_id=cluster_id)}

    def get_revs_context(self, cluster_id):
        revisions = mod_models.ClusterRev.objects.filter(cid=cluster_id)\
            .order_by('-rev')\
            .values('rev', 'name', 'network_id', 'folder_id', 'description', 'status')[:100]
        return {'revisions': revisions}

    def get_maintenance_tasks_context(self, cluster_id):
        return {'maintenance_tasks': mod_models.MaintenanceTask.objects.filter(cluster_id=cluster_id)}

    def get_worker_tasks_context(self, cluster_id):
        return {
            'worker_tasks': mod_models.WorkerTask.ui_list.filter(cluster__cid=cluster_id)[:self.WORKER_TASKS_LIMIT]
        }

    def get_pillar_context(self, cluster_id):
        pillar = mod_models.Pillar.objects.filter(cid=cluster_id)[0]
        return {'pillar': json.dumps(pillar.value, sort_keys=True, indent=4)}

    def get_pillar_revs_context(self, cluster_id):
        return meta_helpers.get_pillar_revs_context(pillar_revs_limit=150, cid=cluster_id)

    def get_subclusters_context(self, cluster_id):
        return {'subclusters': mod_models.Subcluster.objects.filter(cluster_id=cluster_id)}

    def get_hosts_health_context(self, cluster_id):
        hosts_map = {}
        cluster = mod_models.Cluster.objects.prefetch_related('subclusters').get(pk=cluster_id)
        hosts = cluster.hosts

        for host in hosts:
            hosts_map[host.fqdn] = host

        health_data, url = HEALTH.hostshealth([h.fqdn for h in hosts])
        for item in health_data:
            host = hosts_map.get(item['fqdn'])
            if host:
                if host.shard:
                    shard = host.shard.name
                else:
                    shard = ''
            else:
                shard = ''

            item['shard'] = shard

        try:
            health_data = sorted(
                health_data,
                key=lambda i: int(i['shard'].split('shard')[1]) if i['shard'] else 0,
            )
        except Exception:
            health_data = sorted(health_data, key=lambda i: i['shard'])

        return {
            'url': url,
            'health_data': health_data,
        }

    def get_cluster_health_context(self, cluster_id):
        health_data, url = HEALTH.clusterhealth(cluster_id)
        return {
            'url': url,
            'health_data': health_data,
        }


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ClusterListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Meta',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_clusters_filters(request),
            'onload_url': '/ui/meta/ajax/clusters',
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
                'identifier': 'meta_clusters',
                'array_name': 'meta_clusters_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Cluster.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('meta/clusters/clusters.html', ctx, request)
        return http.JsonResponse(response)
