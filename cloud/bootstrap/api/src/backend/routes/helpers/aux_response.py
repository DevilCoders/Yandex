"""Auxiliarily function for making response"""

import http
from typing import Any, Callable, Dict, Tuple, Union

import flask
import flask_restplus
import ujson

from bootstrap.api.logging import message_to_access_log


class EResponseStatus:
    SUCCESS = "success"  # request succeeded
    FAIL = "fail"  # request can not be executed due to bad request params or data
    ERROR = "error"  # request can not be executed due to backend malfunction


def _wrap_response(data: Union[Dict, str], response_code=http.HTTPStatus.OK) -> Dict:
    """Wrap data to be returned into Response object with rest paradigm"""

    if 200 <= response_code <= 399:
        status = EResponseStatus.SUCCESS
        error_message = None
    elif 400 <= response_code <= 499:
        status = EResponseStatus.FAIL
        error_message = data
        data = None
    else:
        status = EResponseStatus.ERROR
        error_message = data
        data = None

    response_json = {
        "data": data,
        "code": int(response_code),
        "status": status,
        "error_message": error_message,
    }

    return response_json


def _make_flask_response(response_json: Dict, code=http.HTTPStatus.OK) -> flask.Response:
    if flask.request.args.get("pretty", False):  # FIXME: looks like bad way
        response_text = ujson.dumps(response_json, sort_keys=True, indent=4)
    else:
        response_text = ujson.dumps(response_json)
    response_text += "\n"

    return flask.Response(response_text, status=code, content_type="application/json")


def make_response(api: flask_restplus.Namespace, model: flask_restplus.Model, as_list: bool = False,
                  code: http.HTTPStatus = http.HTTPStatus.OK) -> flask.Response:
    """Decorator to construct response. At the moment doing the following:
         - marshal response with flask_restplus Response model
         - wrap response to make it more RESTful
    """

    def real_make_response(function: Callable[..., Tuple[Any, http.HTTPStatus]]):
        # wrap model with REST stuff
        wrapped_model = api.model("RestWrapped{}".format(model.name), {
            "data": flask_restplus.fields.Nested(model, allow_null=True),
            "code": flask_restplus.fields.Integer(required=True),
            "status": flask_restplus.fields.String(required=True),
            "error_message": flask_restplus.fields.String(),
        })

        # marshall with updated model, wrap result
        @api.marshal_with(wrapped_model, as_list=as_list, code=int(code))
        def wrapper(*args, **kwargs):
            response, response_code = function(*args, **kwargs)

            message_to_access_log(response, response_code)

            return _wrap_response(response, response_code), response_code

        # create flask response
        def flask_response_wrapper(*args, **kwargs):
            response_json, response_code, _ = wrapper(*args, **kwargs)
            return _make_flask_response(response_json, code=response_code)
        flask_response_wrapper.__apidoc__ = wrapper.__apidoc__

        return flask_response_wrapper

    return real_make_response
