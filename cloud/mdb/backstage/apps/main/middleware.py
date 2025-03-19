import logging

from django.conf import settings
import django.http
import django.utils.timezone as timezone

import cloud.mdb.backstage.apps.main.profile as mod_profile


logger = logging.getLogger('backstage.main.middleware')


class UsagePermissionMiddleware:
    def __init__(self, get_response):
        self.get_response = get_response

    def __call__(self, request):
        if request.path.startswith('/ping/'):
            return self.get_response(request)
        if settings.CONFIG.iam.get('debug', False):
            if request.path.startswith('/auth/debug') or request.path.startswith('/auth/ajax/debug'):
                return self.get_response(request)
        if request.iam_user.is_authenticated():
            if not request.iam_user.has_perm(settings.CONFIG.iam.permissions.usage_permission):
                return django.http.HttpResponse(f"ACCESS DENIED for user {request.iam_user.sub}", status=403)
        return self.get_response(request)


class UserProfileMiddleware:
    def __init__(self, get_response):
        self.get_response = get_response

    def __call__(self, request):
        self.assign_user_profile(request)
        return self.get_response(request)

    def assign_user_profile(self, request):
        request.iam_user.__class__.profile = mod_profile.ProfileDescriptor()
        request.user.__class__.profile = mod_profile.ProfileDescriptor()


class UserTimezoneMiddleware:
    def __init__(self, get_response):
        self.get_response = get_response

    def __call__(self, request):
        try:
            timezone.activate(request.iam_user.profile.timezone)
        except Exception as err:
            logger.exception(f'failed to activate timezone: {err}')
        return self.get_response(request)
