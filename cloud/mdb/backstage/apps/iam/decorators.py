import functools
import django.core.exceptions
import django.utils.decorators


import cloud.mdb.backstage.apps.iam.middleware as mod_middleware


auth_required = django.utils.decorators.decorator_from_middleware(
    mod_middleware.IAMAuthRequiredMiddleware
)


def permission_required(perm):
    def decorator(view_func):
        @functools.wraps(view_func)
        def _wrapped_view(request, *args, **kwargs):
            if request.iam_user.has_perm(perm):
                return view_func(request, *args, **kwargs)
            else:
                raise django.core.exceptions.PermissionDenied
        return _wrapped_view
    return decorator
