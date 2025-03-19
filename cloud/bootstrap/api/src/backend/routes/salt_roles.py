"""Salt roles blueprint (CLOUD-36446)"""

import http
from typing import Any, Tuple

import flask
import flask_restplus

from bootstrap.api.core.salt_roles import get_all_salt_roles, get_salt_role, add_salt_role, delete_salt_role

from .helpers.aux_resource import BootstrapResource
from .helpers.aux_response import make_response
from .helpers.models import empty_model, add_salt_role_model, salt_role_model

api = flask_restplus.Namespace("salt_roles", description="Salt roles api")

api.add_model(empty_model.name, empty_model)
api.add_model(add_salt_role_model.name, add_salt_role_model)
api.add_model(salt_role_model.name, salt_role_model)


@api.route("")
class SaltRoles(BootstrapResource):
    @make_response(api, salt_role_model, as_list=True)
    def get(self) -> Tuple[Any, http.HTTPStatus]:
        """Get list of stands"""
        return [salt_role.to_json() for salt_role in get_all_salt_roles()], http.HTTPStatus.OK

    @api.expect(add_salt_role_model)
    @make_response(api, salt_role_model)
    def post(self) -> Tuple[Any, http.HTTPStatus]:
        body = flask.request.json

        stand = add_salt_role(name=body["name"])

        return stand.to_json(), http.HTTPStatus.CREATED


@api.route("/<string:salt_role_name>")
class Stand(BootstrapResource):
    @make_response(api, salt_role_model)
    def get(self, salt_role_name: str) -> Tuple[Any, http.HTTPStatus]:
        return get_salt_role(salt_role_name).to_json(), http.HTTPStatus.OK

    @make_response(api, empty_model)
    def delete(self, salt_role_name: str) -> Tuple[Any, http.HTTPStatus]:
        delete_salt_role(salt_role_name)
        return None, http.HTTPStatus.OK
