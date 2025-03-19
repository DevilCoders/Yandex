"""Test auth in bootstrap.api"""


import flask_restplus

from bootstrap.api.routes import api as flask_restplus_api
from bootstrap.api.routes.admin import Health

_ALLOW_NO_AUTH_HEADER_RESOURCES = [
    Health,
]


def test_authentification_header_present():
    """CLOUD-31692: check if authentification header is prestent in EVERY request"""
    for resource, _, _, _ in flask_restplus_api.resources:
        if resource in _ALLOW_NO_AUTH_HEADER_RESOURCES:
            continue

        for method_name in ("get", "post", "patch", "delete"):
            if not hasattr(resource, method_name):
                continue
            method = getattr(resource, method_name)

            flask_parsers_args = []
            for may_be_parser in method.__apidoc__.get("expect", []):
                if isinstance(may_be_parser, flask_restplus.reqparse.RequestParser):
                    flask_parsers_args.extend(may_be_parser.args)

            if ("headers", "Authorization") not in [(arg.location, arg.name) for arg in flask_parsers_args]:
                raise Exception("Method <{}> for resource {} does not have auth headers".format(
                    method_name, resource
                ))
