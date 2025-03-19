"""Stands blueprint"""

import http
from typing import Any, Tuple

import flask
import flask_restplus

from bootstrap.api.core.instances import get_stand_hosts_and_svms_with_version, get_stand_instances
from bootstrap.api.core.instance_groups import get_stand_instance_groups, get_instance_group, \
    add_instance_group, delete_instance_group, get_instance_group_release, set_instance_group_release
from bootstrap.api.core.stands import get_all_stands, get_stand, add_stand, delete_stand
from bootstrap.api.core.cluster_maps import get_stand_cluster_map, get_stand_cluster_map_version, \
    update_stand_cluster_map
from bootstrap.api.core.types import EInstanceTypes

from .helpers.aux_resource import BootstrapResource
from .helpers.aux_response import make_response
from .helpers.models import empty_model, host_model, svm_model, add_stand_model, stand_model, \
    stand_cluster_map_model, stand_cluster_map_version_model, update_stand_cluster_map_model, \
    add_instance_group_model, instance_group_model, put_instance_group_release_model, instance_group_release_model, \
    hosts_and_svms_model

api = flask_restplus.Namespace("stands", description="Stands api")
api.add_model(empty_model.name, empty_model)
api.add_model(host_model.name, host_model)
api.add_model(svm_model.name, svm_model)
api.add_model(add_stand_model.name, add_stand_model)
api.add_model(stand_model.name, stand_model)
api.add_model(stand_cluster_map_model.name, stand_cluster_map_model)
api.add_model(stand_cluster_map_version_model.name, stand_cluster_map_model)
api.add_model(update_stand_cluster_map_model.name, update_stand_cluster_map_model)
api.add_model(add_instance_group_model.name, add_instance_group_model)
api.add_model(instance_group_model.name, instance_group_model)
api.add_model(put_instance_group_release_model.name, put_instance_group_release_model)
api.add_model(instance_group_release_model.name, instance_group_release_model)
api.add_model(hosts_and_svms_model.name, hosts_and_svms_model)


@api.route("")
class Stands(BootstrapResource):
    @make_response(api, stand_model, as_list=True)
    def get(self) -> Tuple[Any, http.HTTPStatus]:
        """Get list of stands"""
        return [lock.to_json() for lock in get_all_stands()], http.HTTPStatus.OK

    @api.expect(add_stand_model)
    @make_response(api, stand_model)
    def post(self) -> Tuple[Any, http.HTTPStatus]:
        body = flask.request.json

        stand = add_stand(name=body["name"])

        return stand.to_json(), http.HTTPStatus.CREATED


@api.route("/<string:stand_name>")
class Stand(BootstrapResource):
    @make_response(api, stand_model)
    def get(self, stand_name: str) -> Tuple[Any, http.HTTPStatus]:
        return get_stand(stand_name).to_json(), http.HTTPStatus.OK

    @make_response(api, empty_model)
    def delete(self, stand_name: str) -> Tuple[Any, http.HTTPStatus]:
        delete_stand(stand_name)
        return None, http.HTTPStatus.OK


@api.route("/<string:stand_name>/hosts")
class StandHosts(BootstrapResource):
    @make_response(api, host_model, as_list=True)
    def get(self, stand_name: str) -> Tuple[Any, http.HTTPStatus]:
        hosts = get_stand_instances(stand_name, instance_type=EInstanceTypes.HOST.value)
        return [host.to_json() for host in hosts], http.HTTPStatus.OK


@api.route("/<string:stand_name>/svms")
class StandSvms(BootstrapResource):
    @make_response(api, svm_model, as_list=True)
    def get(self, stand_name: str) -> Tuple[Any, http.HTTPStatus]:
        svms = get_stand_instances(stand_name, instance_type=EInstanceTypes.SVM.value)
        return [svm.to_json() for svm in svms], http.HTTPStatus.OK


