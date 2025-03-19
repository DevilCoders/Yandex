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
from .helpers.models import empty_model, ipv4_config_model, ipv6_config_model, svm_dynamic_config_model, \
    update_svm_model, batch_update_svm_model, add_svm_model, batch_delete_svm_model, svm_model


api = flask_restplus.Namespace("svms", description="Api to bootstrap svms")

api.add_model(empty_model.name, empty_model)
api.add_model(ipv4_config_model.name, ipv4_config_model)
api.add_model(ipv6_config_model.name, ipv6_config_model)
api.add_model(svm_dynamic_config_model.name, svm_dynamic_config_model)
api.add_model(update_svm_model.name, update_svm_model)
api.add_model(batch_update_svm_model.name, batch_update_svm_model)
api.add_model(add_svm_model.name, add_svm_model)
api.add_model(batch_delete_svm_model.name, batch_delete_svm_model)
api.add_model(svm_model.name, svm_model)


@api.route("")
class Svms(BootstrapResource):
    @make_response(api, svm_model, as_list=True)
    def get(self) -> Tuple[Any, http.HTTPStatus]:
        return [svm.to_json() for svm in get_all_instances(instance_type=EInstanceTypes.SVM.value)], http.HTTPStatus.OK

    @api.expect(add_svm_model)
    @make_response(api, svm_model)
    def post(self) -> Tuple[Any, http.HTTPStatus]:
        body = flask.request.json

        svm = add_instance(
            fqdn=body["fqdn"], dynamic_config=body["dynamic_config"], stand=body["stand"],
            instance_type=EInstanceTypes.SVM.value
        )

        return svm.to_json(), http.HTTPStatus.CREATED


@api.route(":batch-add")
class SvmsBatchAdd(BootstrapResource):
    @api.expect([add_svm_model])
    @make_response(api, svm_model, as_list=True)
    def post(self):
        svms = upsert_instances(flask.request.json, instance_type=EInstanceTypes.SVM.value, ensure_all_new=True)
        return [svm.to_json() for svm in svms], http.HTTPStatus.OK


@api.route(":batch-update")
class SvmsBatchUpdate(BootstrapResource):
    @api.expect([batch_update_svm_model])
    @make_response(api, svm_model, as_list=True)
    def post(self):
        svms = upsert_instances(flask.request.json, instance_type=EInstanceTypes.SVM.value, ensure_all_existing=True)
        return [svm.to_json() for svm in svms], http.HTTPStatus.OK


@api.route(":batch-upsert")
class SvmsBatchUpsert(BootstrapResource):
    @api.expect([add_svm_model])
    @make_response(api, svm_model, as_list=True)
    def post(self):
        svms = upsert_instances(flask.request.json, instance_type=EInstanceTypes.SVM.value)
        return [svm.to_json() for svm in svms], http.HTTPStatus.OK


@api.route(":batch-delete")
class HostsBatchDelete(BootstrapResource):
    @api.expect(batch_delete_svm_model)
    @make_response(api, svm_model, as_list=True)
    def post(self):
        delete_instances(flask.request.json["fqdns"], instance_type=EInstanceTypes.SVM.value)
        return None, http.HTTPStatus.OK


@api.route("/<string:svm_fqdn>")
class Svm(BootstrapResource):
    @make_response(api, svm_model)
    def get(self, svm_fqdn: str) -> Tuple[Any, http.HTTPStatus]:
        return get_instance(svm_fqdn, instance_type=EInstanceTypes.SVM.value).to_json(), http.HTTPStatus.OK

    @api.expect(update_svm_model)
    @make_response(api, svm_model)
    def patch(self, svm_fqdn: str) -> Tuple[Any, http.HTTPStatus]:
        svm = update_instance(fqdn=svm_fqdn, data=flask.request.json, instance_type=EInstanceTypes.SVM.value)
        return svm.to_json(), http.HTTPStatus.OK

    @make_response(api, empty_model)
    def delete(self, svm_fqdn: str) -> Tuple[Any, http.HTTPStatus]:
        delete_instance(svm_fqdn, instance_type=EInstanceTypes.SVM.value)
        return None, http.HTTPStatus.OK
