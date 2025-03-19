"""
Provider for instance group service
"""
import logging
from collections import namedtuple

from flask import current_app

from dbaas_common import retry
from yandex.cloud.priv.microcosm.instancegroup.v1 import (
    instance_group_service_pb2,
    instance_group_service_pb2_grpc,
)
import grpc
from ..core.exceptions import DbaasClientError
from ..utils import iam_jwt
from dbaas_common.tracing import grpc_channel_tracing_interceptor


class InstanceGroupNotFoundError(DbaasClientError):
    """
    InstanceGroup not found error
    """


class UserComputeApiError(DbaasClientError):
    """
    Base user compute error
    """


class InstanceGroup:
    """
    Instance group provider
    """

    def __init__(self, token=None):
        config = current_app.config['INSTANCE_GROUP_SERVICE']
        self.url = config['url']
        self.grpc_timeout = config['grpc_timeout']
        self.logger = logging.getLogger(current_app.config['LOGCONFIG_BACKGROUND_LOGGER'])
        self.token = token

        self.iam_jwt = None
        if not token:
            self.iam_jwt = iam_jwt.get_provider()

    def _create_instance_group_request(self):
        call_credentials = grpc.access_token_call_credentials(
            self.iam_jwt.get_iam_token() if self.iam_jwt is not None else self.token
        )
        composite_credentials = grpc.composite_channel_credentials(
            grpc.ssl_channel_credentials(),
            call_credentials,
        )
        channel = grpc_channel_tracing_interceptor(grpc.secure_channel(self.url, composite_credentials))
        return instance_group_service_pb2_grpc.InstanceGroupServiceStub(channel)

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    def _handle_instance_group_request(self, request_type, method_type):
        try:
            response = namedtuple('GrpcResponse', ['data', 'meta'])
            response.data = method_type(request_type, timeout=self.grpc_timeout)
            self.logger.info('InstanceGroup API url=%s request=%s response=%s', self.url, request_type, response.data)
            return response
        except grpc.RpcError as rpc_error:
            code = None
            # Workaround for too general error class in gRPC
            if hasattr(rpc_error, 'code') and callable(rpc_error.code):  # pylint: disable=E1101
                code = rpc_error.code()  # pylint: disable=E1101
            if code == grpc.StatusCode.NOT_FOUND:
                if hasattr(request_type, 'instance_group_id'):
                    raise InstanceGroupNotFoundError(f'Instance group {request_type.instance_group_id} is not found')
                elif hasattr(request_type, 'folder_id'):
                    raise InstanceGroupNotFoundError(f'Folder {request_type.folder_id} is not found')
            raise

    def get(self, instance_group_id: str):
        """
        Get instance group by id
        """
        instance_group_service = self._create_instance_group_request()
        return self._handle_instance_group_request(
            request_type=instance_group_service_pb2.GetInstanceGroupRequest(instance_group_id=instance_group_id),
            method_type=instance_group_service.Get,
        )

    def list(self, folder_id: str):
        """
        List instance groups in folder
        """
        instance_group_service = self._create_instance_group_request()
        return self._handle_instance_group_request(
            request_type=instance_group_service_pb2.ListInstanceGroupsRequest(folder_id=folder_id),
            method_type=instance_group_service.List,
        )

    def list_instances(self, instance_group_id: str):
        """
        List instances in instance group
        """
        instance_group_service = self._create_instance_group_request()
        return self._handle_instance_group_request(
            request_type=instance_group_service_pb2.ListInstanceGroupInstancesRequest(
                instance_group_id=instance_group_id,
            ),
            method_type=instance_group_service.ListInstances,
        )
