import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.dbm.models as mod_models
import cloud.mdb.backstage.apps.dbm.filters as mod_filters

import cloud.mdb.backstage.apps.meta.models as meta_models


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class Dom0HostSkelView(django.views.generic.View):
    def get(self, request, fqdn, section='common'):
        dom0_host = mod_models.Dom0Host.ui_obj.get(pk=fqdn)
        ctx = {
            'menu': 'DBM',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': dom0_host,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class Dom0HostSectionView(django.views.generic.View):
    def get(self, request, fqdn, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(fqdn)
        response.html = mod_helpers.render_html(f'dbm/dom0_hosts/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, fqdn):
        return {'obj': mod_models.Dom0Host.ui_obj.get(pk=fqdn)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class Dom0HostBlockView(django.views.generic.View):
    def get(self, request, fqdn, block):
        BLOCKS = {
            'containers': self.get_containers_context,
            'clusters': self.get_clusters_context,
            'resources': self.get_resources_context,
        }
        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(fqdn)
        response.html = mod_helpers.render_html(f'dbm/dom0_hosts/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_containers_context(self, fqdn):
        containers = mod_models.Container.objects.filter(dom0host__fqdn=fqdn).select_related('cluster')
        return {'containers': containers}

    def get_clusters_context(self, fqdn):
        clusters = []

        containers = mod_models.Container.objects\
            .filter(dom0host__fqdn=fqdn)\
            .select_related('cluster')

        clusters_names = set([c.cluster.name for c in containers])
        dbm_clusters = mod_models.Cluster.objects\
            .filter(name__in=clusters_names)

        containers_to_cluster = {}
        for container in containers:
            containers_list = containers_to_cluster.get(container.cluster.name, [])
            containers_list.append(container)
            containers_to_cluster[container.cluster.name] = containers_list

        meta_clusters = meta_models.Cluster.ui_list\
            .filter(pk__in=list(clusters_names))

        meta_clusters_map = {}
        for meta_cluster in meta_clusters:
            meta_clusters_map[meta_cluster.pk] = meta_cluster

        for dbm_cluster in dbm_clusters:
            clusters.append({
                'dbm': dbm_cluster,
                'meta': meta_clusters_map.get(dbm_cluster.name),
                'containers': containers_to_cluster.get(dbm_cluster.name, []),
            })

        return {'clusters': clusters}

    def get_resources_context(self, fqdn):
        dom0host = mod_models.Dom0Host.ui_obj.get(pk=fqdn)

        used = dom0host.resources_used
        resources = {
            'cpu': {
                'total': dom0host.cpu_cores,
                'used': used['cpu'],
                'free': dom0host.cpu_cores - used['cpu'],
            },
            'memory': {
                'total': dom0host.memory,
                'used': used['memory'],
                'free': dom0host.memory - used['memory'],
            },
            'net': {
                'total': dom0host.net_speed,
                'used': used['net'],
                'free': dom0host.net_speed - used['net'],
            },
            'io': {
                'total': dom0host.max_io,
                'used': used['io'],
                'free': dom0host.max_io - used['io'],
            },
            'ssd': {
                'total': dom0host.ssd_space,
                'used': used['ssd'],
                'free': dom0host.ssd_space - used['ssd'],
            },
            'sata': {
                'total': dom0host.sata_space,
                'used': used['sata'],
                'free': dom0host.sata_space - used['sata'],
            },
        }
        return {'resources': resources}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class Dom0HostListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'DBM',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_dom0_hosts_filters(request),
            'onload_url': '/ui/dbm/ajax/dom0_hosts',
            'title': 'Dom0Hosts',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class Dom0HostListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_dom0_hosts_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'dbm_dom0_hosts',
                'array_name': 'dbm_dom0_hosts_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Dom0Host.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('dbm/dom0_hosts/dom0_hosts.html', ctx, request)
        return http.JsonResponse(response)
