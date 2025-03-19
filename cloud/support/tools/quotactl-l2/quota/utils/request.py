#!/usr/bin/env python3
"""This module contains RpcError, RestRequest and gRPCRequest classes."""

import re
import json
import grpc
import logging
import requests

from enum import Enum
from quota.utils.response import Response
from quota.utils.helpers import retry
from quota.error import (Unauthorized, BadRequest, PermissionDenied, NetworkError, HTTPError, SSLError,
                             QuotaServiceError, TimedOut, ResourceNotFound, FeatureNotImplemented)

from yandex.cloud.priv.quota import quota_pb2
from yandex.cloud.priv.compute.v1 import quota_service_pb2_grpc as compute
from yandex.cloud.priv.serverless.functions.v1 import quota_service_pb2_grpc as functions
from yandex.cloud.priv.serverless.triggers.v1 import quota_service_pb2_grpc as triggers
from yandex.cloud.priv.microcosm.instancegroup.v1 import quota_service_pb2_grpc as instance_group
from yandex.cloud.priv.k8s.v1 import quota_service_pb2_grpc as kubernetes
from yandex.cloud.priv.containerregistry.v1 import quota_service_pb2_grpc as container_registry
from yandex.cloud.priv.vpc.v1 import quota_service_pb2_grpc as vpc
from yandex.cloud.priv.monitoring.v2 import quota_service_pb2_grpc as monitoring
from yandex.cloud.priv.iot.devices.v1 import quota_service_pb2_grpc as iot
from yandex.cloud.priv.loadbalancer.v1 import quota_service_pb2_grpc as load_balancer
from yandex.cloud.priv.ydb.v1 import quota_service_pb2_grpc as ydb
from yandex.cloud.priv.dns.v1 import quota_service_pb2_grpc as dns

logger = logging.getLogger(__name__)


class RpcError(Enum):
    """This class convert grpc digit error codes to messages."""

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


class RestRequest:
    """Base request wrapper for app.yandex.Cloud Quota REST API."""

    BASE_HEADERS = {
        'content-type': 'application/json'
    }

    def __init__(self,
                 client=None,
                 headers=None,
                 timeout=None,
                 **kwargs):

        self.headers = headers or self.BASE_HEADERS.copy()
        self.client = self.set_and_return_client(client)
        self.timeout = int(timeout) if timeout else 10

    def set_authorization(self, token):
        self.headers.update({'X-YaCloud-SubjectToken': token})

    def set_and_return_client(self, client):
        self.client = client

        if self.client and self.client.token:
            self.set_authorization(self.client.token)

        return self.client

    @staticmethod
    def _convert_camel_to_snake(text):
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', text)
        return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

    @staticmethod
    def _object_hook(obj: dict):
        cleaned_object = {}
        for key, value in obj.items():
            key = RestRequest._convert_camel_to_snake(key.replace('-', '_'))

            if len(key) and key[0].isdigit():
                key = '_' + key

            cleaned_object.update({key: value})
        return cleaned_object

    def _parse(self, json_data: bytes):
        try:
            decoded_s = json_data.decode('utf-8')
            data = json.loads(decoded_s)  # object_hook=Request._object_hook for future
        except UnicodeDecodeError:
            log.debug('Logging raw invalid UTF-8 response:\n%r', json_data)
            raise QuotaServiceError('Server response could not be decoded using UTF-8')
        except (AttributeError, ValueError):
            log.critical(AttributeError, ValueError)
            raise QuotaServiceError('Invalid server response')

        if data is not None and data.get('result') is None:
            data = {
                'result': data,
                'error': RpcError(data.get('code')) if data.get('code') else data.get('error', 'Error'),
                'error_description': data.get('message') or data.get('detail')
            }
        else:
            data = {
                'result': data,
                'error': 'Error',
                'error_description': ''
            }
            log.warning(str(data))

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
            log.error(str(resp.text)+str(resp.status_code))
            raise Unauthorized(message)
        elif resp.status_code == 400:
            log.error(str(resp.text)+str(resp.status_code))
            raise BadRequest(message)
        elif resp.status_code == 403:
            log.error(str(resp.text)+str(resp.status_code))
            raise PermissionDenied(message)
        elif resp.status_code == 404:
            log.error(str(resp.text)+str(resp.status_code))
            raise ResourceNotFound(message)
        elif resp.status_code in (409, 413):
            log.error(resp.text, resp.status_code)
            raise HTTPError(f'HTTP {resp.status_code} – {message}')

        elif resp.status_code == 510:
            log.error(str(resp.text)+str(resp.status_code))
            raise FeatureNotImplemented(message)
        else:
            log.error(str(resp.text)+str(resp.status_code))
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


