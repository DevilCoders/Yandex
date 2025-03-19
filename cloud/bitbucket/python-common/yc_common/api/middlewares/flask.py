"""Flask middleware for HTTP request handling."""

# TODO: We've got a lot of hacks and bad decisions here which originate from previous support of several API at the same
# time (Yandex.Cloud and OpenStack). Needs rethink and refactoring.

import flask
import functools
import logging
import werkzeug.serving

from yc_common.api.handling import render_api_error
from yc_common.context import context_session
from yc_common.exceptions import BadRequestError
from yc_common.logging import get_logger

from . import BaseMiddleware


log = get_logger(__name__)
werkzeug_log = logging.getLogger("werkzeug")


class FlaskServer:
    def __init__(self, error_renderer=render_api_error):
        # FIXME: Always return a valid JSON errors on error. For example now Flask generates 405 errors without our
        # renderer.

        self.__app = flask.Flask(__name__)
        self.__app.config["JSONIFY_PRETTYPRINT_REGULAR"] = False
        self.__middleware = FlaskMiddleware(self.__app, error_renderer=error_renderer)  # Rejects all unknown requests

        def set_cache_control(response):
            response.cache_control.no_cache = True
            return response

        # API responses mustn't be cached
        self.__app.after_request(set_cache_control)

    def register_blueprint(self, path, blueprint):
        self.__app.register_blueprint(blueprint, url_prefix=path)

    def run(self, port):
        # FIXME: Use another server?
        self.__app.run(host="::", port=port, threaded=True, request_handler=_WSGIRequestHandler)


class FlaskMiddleware(BaseMiddleware):
    def __init__(self, blueprint, error_renderer=render_api_error):
        self._blueprint = blueprint

        def unknown_uri_handler(path):
            log.warning("Reject an API request to an unknown resource: %r.", flask.request.path)
            return error_renderer(self, BadRequestError("UnknownApiResource", "Unknown API resource."))

        # Set up a catch-all handler that returns 400 instead of 404 for unknown URIs
        self._blueprint.add_url_rule("/", view_func=unknown_uri_handler, defaults={"path": ""})
        self._blueprint.add_url_rule("/<path:path>", view_func=unknown_uri_handler)

    def register_route(self, methods, path, ApiRequestClass, handler):
        @functools.wraps(handler)
        def route_handler(**kwargs):
            body = flask.request.data
            query_args = self.convert_query_args(flask.request.args.to_dict(flat=False))

            with context_session():
                api_request = ApiRequestClass(flask.request, query_args, kwargs)

                try:
                    api_request.parse(body)
                    result = handler(**api_request.kwargs)
                except Exception as e:
                    return api_request.render_error(e)
                else:
                    return api_request.render_result(result)

        if isinstance(path, str):
            path = (path,)

        for single_path in path:
            self._blueprint.add_url_rule(single_path, methods=methods, view_func=route_handler)

    @staticmethod
    def get_remote_address(request):
        return request.remote_addr

    @staticmethod
    def get_header_values(request, name):
        return request.headers.getlist(name)

    @staticmethod
    def convert_query_args(query_args):
        return query_args

    @staticmethod
    def render_json_response(result, status=200):
        response = flask.jsonify(**result)
        response.status_code = status
        return response

    @staticmethod
    def authenticate(auth_method, request):
        return auth_method.authenticate_flask_request(request)


class _WSGIRequestHandler(werkzeug.serving.WSGIRequestHandler):
    def log(self, type, message, *args):
        getattr(werkzeug_log, type)("%s " + message, self.address_string(), *args)
