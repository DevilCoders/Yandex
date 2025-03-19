#!/usr/bin/env python3
"""This module contains RpcError and Request classes."""

import re
import json
import logging
import requests

from enum import Enum
from requests.exceptions import ConnectionError, Timeout

from utils.response import Response
from utils.decorators import retry, log_debug
from core.error import (Unauthorized, BadRequest, PermissionDenied,
                        NetworkError, HTTPError, CoreError, TimedOut,
                        NotFound, FeatureNotImplemented)


logger = logging.getLogger(__name__)


class Request:
    """Base request wrapper for Yandex.Cloud REST API.

    Arguments:
      client: object
      headers: dict
      timeout: int
      token_type: str - 'OAuth' or 'Bearer' or 'TVM'

    """

    BASE_HEADERS = {
        'content-type': 'application/json'
    }

    def __init__(self,
                 client=None,
                 headers=None,
                 timeout=None,
                 token_type='OAuth',
                 **kwargs):

        self.headers = headers or self.BASE_HEADERS.copy()
        self.token_type = token_type or 'OAuth'
        self.client = self.set_and_return_client(client)
        self.timeout = int(timeout) if timeout else 10

    def set_authorization(self, token, service_ticket: str = None, user_ticket: str = None):
        if self.token_type == 'TVM':
            self.headers.update({'X-Ya-Service-Ticket': f'{service_ticket}'})
            self.headers.update({'X-Ya-User-Ticket': f'{user_ticket}'})
        else:
            self.headers.update({'Authorization': f'{self.token_type} {token}'})

    def set_and_return_client(self, client):
        self.client = client

        if self.client and self.client.token:
            self.set_authorization(self.client.token)

        return self.client

    @staticmethod
    def _convert_camel_to_snake(text):
        s1 = re.sub('<>(.)([A-Z][a-z]+)', r'\1_\2', text)
        return re.sub('<>([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

    @staticmethod
    def _object_hook(obj: dict):
        cleaned_object = {}
        for key, value in obj.items():
            key = Request._convert_camel_to_snake(key.replace('-', '_'))
            key = Request._convert_camel_to_snake(key.replace('<', ''))
            key = Request._convert_camel_to_snake(key.replace('>', ''))
            if len(key) and key[0].isdigit():
                key = '_' + key

            cleaned_object.update({key: value})
        return cleaned_object

    @log_debug
    def _parse(self, json_data: bytes):
        try:
            decoded_s = json_data.decode('utf-8')
            data = json.loads(decoded_s or '{}', object_hook=Request._object_hook)
        except UnicodeDecodeError:
            logger.debug('Logging raw invalid UTF-8 response:\n%r', json_data)
            raise CoreError('Server response could not be decoded using UTF-8')
        except (AttributeError, ValueError):
            logger.error(f'Bad server response: {json_data}')
            raise CoreError(f'Invalid server response: {json_data}')

        if isinstance(data, list):
            data = {
                'result': data,
                'error': None,
                'error_description': None
            }
        else:
            if data.get('result') is None:
                data = {
                    'result': data,
                    'error': RpcError(data.get('code')) if data.get('code') else data.get('error', None),
                    'error_description': data.get('message') or data.get('detail') or data.get('details', None)
                }

        return Response.de_json(data, self.client)

    @retry((NetworkError, TimedOut))
    def _request_wrapper(self, *args, **kwargs):
        if 'headers' not in kwargs:
            kwargs['headers'] = {}

        try:
            resp = requests.request(*args, **kwargs)
            logger.debug(resp.text)
        except requests.Timeout:
            raise TimedOut()
        except requests.RequestException as e:
            raise NetworkError(e)

        if 200 <= resp.status_code <= 299:
            return resp

        parse = self._parse(resp.content)
        message = parse.error or 'Unknown HTTPError'

        if resp.status_code == 401:
            raise Unauthorized(message)
        elif resp.status_code == 400:
            raise BadRequest(message)
        elif resp.status_code == 403:
            raise PermissionDenied(message)
        elif resp.status_code == 404:
            raise NotFound(message)
        elif resp.status_code in (409, 413):
            raise HTTPError(f'HTTP {resp.status_code} – {message}')

        elif resp.status_code == 510:
            raise FeatureNotImplemented(message)
        else:
            raise HTTPError(f'{resp.status_code} – {message}')

    def get(self, url, params=None, *args, **kwargs):
        result = self._request_wrapper('GET', url, params=params, headers=self.headers,
            timeout=self.timeout, *args, **kwargs)

        return self._parse(result.content).result

    def post(self, url, data=None, json=None, *args, **kwargs):
        result = self._request_wrapper('POST', url, headers=self.headers, data=data, json=json,
            timeout=self.timeout, *args, **kwargs)

        return self._parse(result.content).result

    def put(self, url, data=None, json=None, *args, **kwargs):
        result = self._request_wrapper('PUT', url, headers=self.headers, data=data, json=json,
            timeout=self.timeout, *args, **kwargs)

        return 'OK' if result.ok else self._parse(result.content).result

    def patch(self, url, data=None, json=None, *args, **kwargs):
        result = self._request_wrapper('PATCH', url, headers=self.headers, data=data, json=json,
            timeout=self.timeout, *args, **kwargs)

        return self._parse(result.content).result

    def delete(self, url, *args, **kwargs):
        result = self._request_wrapper('DELETE', url, headers=self.headers, timeout=self.timeout, *args, **kwargs)

        return self._parse(result.content).result


class RpcError(Enum):
    """This class convert RPC/HTTP digit error codes to messages."""

    # RPC codes
    CANCELLED = 1
    UNKNOWN = 2
    INVALID_ARGUMENT = 3
    DEADLINE_EXCEEDED = 4
    NOT_FOUND = 5
    ALREADY_EXISTS = 6
    PERMISSION_DENIED = 7
    RESOURCE_EXHAUSTED = 8
    FAILED_PRECONDITION = 9
    ABORTED = 10
    OUT_OF_RANGE = 11
    NOT_IMPLEMENTED = 12
    INTERNAL = 13
    UNAVAILABLE = 14
    DATA_LOSS = 15
    UNAUTHENTICATED = 16

    # HTTP compability
    HTTP_Canceled = 499
    HTTP_Internal = 500
    HTTP_InvalidArgument = 400
    HTTP_DeadlineExceeded = 504
    HTTP_NotFound = 400
    HTTP_AlreadyExist = 409
    HTTP_PermissionDenied = 403
    HTTP_ResourceExhaused = 429
    HTTP_NotImplemented = 501
    HTTP_Unavailable = 503
    HTTP_Unauthenticated = 401
