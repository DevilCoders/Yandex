import copy
import http
import traceback
from typing import Dict

import flask_restplus
from flask_log_request_id import current_request_id
import werkzeug.exceptions as werkzeug_exceptions

from bootstrap.common.rdbms.exceptions import RecordNotFoundError, RecordAlreadyInDbError

from bootstrap.api.core.auth import NotAuthorizedError
from bootstrap.api.core.constants import VERSION, API_VERSION_PREFIX, API_DOC_URL
from bootstrap.api.core.exceptions import DbInMigrationError, UnsupportedDbVersionError
from bootstrap.api.logging import message_to_access_log

from .helpers.aux_response import _wrap_response, _make_flask_response

from .admin import api as admin_namespace
from .locks import api as locks_namespace
from .hosts import api as hosts_namespace
from .svms import api as svms_namespace
from .stands import api as stands_namespace
from .host_configs_info import api as host_configs_info_namespace
from .instances import api as instances_namespace
from .salt_roles import api as salt_roles_namespace


class BootstrapApi(flask_restplus.Api):
    """Custom api for handling errors"""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs, catch_all_404s=True)

    def handle_error(self, e):
        """Handle exceptions"""
        if isinstance(e, werkzeug_exceptions.HTTPException):
            response_code = e.code
            # FIXME: unobvious way to detect validation error
            if hasattr(e, "data") and e.data.get("message", None) == "Input payload validation failed":
                errors = ["{}: {}".format(k, v) for k, v in e.data["errors"].items()]
                message = "{}:\n  {}".format(e.data["message"], "\n  ".join(errors))
            else:
                message = str(e)
        elif isinstance(e, RecordNotFoundError):
            response_code = http.HTTPStatus.NOT_FOUND
            message = str(e)
        elif isinstance(e, RecordAlreadyInDbError):
            response_code = http.HTTPStatus.CONFLICT
            message = str(e)
        elif isinstance(e, NotAuthorizedError):
            response_code = http.HTTPStatus.UNAUTHORIZED
            message = str(e)
        elif isinstance(e, (DbInMigrationError, UnsupportedDbVersionError)):
            response_code = http.HTTPStatus.SERVICE_UNAVAILABLE
            message = str(e)
        else:
            response_code = http.HTTPStatus.INTERNAL_SERVER_ERROR
            message = str(e)

        response_json = _wrap_response("{}: {}. rid={}".format(e.__class__.__name__, message, current_request_id()),
                                       response_code)

        message_to_access_log(response_json, response_code, traceback.format_exc())

        return _make_flask_response(response_json, code=response_code)


def _swagger_view_get(self):
    """Kinda fix schema (remove ['object', 'null'] types, which are valid in jsonschema but not valid in Swagger2.0"""
    def _recurse_fix_type(d: Dict):
        """Fix dict"""
        if not isinstance(d, dict):
            return d

        for k, v in d.items():
            d[k] = _recurse_fix_type(v)

        # replace <'type': ['something', 'null']> by <'type': 'something'>
        if ("type" in d) and isinstance(d["type"], list) and (len(d["type"]) == 2) and (d["type"][-1] == "null"):
            d["type"] = d["type"][0]

        return d

    schema = _recurse_fix_type(copy.deepcopy(self.api.__schema__))

    if "error" in schema:
        code = flask_restplus._http.HTTPStatus.INTERNAL_SERVER_ERROR
    else:
        code = flask_restplus._http.HTTPStatus.OK

    return schema, code


flask_restplus.api.SwaggerView.get = _swagger_view_get  # FIXME: dirty hack


api = BootstrapApi(version=VERSION, title="Bootstrap API", doc=API_DOC_URL, validate=True)

# add blueprints
api.add_namespace(admin_namespace, path="/admin")
api.add_namespace(locks_namespace, path="{}/locks".format(API_VERSION_PREFIX))
api.add_namespace(hosts_namespace, path="{}/hosts".format(API_VERSION_PREFIX))
api.add_namespace(svms_namespace, path="{}/svms".format(API_VERSION_PREFIX))
api.add_namespace(stands_namespace, path="{}/stands".format(API_VERSION_PREFIX))
api.add_namespace(host_configs_info_namespace, path="{}/host-configs-info".format(API_VERSION_PREFIX))
api.add_namespace(instances_namespace, path="{}/instances".format(API_VERSION_PREFIX))
api.add_namespace(salt_roles_namespace, path="{}/salt-roles".format(API_VERSION_PREFIX))
