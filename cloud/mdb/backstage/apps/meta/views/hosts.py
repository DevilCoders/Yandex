import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.search as mod_search
import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.mlock.lock as mod_lock

import cloud.mdb.backstage.apps.meta.models as mod_models
import cloud.mdb.backstage.apps.meta.helpers as meta_helpers
import cloud.mdb.backstage.apps.meta.filters as mod_filters


HOST_REV_MAX_LIMIT = 100


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class HostSkelView(django.views.generic.View):
    def get(self, request, fqdn, section='common'):
        host = mod_models.Host.ui_obj.get(pk=fqdn)
        ctx = {
            'menu': 'Meta',
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
        response.html = mod_helpers.render_html(f'meta/hosts/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, fqdn):
        return {'obj': mod_models.Host.ui_obj.get(pk=fqdn)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class HostBlockView(django.views.generic.View):
    def get(self, request, fqdn, block):
        BLOCKS = {
            'pillar': self.get_pillar_context,
            'pillar_revs': self.get_pillar_revs_context,
            'revs': self.get_revs_context,
            'cms_last_decision': self.get_cms_last_decision_context,
            'mlock': self.get_mlock_context,
        }
        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(fqdn)
        response.html = mod_helpers.render_html(f'meta/hosts/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_pillar_context(self, fqdn):
        return meta_helpers.get_pillar_context(fqdn=fqdn)

    def get_pillar_revs_context(self, fqdn):
        return meta_helpers.get_pillar_revs_context(fqdn=fqdn)

    def get_cms_last_decision_context(self, fqdn):
        return {'decision': mod_search.get_cms_last_decision(fqdn=fqdn)}

    def get_revs_context(self, fqdn):
        revs = mod_models.HostRev.objects\
            .select_related('subcluster')\
            .select_related('shard')\
            .select_related('geo')\
            .select_related('disk_type')\
            .select_related('flavor')\
            .filter(fqdn=fqdn)\
            .order_by('-rev')[:HOST_REV_MAX_LIMIT]
        return {'revs': revs}

    def get_mlock_context(self, fqdn):
        return {'lock': mod_lock.Lock.get_lock_for_fqdn(fqdn)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class HostListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Meta',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_hosts_filters(request),
            'onload_url': '/ui/meta/ajax/hosts',
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
                'identifier': 'meta_hosts',
                'array_name': 'meta_hosts_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Host.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('meta/hosts/hosts.html', ctx, request)
        return http.JsonResponse(response)
