import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.meta.models as mod_models
import cloud.mdb.backstage.apps.meta.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class FolderSkelView(django.views.generic.View):
    def get(self, request, folder_id, section='common'):
        folder = mod_models.Folder.objects.get(pk=folder_id)
        ctx = {
            'menu': 'Meta',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': folder,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class FolderSectionView(django.views.generic.View):
    def get(self, request, folder_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(folder_id)
        response.html = mod_helpers.render_html(f'meta/folders/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, folder_id):
        return {'obj': mod_models.Folder.objects.get(pk=folder_id)}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class FolderListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Meta',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_folders_filters(request),
            'onload_url': '/ui/meta/ajax/folders',
            'title': 'Folders',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class FolderListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_folders_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'meta_folders',
                'array_name': 'meta_folders_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Folder.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('meta/folders/folders.html', ctx, request)
        return http.JsonResponse(response)