@api.route("/<string:stand_name>/hosts-and-svms-with-version")
class StandHostsAndSvms(BootstrapResource):
    """CLOUD-35467: get all stand hosts and svms with cluster_configs version"""
    @make_response(api, hosts_and_svms_model)
    def get(self, stand_name: str) -> Tuple[Any, http.HTTPStatus]:
        hosts, svms, cluster_configs_version = get_stand_hosts_and_svms_with_version(stand_name)
        return {
            "hosts": [host.to_json() for host in hosts],
            "svms": [svm.to_json() for svm in svms],
            "cluster_configs_version": cluster_configs_version,
        }, http.HTTPStatus.OK


@api.route("/<string:stand_name>/cluster-map")
class StandClusterMap(BootstrapResource):
    @make_response(api, stand_cluster_map_model)
    def get(self, stand_name: str) -> Tuple[Any, http.HTTPStatus]:
        cluster_map = get_stand_cluster_map(stand_name)
        return cluster_map.to_json(), http.HTTPStatus.OK

    @api.expect(update_stand_cluster_map_model)
    @make_response(api, stand_cluster_map_model)
    def patch(self, stand_name: str) -> Tuple[Any, http.HTTPStatus]:
        body = flask.request.json

        cluster_map = update_stand_cluster_map(stand_name, body)

        return cluster_map.to_json(), http.HTTPStatus.OK


@api.route("/<string:stand_name>/cluster-map/version")
class StandClusterMapVersion(BootstrapResource):
    @make_response(api, stand_cluster_map_version_model)
    def get(self, stand_name: str) -> Tuple[Any, http.HTTPStatus]:
        cluster_map_version = get_stand_cluster_map_version(stand_name)
        return cluster_map_version.to_json(), http.HTTPStatus.OK


@api.route("/<string:stand_name>/instance-groups")
class StandInstanceGroups(BootstrapResource):
    @make_response(api, instance_group_model, as_list=True)
    def get(self, stand_name: str) -> Tuple[Any, http.HTTPStatus]:
        stands = get_stand_instance_groups(stand_name)
        return [stand.to_json() for stand in stands], http.HTTPStatus.OK

    @api.expect(add_instance_group_model)
    @make_response(api, instance_group_model)
    def post(self, stand_name: str) -> Tuple[Any, http.HTTPStatus]:
        body = flask.request.json

        instance_group = add_instance_group(stand_name, body["name"])

        return instance_group.to_json(), http.HTTPStatus.CREATED


@api.route("/<string:stand_name>/instance-groups/<string:instance_group_name>")
class StandInstanceGroup(BootstrapResource):
    @make_response(api, instance_group_model)
    def get(self, stand_name: str, instance_group_name: str) -> Tuple[Any, http.HTTPStatus]:
        instance_group = get_instance_group(stand_name, instance_group_name)
        return instance_group.to_json(), http.HTTPStatus.OK

    @make_response(api, empty_model)
    def delete(self, stand_name: str, instance_group_name: str) -> Tuple[Any, http.HTTPStatus]:
        delete_instance_group(stand_name, instance_group_name)
        return None, http.HTTPStatus.OK


@api.route("/<string:stand_name>/instance-groups/<string:instance_group_name>/release")
class StandInstanceGroupRelease(BootstrapResource):
    @make_response(api, instance_group_release_model)
    def get(self, stand_name: str, instance_group_name: str) -> Tuple[Any, http.HTTPStatus]:
        instance_group_release = get_instance_group_release(stand_name, instance_group_name)
        return instance_group_release.to_json(), http.HTTPStatus.OK

    @api.expect(put_instance_group_release_model)
    @make_response(api, instance_group_release_model)
    def put(self, stand_name: str, instance_group_name: str) -> Tuple[Any, http.HTTPStatus]:
        body = flask.request.json

        instance_group_release = set_instance_group_release(
            stand_name, instance_group_name, body["url"], body["image_id"]
        )

        return instance_group_release.to_json(), http.HTTPStatus.OK
