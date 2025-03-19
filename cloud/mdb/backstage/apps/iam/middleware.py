from django.conf import settings
import django.http
import django.utils.deprecation

import cloud.mdb.backstage.apps.iam.user as iam_user


class IAMAuthMiddleware:
    def __init__(self, get_response):
        self.get_response = get_response

    def __call__(self, request):
        self.assign_iam_user(request)
        return self.get_response(request)

    def assign_iam_user(self, request):
        request.__class__.iam_user = iam_user.UserDescriptor()
        request.__class__.user = iam_user.UserDescriptor()


class IAMAuthTestMiddleware(IAMAuthMiddleware):
    def assign_iam_user(self, request):
        request.__class__.iam_user = iam_user.TestUserDescriptor()
        request.__class__.user = iam_user.TestUserDescriptor()


class IAMAuthRequiredMiddleware(django.utils.deprecation.MiddlewareMixin):
    """
    Old style middleware required: https://docs.djangoproject.com/en/3.2/ref/utils/#django.utils.decorators.decorator_from_middleware
    """
    def process_request(self, request):
        if not request.iam_user.is_authenticated():
            return django.http.HttpResponseRedirect(
                f'{request.iam_user._auth_url}?client_id={settings.CONFIG.iam.client_id}&client_secret={settings.CONFIG.iam.client_secret}&scope=openid&response_type=code'
            )
