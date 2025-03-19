"""Auxiliarily classes for resources"""

import flask_restplus
import flask.views

from bootstrap.api.core.auth import require_oauth_token


class RequireOAuthToken(type):
    def __new__(cls, clsname, bases, dct):
        newclass = super(RequireOAuthToken, cls).__new__(cls, clsname, bases, dct)

        for method_name in ("get", "post", "patch", "put", "delete"):
            method = getattr(newclass, method_name, None)
            if not method:
                continue
            setattr(newclass, method_name, require_oauth_token(method))

        return newclass


class RequireOAuthTokenMetaHelper(RequireOAuthToken, flask.views.MethodViewType, flask.views.View):
    """Helper class to avoid bug with metaclasses do not origin from same base class"""
    pass


class BootstrapResource(flask_restplus.Resource, metaclass=RequireOAuthTokenMetaHelper):
    pass
