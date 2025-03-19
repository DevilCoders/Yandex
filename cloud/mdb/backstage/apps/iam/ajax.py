import copy
import hashlib

from django.conf import settings
import django.http as http

import cloud.mdb.backstage.lib.apps as mod_apps
import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.iam.session_service
import cloud.mdb.backstage.apps.iam.access_service


session_service = cloud.mdb.backstage.apps.iam.session_service.SessionService()
access_service = cloud.mdb.backstage.apps.iam.access_service.AccessService()


def debug_config(request):
    response = mod_response.Response()
    config = copy.deepcopy(settings.CONFIG.iam)
    if config['client_secret']:
        try:
            client_secret = config['client_secret'].encode('utf-8')
        except AttributeError:
            client_secret = config['client_secret']
        md5 = hashlib.md5(client_secret).hexdigest()
        client_secret = f'md5:{md5}'
    else:
        client_secret = None

    config['client_secret'] = client_secret

    ctx = {
        'config': config
    }
    response.html = mod_helpers.render_html('iam/ajax/debug_config.html', ctx, request)
    return http.JsonResponse(response)


def debug_auth_info(request):
    response = mod_response.Response()

    data = {
        'user': {
            'is_authenticated': request.iam_user.is_authenticated(),
            'login': request.iam_user.login,
            'sub': request.iam_user.sub,
            'federation': request.iam_user.federation,
        },
        'permissions': {
            'usage_permission': {
                'name': settings.CONFIG.iam.permissions.usage_permission,
                'result': bool(request.iam_user.has_perm(settings.CONFIG.iam.permissions.usage_permission)),
            }
        }
    }
    if mod_apps.META.is_enabled:
        data['permissions']['meta.admin_permission'] = {
            'name': settings.CONFIG.apps.meta.admin_permission,
            'result': bool(request.iam_user.has_perm(settings.CONFIG.apps.meta.admin_permission)),
        }
    if mod_apps.CMS.is_enabled:
        data['permissions']['cms.admin_permission'] = {
            'name': settings.CONFIG.apps.cms.admin_permission,
            'result': bool(request.iam_user.has_perm(settings.CONFIG.apps.cms.admin_permission)),
        }

    ctx = {
        'data': data,
    }
    response.html = mod_helpers.render_html('iam/ajax/debug_auth_info.html', ctx, request)
    return http.JsonResponse(response)
