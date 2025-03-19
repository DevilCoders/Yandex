"""Tiny host-configs blueprint (CLOUD-35467)"""

import http
from typing import Any, Tuple

import flask_restplus

from bootstrap.api.core.instances import get_host_configs_version

from .helpers.aux_resource import BootstrapResource
from .helpers.aux_response import make_response


api = flask_restplus.Namespace("host-configs-info", description="Various host_configs info (only verison now)")


version_model = api.model("HostConfigsVersionConfig", {
    "version": flask_restplus.fields.Integer(required=True),
})


@api.route("/version")
class Version(BootstrapResource):
    @make_response(api, version_model)
    def get(self) -> Tuple[Any, http.HTTPStatus]:
        return {"version": get_host_configs_version()}, http.HTTPStatus.OK
