"""Instances blueprint (CLOUD-36446)"""

import http
from typing import Any, Tuple

import flask
import flask_restplus

from bootstrap.api.core.instances import get_instance_salt_roles, get_instance_salt_role, add_instance_salt_role, \
    delete_instance_salt_role, upsert_instance_salt_roles, get_instance_salt_role_packages, \
    upsert_instance_salt_role_packages

from .helpers.aux_resource import BootstrapResource
from .helpers.aux_response import make_response
from .helpers.models import empty_model, salt_role_model, add_salt_role_model, instance_salt_role_package_model, \
    update_instance_salt_role_package_model

api = flask_restplus.Namespace("instances", description="Instances api")

api.add_model(empty_model.name, empty_model)
api.add_model(instance_salt_role_package_model.name, instance_salt_role_package_model)
api.add_model(update_instance_salt_role_package_model.name, update_instance_salt_role_package_model)


@api.route("/<string:fqdn>/salt-roles")
class InstanceSaltRoles(BootstrapResource):
    @make_response(api, salt_role_model, as_list=True)
    def get(self, fqdn: str) -> Tuple[Any, http.HTTPStatus]:
        return [salt_role.to_json() for salt_role in get_instance_salt_roles(fqdn)], http.HTTPStatus.OK

    @api.expect(add_salt_role_model)
    @make_response(api, salt_role_model, as_list=True)
    def post(self, fqdn: str) -> Tuple[Any, http.HTTPStatus]:
        return add_instance_salt_role(fqdn, flask.request.json["name"]), http.HTTPStatus.CREATED

    @api.expect([add_salt_role_model])
    @make_response(api, salt_role_model, as_list=True)
    def put(self, fqdn: str):
        salt_roles = upsert_instance_salt_roles(fqdn=fqdn, salt_roles_data=flask.request.json)
        return [salt_role.to_json() for salt_role in salt_roles], http.HTTPStatus.OK


@api.route("/<string:fqdn>/salt-roles/<string:salt_role_name>")
class InstanceSaltRole(BootstrapResource):
    @make_response(api, salt_role_model)
    def get(self, fqdn: str, salt_role_name: str) -> Tuple[Any, http.HTTPStatus]:
        return get_instance_salt_role(fqdn, salt_role_name).to_json(), http.HTTPStatus.OK

        return [salt_role.to_json() for salt_role in get_instance_salt_roles(fqdn)], http.HTTPStatus.OK

    @make_response(api, empty_model)
    def delete(self, fqdn: str, salt_role_name: str) -> Tuple[Any, http.HTTPStatus]:
        return delete_instance_salt_role(fqdn, salt_role_name), http.HTTPStatus.OK


@api.route("/<string:fqdn>/salt-role-releases")
class InstanceSaltRolePackages(BootstrapResource):
    @make_response(api, instance_salt_role_package_model, as_list=True)
    def get(self, fqdn: str):
        packages = get_instance_salt_role_packages(fqdn=fqdn)
        return [package.to_json() for package in packages], http.HTTPStatus.OK

    @api.expect([update_instance_salt_role_package_model])
    @make_response(api, instance_salt_role_package_model, as_list=True)
    def put(self, fqdn: str):
        packages = upsert_instance_salt_role_packages(fqdn=fqdn, packages_info=flask.request.json)
        return [package.to_json() for package in packages], http.HTTPStatus.OK
