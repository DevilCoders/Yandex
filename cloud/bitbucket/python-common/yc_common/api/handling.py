"""API request handling logic."""

# TODO: We've got a lot of hacks and bad decisions here which originate from previous support of several API at the same
# time (Yandex.Cloud and OpenStack). Needs rethink and refactoring.

import cgi
import sys
import time

import functools
import simplejson as json

from yc_auth.exceptions import YcAuthClientError
from yc_common import constants
from yc_common import logging
from yc_common import metrics
from yc_common.clients.kikimr import KikimrError, RetryableError
from yc_common.context import update_context
from yc_common.exceptions import CommonErrorCodes, GrpcStatus, Error, LogicalError, ApiError, BadRequestError, \
    InternalServerError, RequestValidationError, ServiceUnavailableError
from yc_common.misc import ellipsis_string, drop_none, generate_id
from yc_common.models import Model, ModelValidationError, MetadataOptions
from yc_common.validation import SchemaValidationError, validate_uuid

from .authentication import AuthenticationContext, YcAuthMethodTypes
from .request_context import RequestContext

log = logging.get_logger(__name__)


_DATA_METHODS = {"PUT", "POST", "PATCH", "DELETE"}


def process_json_body(request, type_options, body):
    encoding = type_options.get("charset", "utf-8")

    try:
        request_json = json.loads(body, encoding=encoding)
        if type(request_json) is not dict:
            raise ValueError

        return request_json
    except ValueError:
        raise RequestValidationError("The specified request data is not a valid JSON object.")


_PROCESS_REQUEST = {
    "application/json": process_json_body
}


api_request_counter = metrics.Metric(
    metrics.MetricTypes.COUNTER,
    "api_request_count",
    ["status_code", "method", "path"],
    "API request counter.")

api_request_latency = metrics.Metric(
    metrics.MetricTypes.HISTOGRAM,
    "api_request_latency",
    ["method", "path"],
    "API request latency histogram.",
    buckets=metrics.time_buckets_ms)

gauthling_latency = metrics.Metric(
    metrics.MetricTypes.HISTOGRAM,
    "gauthling_latency",
    label_names=["type"],
    doc="Gauthling latency histogram.",
    buckets=metrics.time_buckets_ms)


class EmptyRequestModel(Model):
    pass


class _ApiRequestBase:
    def __init__(self, request, middleware, error_renderer, request_raw_path):
        self.__middleware = middleware
        self.__error_renderer = error_renderer
        self.__time = time.time()
        self.__method = request.method
        self.__raw_path = request_raw_path

    def render_result(self, result):
        log.info("[%s] API request has completed.", self.__render_processing_time())

        try:
            if isinstance(result, Model):
                result = result.to_user()

            status_code = 200
            if type(result) is dict:
                result = self.__middleware.render_json_response(result)
                try:
                    # class flask.Response
                    status_code = result.status_code
                except AttributeError:
                    # class aiohttp.ClientResponse
                    status_code = result.status
            elif type(result) is tuple and len(result) == 2 and type(result[0]) is dict:
                response, status_code = result
                result = self.__middleware.render_json_response(response, status_code)

            api_request_counter.labels(status_code, self.__method, self.__raw_path).inc()
            api_request_latency.labels(self.__method, self.__raw_path).observe((time.time() - self.__time) * 1000)
            return result

        except Exception as e:
            return self.render_error(e)

    def render_error(self, error):
        if isinstance(error, ApiError):
            log.warning("[%s] API request returned an error [%s]: %s",
                        self.__render_processing_time(), error.code, error)

            status_code = error.http_code
            err = self.__error_renderer(self.__middleware, error)
        elif isinstance(error, YcAuthClientError):
            if error.http_code == GrpcStatus.UNAUTHENTICATED.http_code:
                grpc_code = GrpcStatus.UNAUTHENTICATED
            elif error.http_code == GrpcStatus.PERMISSION_DENIED.http_code:
                grpc_code = GrpcStatus.PERMISSION_DENIED
            else:
                grpc_code = GrpcStatus.INTERNAL

            (log.error if grpc_code == GrpcStatus.INTERNAL else log.info)(
                "[%s] API request resulted in an auth failure [%s]: %s",
                self.__render_processing_time(), error.code, error)

            # FIXME: Should we take into account internal from authentication service?
            api_error = ApiError(http_code=grpc_code, code=error.code, message=error.message,
                                 internal=error.internal)
            status_code = api_error.http_code
            err = self.__error_renderer(self.__middleware, api_error)
        elif isinstance(error, KikimrError) and isinstance(error.parent_error, RetryableError):
            status_code = 503
            err = self.__error_renderer(self.__middleware, ServiceUnavailableError())
        else:
            (log.error if isinstance(error, Error) else log.exception)(
                "[%s] API request has crashed: %s", self.__render_processing_time(), error)
            status_code = 500
            err = self.__error_renderer(self.__middleware, InternalServerError())

        api_request_counter.labels(status_code, self.__method, self.__raw_path).inc()
        return err

    def __render_processing_time(self):
        return "rt={:.2f}".format(time.time() - self.__time)


