from django.conf import settings
import django.db

import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.apps as apps
import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.cms.models as mod_models
import cloud.mdb.backstage.apps.cms.filters as mod_filters
import cloud.mdb.backstage.apps.cms.actions.decisions as mod_actions


if apps.CMS.is_enabled:
    CMS_ADMIN_PERM = settings.CONFIG.apps.cms.admin_permission
else:
    CMS_ADMIN_PERM = None


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class DecisionSkelView(django.views.generic.View):
    def get(self, request, decision_id, section='common'):
        decision = mod_models.Decision.ui_obj.get(pk=decision_id)
        ctx = {
            'menu': 'CMS',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': decision,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class DecisionSectionView(django.views.generic.View):
    def get(self, request, decision_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(decision_id, request)
        response.html = mod_helpers.render_html(f'cms/decisions/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, decision_id, request):
        if request.iam_user.has_perm(CMS_ADMIN_PERM):
            actions = mod_models.DecisionAction.all
        else:
            actions = []

        return {
            'obj': mod_models.Decision.ui_obj.get(pk=decision_id),
            'actions': actions,
        }


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class DecisionListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'CMS',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_decisions_filters(request),
            'onload_url': '/ui/cms/ajax/decisions',
            'title': 'Dom0 decisions',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class DecisionListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_decisions_filters(request)

        if request.iam_user.has_perm(CMS_ADMIN_PERM):
            actions = mod_models.DecisionAction.all
        else:
            actions = []

        ctx = {
            'filters': filters,
            'model': mod_models.Decision,
            'actions': actions,
            'js': {
                'identifier': 'cms_decisions',
                'array_name': 'cms_decisions_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.Decision.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('cms/decisions/decisions.html', ctx, request)
        return http.JsonResponse(response)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
@django.utils.decorators.method_decorator(iam_decorators.permission_required(CMS_ADMIN_PERM), name='dispatch')
class DecisionDialogView(django.views.generic.View):
    def post(self, request, action):
        response = mod_response.Response()

        if action not in mod_models.DecisionAction.map.keys():
            response.mark_failed(f'Unknown action: {action}')

        else:
            post_data = mod_helpers.get_post_data(request)

            objects = mod_models.Decision.ui_list.filter(pk__in=post_data['ids'])

            ctx = {
                'action': mod_models.DecisionAction.map[action],
                'action_ability': all([o.action_ability(action)[0] for o in objects]),
                'objects': objects,
                'js': {
                    'array_name': post_data['array_name'],
                    'app': post_data['app'],
                    'model': post_data['model'],
                },
            }

            response.html = mod_helpers.render_html('cms/decisions/action_dialog.html', ctx, request)
        return http.JsonResponse(response)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
@django.utils.decorators.method_decorator(iam_decorators.permission_required(CMS_ADMIN_PERM), name='dispatch')
class DecisionActionView(django.views.generic.View):
    def post(self, request, action):
        response = mod_response.Response()

        if action not in mod_models.DecisionAction.map.keys():
            response.mark_failed(f'Unknown action: {action}')
        else:
            post_data = mod_helpers.get_post_data(request)

            results = mod_actions.process_action(
                objects=mod_models.Decision.ui_list.filter(pk__in=post_data['ids']),
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
