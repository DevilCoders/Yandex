"""Admin blueprint"""

import http
from typing import Any, Tuple

import flask_restplus

from bootstrap.api.core.constants import VERSION

from .helpers.aux_resource import BootstrapResource
from .helpers.aux_response import make_response


api = flask_restplus.Namespace("admin", description="Various admin functions")


version_model = api.model("VersionConfig", {
    "version": flask_restplus.fields.String(required=True),
})


health_model = api.model("HealthConfig", {
})


@api.route("/version")
class Version(BootstrapResource):
    @make_response(api, version_model)
    def get(self) -> Tuple[Any, http.HTTPStatus]:
        return {"version": VERSION}, http.HTTPStatus.OK


@api.route("/health")
class Health(flask_restplus.Resource):  # authentification for health check is not needed
    @make_response(api, health_model)
    def get(self) -> Tuple[Any, http.HTTPStatus]:
        # TODO: add some real checks
        return None, http.HTTPStatus.OK