# FIXME: no-cache response headers
# FIXME: error handling, body size limits
# FIXME: return 400 for unknown URLs instead of 404
def generic_api_handler(middleware, methods, path,
                        doc_writer=None,
                        model=None,
                        response_model=None,
                        process_request_rules=_PROCESS_REQUEST,
                        params_model=None,
                        query_variables=False,
                        error_renderer=None, context_params=None, request_context=True,
                        auth_method: YcAuthMethodTypes = None,
                        idempotency_support=False, extract_cookies=False, mask_fields=None):

    if isinstance(methods, str):
        methods = (methods,)

    if model is not None and not set(methods).intersection(_DATA_METHODS):
        raise LogicalError()

    if error_renderer is None:
        error_renderer = render_api_error

    if process_request_rules is None:
        raise LogicalError()

    if not request_context and (auth_method is not None or idempotency_support):
        raise LogicalError("Got a conflicting API request handler parameters.")

    # FIXME: Don't create a separate class for every handler
    class ApiRequest(_ApiRequestBase):
        def __init__(self, request, query_args, kwargs):
            super().__init__(request, middleware, error_renderer, request_raw_path=path)
            self.query_args = query_args
            self.kwargs = kwargs
            self.__request = request

        def parse(self, body):
            request = self.__request

            request_url = request.path
            if request.query_string:
                if isinstance(request.query_string, bytes):
                    try:
                        query_str = request.query_string.decode("utf-8")
                    except ValueError:
                        raise RequestValidationError("Invalid query string.")
                else:
                    query_str = request.query_string
                request_url += "?" + query_str

            auth_time = None
            request_data = None

            try:
                if "X-Request-UID" in request.headers:
                    request_uid = validate_uuid(request.headers["X-Request-UID"], "Invalid X-Request-UID header value.")
                else:
                    request_uid = generate_id()

                if "X-Request-ID" in request.headers:
                    request_id = validate_uuid(request.headers["X-Request-ID"], "Invalid X-Request-ID header value.")
                else:
                    request_id = request_uid

                update_context(request_id=request_id, request_uid=request_uid)

                if request.method in _DATA_METHODS:
                    content_type, type_options = cgi.parse_header(request.headers.get("Content-Type", ""))

                    if content_type in process_request_rules:
                        request_data = process_request_rules[content_type](request, type_options, body)

                        request_data = middleware.convert_json_request(request_data)
                    elif request.method == "DELETE":
                        pass
                    else:
                        raise RequestValidationError("Request data must be an application/json object.")

                handler_arguments = []

                try:
                    if params_model is not None:
                        query_args = {k: v[0] for k, v in self.query_args.items()}
                        if query_variables:
                            move_query_variables(self.kwargs, query_args, params_model)

                        handler_arguments.append(("request", params_model.from_api(query_args)))
                    else:
                        EmptyRequestModel.from_api(self.query_args)

                    if request_data is not None:
                        if model is not None:
                            if query_variables:
                                move_query_variables(self.kwargs, request_data, model)

                            handler_arguments.append(("request", model.from_api(request_data)))
                        else:
                            EmptyRequestModel.from_api(request_data)
                except ModelValidationError as e:
                    raise BadRequestError(CommonErrorCodes.RequestValidationError, e.public_message)
                except SchemaValidationError as e:
                    raise RequestValidationError(str(e))

                if extract_cookies:
                    handler_arguments.append(("cookies", request.cookies))

                ctx = RequestContext.new(request_id=request_id)
                ctx.peer_address = middleware.get_remote_address(request)
                ctx.user_agent = _get_last_header(middleware, request, "User-Agent") or None
                ctx.remote_address = _get_last_header(middleware, request, "X-Forwarded-For") or None

                if auth_method is not None:
                    auth_start_time = time.monotonic()

                    try:
                        auth_ctx = middleware.authenticate(auth_method, request)
                    except BaseException:
                        auth_response_type = "error"
                        raise
                    else:
                        auth_response_type = "response"
                    finally:
                        auth_time = time.monotonic() - auth_start_time
                        gauthling_latency.labels(auth_response_type).observe(auth_time * 1000)

                    ctx.auth = AuthenticationContext.new(
                        user=AuthenticationContext.User.new(
                            id=auth_ctx.user.id, name=auth_ctx.user.name, type=auth_ctx.user.type,
                        ),
                        token_bytes=auth_ctx.token,
                    )

                    if ctx.auth.user.is_service_account():
                        ctx.auth.user.folder_id = auth_ctx.user.folder_id

                    update_context(user_id=ctx.auth.user.id)

                if constants.IDEMPOTENCE_HEADER in request.headers:
                    idempotence_id = validate_uuid(request.headers[constants.IDEMPOTENCE_HEADER],
                                                   "Invalid {} header value.", constants.IDEMPOTENCE_HEADER)

                    request_idempotence_id = idempotence_id
                    if auth_method is not None:
                        request_idempotence_id = ctx.auth.user.id + ":" + idempotence_id

                    if not idempotency_support:
                        raise RequestValidationError("{} header is not supported for this method yet.", constants.IDEMPOTENCE_HEADER)

                    ctx.idempotence_id = idempotence_id
                    update_context(request_idempotence_id=request_idempotence_id)

                if request_context:
                    handler_arguments.append(("request_context", ctx))

                for arg, value in handler_arguments:
                    if arg in self.kwargs:
                        raise Error("Logical error: Request handler got conflicting arguments: {}.", arg)
                    self.kwargs[arg] = value

                if "request" in self.kwargs and "request_context" in self.kwargs:
                    try:
                        self.kwargs["request_context"].is_internal = self.kwargs["request"].is_internal
                    except AttributeError:
                        pass

                if context_params:
                    context_to_update = {param: self.kwargs[param] for param in context_params if param in self.kwargs}

                    if "user_id" in context_params and "user_id" not in context_to_update:
                        user_id = _user_id_extractor(self.kwargs.get("request_context"))
                        if user_id is not None:
                            context_to_update["user_id"] = user_id

                    if "org_id" in context_params and "org_id" not in context_to_update:
                        org_id = _organization_extractor(**self.kwargs)
                        if org_id is not None:
                            context_to_update["org_id"] = org_id

                    update_context(**context_to_update)
            finally:
                if request_data is None:
                    request_log_message = ""
                else:
                    masked_data = logging.mask_sensitive_fields(request_data, extra_fields=mask_fields)
                    request_log_message = ellipsis_string(str(masked_data), 10000)

                with update_context(**drop_none({
                    "remote_address": middleware.get_remote_address(request),
                    "x_real_ip": _get_last_header(middleware, request, "X-Real-IP") or None,
                    "x_forwarded_for": _get_last_header(middleware, request, "X-Forwarded-For") or None,
                })):
                    log.info(
                        "API request: %s %s %s",
                        request.method, ellipsis_string(request_url, 10000), request_log_message
                    )

                if auth_time is not None and auth_time > 1:
                    log.warning("API request authentication process took %.1f seconds.", auth_time)

    def decorator(func):
        if doc_writer is not None:
            for method in methods:
                doc_writer.add_api_request(
                    method,
                    path,
                    func,
                    params_model=params_model,
                    request_model=model,
                    response_model=response_model,
                    module_name=sys.modules[func.__module__].__name__,
                    module_description=sys.modules[func.__module__].__doc__
                )

        middleware.register_route(methods, path, ApiRequest, func)
        return func

    return decorator


