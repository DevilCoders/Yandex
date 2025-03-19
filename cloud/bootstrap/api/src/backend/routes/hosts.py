"""Hosts blueprint (almost similar to svms api)"""

import http
from typing import Any, Tuple

import flask
import flask_restplus

from bootstrap.api.core.instances import get_all_instances, get_instance, add_instance, update_instance, \
    delete_instance, upsert_instances, delete_instances
from bootstrap.api.core.types import EInstanceTypes

from .helpers.aux_response import make_response
from .helpers.aux_resource import BootstrapResource
from .helpers.models import empty_model, ipv4_config_model, ipv6_config_model, host_dynamic_config_model, \
    update_host_model, batch_update_host_model, batch_delete_host_model, add_host_model, host_model


api = flask_restplus.Namespace("hosts", description="Api to bootstrap hosts")

api.add_model(empty_model.name, empty_model)
api.add_model(ipv4_config_model.name, ipv4_config_model)
api.add_model(ipv6_config_model.name, ipv6_config_model)
api.add_model(host_dynamic_config_model.name, host_dynamic_config_model)
api.add_model(update_host_model.name, update_host_model)
api.add_model(batch_update_host_model.name, batch_update_host_model)
api.add_model(add_host_model.name, add_host_model)
api.add_model(batch_delete_host_model.name, batch_delete_host_model)
api.add_model(host_model.name, host_model)


@api.route("")
class Hosts(BootstrapResource):
    @make_response(api, host_model, as_list=True)
    def get(self) -> Tuple[Any, http.HTTPStatus]:
        return [host.to_json() for host in get_all_instances(instance_type=EInstanceTypes.HOST.value)], http.HTTPStatus.OK

    @api.expect(add_host_model)
    @make_response(api, host_model)
    def post(self) -> Tuple[Any, http.HTTPStatus]:
        body = flask.request.json

        host = add_instance(
            fqdn=body["fqdn"], dynamic_config=body["dynamic_config"], stand=body["stand"],
            instance_type=EInstanceTypes.HOST.value
        )

        return host.to_json(), http.HTTPStatus.CREATED


@api.route(":batch-add")
class HostsBatchAdd(BootstrapResource):
    @api.expect([add_host_model])
    @make_response(api, host_model, as_list=True)
    def post(self):
        hosts = upsert_instances(flask.request.json, instance_type=EInstanceTypes.HOST.value, ensure_all_new=True)
        return [host.to_json() for host in hosts], http.HTTPStatus.OK


@api.route(":batch-update")
class HostsBatchUpdate(BootstrapResource):
    @api.expect([batch_update_host_model])
    @make_response(api, host_model, as_list=True)
    def post(self):
        hosts = upsert_instances(flask.request.json, instance_type=EInstanceTypes.HOST.value, ensure_all_existing=True)
        return [host.to_json() for host in hosts], http.HTTPStatus.OK


@api.route(":batch-upsert")
class HostsBatchUpsert(BootstrapResource):
    @api.expect([add_host_model])
    @make_response(api, host_model, as_list=True)
    def post(self):
        hosts = upsert_instances(flask.request.json, instance_type=EInstanceTypes.HOST.value)
        return [host.to_json() for host in hosts], http.HTTPStatus.OK


@api.route(":batch-delete")
class HostsBatchDelete(BootstrapResource):
    @api.expect(batch_delete_host_model)
    @make_response(api, host_model, as_list=True)
    def post(self):
        delete_instances(flask.request.json["fqdns"], instance_type=EInstanceTypes.HOST.value)
        return None, http.HTTPStatus.OK


@api.route("/<string:host_fqdn>")
class Host(BootstrapResource):
    @make_response(api, host_model)
    def get(self, host_fqdn: str) -> Tuple[Any, http.HTTPStatus]:
        return get_instance(host_fqdn, instance_type=EInstanceTypes.HOST.value).to_json(), http.HTTPStatus.OK

    @api.expect(update_host_model)
    @make_response(api, host_model)
    def patch(self, host_fqdn: str) -> Tuple[Any, http.HTTPStatus]:
        host = update_instance(fqdn=host_fqdn, data=flask.request.json, instance_type=EInstanceTypes.HOST.value)
        return host.to_json(), http.HTTPStatus.OK

    @make_response(api, empty_model)
    def delete(self, host_fqdn: str) -> Tuple[Any, http.HTTPStatus]:
        delete_instance(host_fqdn, instance_type=EInstanceTypes.HOST.value)
        return None, http.HTTPStatus.OK
