from django.conf import settings
import collections

import django.db
import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.apps as apps
import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.meta.models as meta_models

import cloud.mdb.backstage.apps.cms.models as mod_models
import cloud.mdb.backstage.apps.cms.filters as mod_filters
import cloud.mdb.backstage.apps.cms.actions.instance_operations as mod_actions


if apps.CMS.is_enabled:
    CMS_ADMIN_PERM = settings.CONFIG.apps.cms.admin_permission
else:
    CMS_ADMIN_PERM = None


def attach_cluster(instance_operations):
    if not instance_operations:
        return None

    clusters_map = collections.defaultdict()
    fqdns = set()
    for io in instance_operations:
        if io.fqdn:
            fqdns.add(io.fqdn)

    for host in meta_models.Host.objects.select_related('subcluster__cluster').filter(fqdn__in=fqdns):
        clusters_map[host.fqdn] = host.subcluster.cluster

    for io in instance_operations:
        io.cluster = clusters_map.get(io.fqdn)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class InstanceOperationSkelView(django.views.generic.View):
    def get(self, request, instance_operation_id, section='common'):
        instance_operation = mod_models.InstanceOperation.ui_obj.get(pk=instance_operation_id)
        ctx = {
            'menu': 'CMS',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': instance_operation,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class InstanceOperationSectionView(django.views.generic.View):
    def get(self, request, instance_operation_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(instance_operation_id, request)
        response.html = mod_helpers.render_html(f'cms/instance_operations/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, instance_operation_id, request):
        if request.iam_user.has_perm(CMS_ADMIN_PERM):
            actions = mod_models.InstanceOperationAction.all
        else:
            actions = []

        instance_operation = mod_models.InstanceOperation.ui_obj.get(pk=instance_operation_id)
        attach_cluster([instance_operation])

        return {
            'obj': instance_operation,
            'actions': actions,
        }


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class InstanceOperationListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'CMS',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_instance_operations_filters(request),
            'onload_url': '/ui/cms/ajax/instance_operations',
            'title': 'Instance operations',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class InstanceOperationListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_instance_operations_filters(request)

        if request.iam_user.has_perm(CMS_ADMIN_PERM):
            actions = mod_models.InstanceOperationAction.all
        else:
            actions = []

        ctx = {
            'filters': filters,
            'model': mod_models.InstanceOperation,
            'actions': actions,
            'js': {
                'callback_func': 'activate_action_buttons_panel',
                'identifier': 'cms_instance_operations',
                'array_name': 'cms_instance_operations_selected',

            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.InstanceOperation.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )
            attach_cluster(ctx['objects'])

        response.html = mod_helpers.render_html('cms/instance_operations/instance_operations.html', ctx, request)
        return http.JsonResponse(response)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
@django.utils.decorators.method_decorator(iam_decorators.permission_required(CMS_ADMIN_PERM), name='dispatch')
class InstanceOperationDialogView(django.views.generic.View):
    def post(self, request, action):
        response = mod_response.Response()

        if action not in mod_models.InstanceOperationAction.map.keys():
            response.mark_failed(f'Unknown action: {action}')

        else:
            post_data = mod_helpers.get_post_data(request)

            objects = mod_models.InstanceOperation.ui_list.filter(pk__in=post_data['ids'])

            ctx = {
                'action': mod_models.InstanceOperationAction.map[action],
                'action_ability': all([o.action_ability(action)[0] for o in objects]),
                'objects': objects,
                'js': {
                    'array_name': post_data['array_name'],
                    'app': post_data['app'],
                    'model': post_data['model'],
                },
            }

            response.html = mod_helpers.render_html('cms/instance_operations/action_dialog.html', ctx, request)
        return http.JsonResponse(response)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
@django.utils.decorators.method_decorator(iam_decorators.permission_required(CMS_ADMIN_PERM), name='dispatch')
class InstanceOperationActionView(django.views.generic.View):
    def post(self, request, action):
        response = mod_response.Response()

        if action not in mod_models.InstanceOperationAction.map.keys():
            response.mark_failed(f'Unknown action: {action}')
        else:
            post_data = mod_helpers.get_post_data(request)

            results = mod_actions.process_action(
                objects=mod_models.InstanceOperation.ui_list.filter(pk__in=post_data['ids']),
                action=action,
                action_params=post_data['inputs'],
                username=request.iam_user.login,
                client_ip=mod_helpers.get_client_ip(request),
                request_id=mod_helpers.get_request_id(request),
            )

            for result in results:
                if not result:
                    response.mark_failed(result.obj_error_html, safe=True)
                else:
                    response.add_msg(result.obj_msg_html, safe=True)

        return http.JsonResponse(response)
