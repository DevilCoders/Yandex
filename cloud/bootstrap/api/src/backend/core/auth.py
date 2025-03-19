"""Basic authentification via oauth"""

import functools
from typing import Optional

import blackbox
from tvmauth import BlackboxTvmId as BlackboxClientId
import flask
import flask_restplus
from schematics.models import Model
from schematics.types import StringType, BooleanType
from schematics.exceptions import ValidationError

from bootstrap.common.exceptions import BootstrapError


OAUTH_HEADER_NAME = "Authorization"
NOAUTH_FAKE_USER = "noauth_fake_user"  # when auth is disabled all requests are treated as requests from special user
BOOTSTRAP_OAUTH_APP_ID = "d549185fd4244a928f17ab2f128bf189"  # auth client id


class NotAuthorizedError(BootstrapError):
    pass


class AuthorizerConfig(Model):
    enabled = BooleanType(default=True)
    service_id = StringType(default=BOOTSTRAP_OAUTH_APP_ID)
    blackbox_url = StringType(default=None)

    # tvm-related stuff
    tvm_client_id = StringType(default=None)
    tvm_secret = StringType(default=None)

    def validate_tvm_destination(self, data, value):
        if data["enabled"]:
            for attr in ("blackbox_url", "tvm_client_id", "tvm_secret",):
                if not data[attr]:
                    raise ValidationError("Auth is enabled, but field <{}> is not specified in config".format(attr))
        return value


class Authorizer:
    def __init__(self, config: AuthorizerConfig):
        self.enabled = config.enabled
        self.service_id = config.service_id
        if self.enabled:
            self.blackbox = blackbox.JsonBlackbox(
                url=config.blackbox_url, tvm2_client_id=config.tvm_client_id, tvm2_secret=config.tvm_secret,
                blackbox_client=BlackboxClientId.ProdYateam,
            )

    def auth_request(self, request) -> Optional[str]:
        """Authentificate request, return login"""
        if not self.enabled:
            return None

        if OAUTH_HEADER_NAME not in request.headers:
            raise NotAuthorizedError("Missing authorization header <{}>".format(OAUTH_HEADER_NAME))

        bb_reply = self.blackbox.oauth(
            headers={OAUTH_HEADER_NAME: request.headers[OAUTH_HEADER_NAME]},
            userip=request.remote_addr,
        )

        if bb_reply["status"]["value"] != "VALID":
            raise NotAuthorizedError("OAuth authentification failed: {}".format(bb_reply["error"]))
        if bb_reply["oauth"]["client_id"] != self.service_id:
            raise NotAuthorizedError("OAuth authentifaction failed: got token for another service")

        return bb_reply["login"]


def _get_auth_parser() -> flask_restplus.reqparse.RequestParser:
    auth_parser = flask_restplus.reqparse.RequestParser()

    request_token_url = "https://oauth.yandex-team.ru/authorize?response_type=token&client_id={}".format(BOOTSTRAP_OAUTH_APP_ID)
    authorization_help = "OAuth &lt;token&gt; (token requested <a href='{}'>HERE</a>).".format(request_token_url)
    auth_parser.add_argument(OAUTH_HEADER_NAME, help=authorization_help, location="headers")

    return auth_parser


def require_oauth_token(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        flask.request.authentificated_user = flask.current_app.authner.auth_request(flask.request)
        if flask.request.authentificated_user is None:
            flask.request.authentificated_user = NOAUTH_FAKE_USER

        return func(*args, **kwargs)

    wrapper.__apidoc__ = getattr(func, "__apidoc__", {})
    wrapper.__apidoc__.setdefault("expect", []).append(_get_auth_parser())

    return wrapper
