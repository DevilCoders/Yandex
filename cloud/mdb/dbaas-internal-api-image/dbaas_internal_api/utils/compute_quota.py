"""
Provider for compute quota service
"""

import logging
from collections import namedtuple
from threading import local
from collections import Counter

import grpc
from flask import current_app, g

from dbaas_common import retry
from yandex.cloud.priv.quota import quota_pb2
from yandex.cloud.priv.compute.v1 import (
    quota_service_pb2_grpc,
)
from ..core.exceptions import DbaasClientError
from ..utils import iam_jwt
from dbaas_common.tracing import grpc_channel_tracing_interceptor


THREAD_CONTEXT = local()
compute_quota_types = (
    'compute.instances.count',
    'compute.instanceCores.count',
    'compute.instanceMemory.size',
    'compute.hddDisks.size',
    'compute.ssdDisks.size',
    'compute.disks.count',
)


class CloudNotFoundError(DbaasClientError):
    """
    Cloud not found error
    """


class ComputeQuota:
    """
    Compute quota service provider
    """

    def __init__(self, config):
        self.logger = logging.getLogger(config['LOGCONFIG_BACKGROUND_LOGGER'])
        quota_service_config = config['COMPUTE_QUOTA_SERVICE']
        self.url = quota_service_config['url']
        self.grpc_timeout = quota_service_config['grpc_timeout']
        self.iam_jwt = iam_jwt.get_provider()

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    def _handle_request(self, request_type, method_type):
        try:
            response = namedtuple('GrpcResponse', ['data', 'meta'])
            response.data = method_type(request_type, timeout=self.grpc_timeout)
            self.logger.info('Compute Quota API url=%s request=%s response=%s', self.url, request_type, response.data)
            return response
        except grpc.RpcError as rpc_error:
            code = None
            # Workaround for too general error class in gRPC
            if hasattr(rpc_error, 'code') and callable(rpc_error.code):  # pylint: disable=E1101
                code = rpc_error.code()  # pylint: disable=E1101
            if code == grpc.StatusCode.NOT_FOUND:
                raise CloudNotFoundError(f'Cloud {request_type.cloud_id} is not found')
            raise

    def get(self, cloud_id: str):
        """
        Get compute quota by cloud id
        """

        call_credentials = grpc.access_token_call_credentials(self.iam_jwt.get_iam_token())
        composite_credentials = grpc.composite_channel_credentials(
            grpc.ssl_channel_credentials(),
            call_credentials,
        )
        channel = grpc_channel_tracing_interceptor(grpc.secure_channel(self.url, composite_credentials))
        quota_service = quota_service_pb2_grpc.QuotaServiceStub(channel)

        return self._handle_request(
            request_type=quota_pb2.GetQuotaRequest(cloud_id=cloud_id),
            method_type=quota_service.Get,
        )

    def get_available_compute_quota(self, cloud_id: str):
        available_cloud_quota = {}
        metrics = self.get(cloud_id=cloud_id).data.metrics
        for metric in metrics:
            available_cloud_quota[metric.name] = metric.limit - metric.usage
        return available_cloud_quota


def check_compute_quota(requested_resources: Counter):
    """
    Compares requested resources with available Compute quota
       and raises an exception if requested resources do not fit in available Compute quota
    """

    compute_quota_provider = current_app.config['COMPUTE_QUOTA_PROVIDER'](current_app.config)
    available_cloud_quota = compute_quota_provider.get_available_compute_quota(cloud_id=g.cloud['cloud_ext_id'])

    error_messages = []
    for quota_type in compute_quota_types:
        if quota_type in requested_resources:
            available = available_cloud_quota[quota_type]
            requested = requested_resources[quota_type]
            if requested > available_cloud_quota[quota_type]:
                error_messages.append(f' {quota_type}. Requested: {requested}. Available: {available}.')
    if error_messages:
        error_messages_string = '\n'.join(error_messages)
        raise DbaasClientError(
            f'Insufficient Compute quota for: \n{error_messages_string}\n'
            'Increase Compute quota or try to make a smaller cluster.'
        )