def move_query_variables(request_args, request_json, request_class: Model):
    """Move query_variables from request_args to request_object"""

    for name, field in request_class.fields.items():
        query_variable = field.metadata.get(MetadataOptions.QUERY_VARIABLE)
        if query_variable is not None:
            if query_variable in request_json:
                raise RequestValidationError("Invalid parameter: {!r}.", query_variable)
            request_json[name] = request_args.pop(query_variable)


def add_param_validation(api_handler_decorator, param, validator):
    """Modifies API handler decorator by adding a validation stage for the specified API handler parameter."""

    def decorator(handler):
        @functools.wraps(handler)
        def validation_wrapper(**kwargs):
            if param in kwargs:
                kwargs[param] = validator(kwargs[param])

            return handler(**kwargs)

        return api_handler_decorator(validation_wrapper)

    return decorator


# FIXME: Don't render internal field in public API
def render_api_error(middleware, api_error: ApiError, public=False):
    response, status = format_api_error(api_error, public=public)
    return middleware.render_json_response(response, status)


def format_api_error(api_error: ApiError, public=False):
    if public and api_error.internal:
        api_error = InternalServerError()

    return drop_none({
        "status": api_error.grpc_code,
        "code": api_error.code,
        "message": api_error.message,
        "internal": api_error.internal,
        "details": api_error.details,
    }), api_error.http_code


def _user_id_extractor(request_context: RequestContext=None):
    if request_context and request_context.get("auth"):
        return request_context.auth.user.id


def _organization_extractor(*args, **kwargs):
    if "request" in kwargs and kwargs["request"].get("organizationId"):
        return kwargs["request"].organizationId
    elif kwargs.get("organization_id"):
        return kwargs.get("organization_id")


def _get_last_header(middleware, request, name):
    values = middleware.get_header_values(request, name)
    return values[-1] if values else None
