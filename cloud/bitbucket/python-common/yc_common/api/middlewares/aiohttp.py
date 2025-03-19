"""aiohttp middleware for HTTP request handling."""

import inspect
import simplejson as json
from hashlib import sha256

import aiohttp.web

try:
    from asyncio.exceptions import CancelledError
except ImportError:
    from asyncio.futures import CancelledError

from yc_common import logging
from yc_common.context import context_session
from yc_requests.signing import CanonicalRequest

from . import BaseMiddleware


log = logging.get_logger(__name__)


class AiohttpMiddleware(BaseMiddleware):
    def __init__(self, router):
        self.__router = router

    def register_route(self, methods, path, ApiRequestClass, handler):
        async def route_handler(request):
            body = await request.read()

            query_args = {}
            for arg, value in request.GET.items():
                query_args.setdefault(arg, []).append(value)

            with context_session():
                api_request = ApiRequestClass(request, query_args, dict(request.match_info.items()))

                try:
                    api_request.parse(body)

                    result = handler(**api_request.kwargs)
                    if inspect.isawaitable(result):
                        result = await result
                except CancelledError:
                    log.info("API request has been cancelled (client closed the connection?).")
                    raise
                except Exception as e:
                    return api_request.render_error(e)
                else:
                    return api_request.render_result(result)

        path = path.replace("<", "{").replace(">", "}")
        for method in methods:
            self.__router.add_route(method, path, route_handler)

    @staticmethod
    def get_remote_address(request):
        return request.transport.get_extra_info("peername", (None, None))[0]

    @staticmethod
    def get_header_values(request, name):
        return request.headers.getall(name, [])

    @staticmethod
    def render_json_response(response, status=200):
        return aiohttp.web.json_response(text=json.dumps(response), status=status)

    @staticmethod
    def authenticate(auth_method, request):
        return auth_method.authenticate_aiohttp_request(request)
