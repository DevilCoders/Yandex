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
class ShardSkelView(django.views.generic.View):
    def get(self, request, shard_id, section='common'):
        shard = mod_models.Shard.objects.get(pk=shard_id)
        ctx = {
            'menu': 'Meta',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': shard,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShardSectionView(django.views.generic.View):
    def get(self, request, shard_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(shard_id)
        response.html = mod_helpers.render_html(f'meta/shards/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, shard_id):
        return {'obj': mod_models.Shard.objects.get(pk=shard_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShardBlockView(django.views.generic.View):
    def get(self, request, shard_id, block):
        BLOCKS = {
            'pillar': self.get_pillar_context,
            'pillar_revs': self.get_pillar_revs_context,
        }
        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(shard_id)
        response.html = mod_helpers.render_html(f'meta/shards/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_pillar_revs_context(self, shard_id):
        return meta_helpers.get_pillar_revs_context(shard=shard_id)

    def get_pillar_context(self, shard_id):
        return meta_helpers.get_pillar_context(shard=shard_id)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShardListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Meta',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_shards_filters(request),
            'onload_url': '/ui/meta/ajax/shards',
            'title': 'Shards',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class ShardListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_shards_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'meta_shards',
                'array_name': 'meta_shards_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Shard.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('meta/shards/shards.html', ctx, request)
        return http.JsonResponse(response)