class gRPCRequest:
    """Base request wrapper for app.yandex.Cloud Quota gRPC API."""

    __SERVICES__ = {
        "compute": compute,
        "functions": functions,
        "triggers": triggers,
        "kubernetes": kubernetes,
        "container-registry": container_registry,
        "instance-group": instance_group,
        "virtual-private-cloud": vpc,
        "monitoring": monitoring,
        "internet-of-things": iot,
        "load-balancer": load_balancer,
        "ydb": ydb,
        "dns": dns
    }

    def __init__(self,
                 client=None,
                 timeout=None):

        self.client = client
        self.timeout = int(timeout) if timeout else 2

    @property
    def token(self):
        if self.client and self.client.token:
            return self.client.token
        raise QuotaServiceError('Token is empty.')

    @property
    def ssl(self):
        if self.client and self.client.ssl:
            return self.client.ssl
        raise SSLError(f'Required root certificate not found in {self.client.ssl}.')

    def quota_service(self, service: str, endpoint: str):
        with open(self.ssl, 'rb') as cert:
            ssl_creds = grpc.ssl_channel_credentials(cert.read())
        call_creds = grpc.access_token_call_credentials(self.token)
        chan_creds = grpc.composite_channel_credentials(ssl_creds, call_creds)

        stub = self.__SERVICES__[service].QuotaServiceStub
        channel = grpc.secure_channel(endpoint, chan_creds)

        return stub(channel)

    def get(self, service: str, endpoint: str, subject_id: str):
        stub = self.quota_service(service, endpoint)
        req = quota_pb2.GetQuotaRequest(cloud_id=subject_id)
        resp = stub.Get(req)

        data = {
            "cloud_id": resp.cloud_id,
            "metrics": [
                {"name": metric.name, "limit": metric.limit, "usage": metric.usage} for metric in resp.metrics
            ]
        }

        return data

    def get_default(self, service: str, endpoint: str):
        stub = self.quota_service(service, endpoint)
        req = quota_pb2.GetQuotaDefaultRequest()
        resp = stub.Get(req)

        data = {
            "cloud_id": resp.cloud_id,
            "metrics": [
                {"name": metric.name, "limit": metric.limit, "usage": metric.usage} for metric in resp.metrics
            ]
        }

        return data

    def update(self, service: str, endpoint: str, data: dict):
        stub = self.quota_service(service, endpoint)
        req = quota_pb2.UpdateQuotaMetricRequest(
            cloud_id=data.get('subject_id'),
            metric=quota_pb2.MetricLimit(
                name=data.get('name'),
                limit=data.get('limit'),
            ),
        )

        result = stub.UpdateMetric(req)
        return result

    # FIXME
    def batch_update(self, service: str, endpoint: str, data: dict):
        metrics = list()
        for metric in data.get('metrics'):
            metrics.append(quota_pb2.MetricLimit(
                name=metric.get('name'),
                limit=metric.get('limit')))
        print(metrics)
        stub = self.quota_service(service, endpoint)
        req = quota_pb2.BatchUpdateQuotaMetricsRequest(
            cloud_id=data.get('subject_id'),
            metrics=metrics,
        )

        result = stub.UpdateMetric(req)
        return result
