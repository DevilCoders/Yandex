"""
YC.Compute interaction helper
"""
import os
import time
from collections import namedtuple

import grpc
from retrying import retry

from yandex.cloud.priv.microcosm.instancegroup.v1 import (
    instance_group_pb2,
    instance_group_service_pb2,
    instance_group_service_pb2_grpc,
    operation_service_pb2,
    operation_service_pb2_grpc,
)


class InstanceGroupApiError(Exception):
    """
    Base compute instance group service error
    """


class InstanceGroupNotFoundError(Exception):
    """
    InstanceGroup not found error
    """


class OperationNotFoundError(Exception):
    """
    Operation not found error
    """


class OperationTimeoutError(Exception):
    """
    Operation timeout error
    """


class InstanceGroupApi(object):
    """
    Instance group provider
    """

    def __init__(self, config, logger):
        self.config = config
        self.logger = logger
        self.url = self.config['instance_group_grpc_url']
        self.grpc_timeout = int(self.config['grpc_timeout'])
        self.instance_group_service = None
        self.operation_service = None
        self.managed_instance_type = instance_group_pb2.ManagedInstance

        self._init_channels(token=self.config['token'])

    def _get_ssl_creds(self, cert_file):
        if not cert_file or not os.path.exists(cert_file):
            return grpc.ssl_channel_credentials()
        with open(cert_file, 'rb') as file_handler:
            certs = file_handler.read()
        return grpc.ssl_channel_credentials(root_certificates=certs)

    def _init_channels(self, token):
        call_credentials = grpc.access_token_call_credentials(token)
        composite_credentials = grpc.composite_channel_credentials(
            self._get_ssl_creds(self.config['ca_path']),
            call_credentials,
        )
        channel = grpc.secure_channel(self.url, composite_credentials)
        self.instance_group_service = instance_group_service_pb2_grpc.InstanceGroupServiceStub(channel)
        self.operation_service = operation_service_pb2_grpc.OperationServiceStub(channel)

    def wait(self, operation_id: str, timeout: int = 600):
        """
        Wait operation by id for specified timeout in seconds
        """
        deadline = time.time() + timeout
        while time.time() < deadline:
            operation = self.get_operation(operation_id)
            if operation.data.done:
                return
        message = f'Operation {operation_id} was not finished before timeout {timeout} seconds expired.'
        raise OperationTimeoutError(message)

    def get_operation(self, operation_id: str):
        """
        Get operation by id
        """
        return self._handle_instance_group_request(
            request_type=operation_service_pb2.GetOperationRequest(operation_id=operation_id),
            method_type=self.operation_service.Get,
        )

    @retry(wait_fixed=5000, stop_max_attempt_number=5)
    def _handle_instance_group_request(self, request_type, method_type, response_meta_type=None, headers: dict = None):
        try:
            response = namedtuple('GrpcResponse', ['data', 'meta'])
            metadata = headers or {}
            response.data = method_type(
                request=request_type,
                timeout=self.grpc_timeout,
                metadata=tuple(metadata.items()) or None,
            )
            self.logger.debug('InstanceGroup API url=%s request=%s response=%s', self.url, request_type, response.data)
            if response_meta_type:
                response.meta = response_meta_type()
                response.data.metadata.Unpack(response.meta)
            return response
        except grpc.RpcError as rpc_error:
            code = None
            # Workaround for too general error class in gRPC
            if hasattr(rpc_error, 'code') and callable(rpc_error.code):  # pylint: disable=E1101
                code = rpc_error.code()  # pylint: disable=E1101
            if code == grpc.StatusCode.NOT_FOUND:
                if isinstance(request_type, operation_service_pb2.GetOperationRequest):
                    raise OperationNotFoundError(f'Operation {request_type.operation_id} is not found')
                if hasattr(request_type, 'instance_group_id'):
                    raise InstanceGroupNotFoundError(f'Instance group {request_type.instance_group_id} is not found')
                elif hasattr(request_type, 'folder_id'):
                    raise InstanceGroupNotFoundError(f'Folder {request_type.folder_id} is not found')
            raise

    def get(self, instance_group_id: str):
        """
        Get instance group by id
        """
        return self._handle_instance_group_request(
            request_type=instance_group_service_pb2.GetInstanceGroupRequest(instance_group_id=instance_group_id),
            method_type=self.instance_group_service.Get,
        )

    def list(self, folder_id: str):
        """
        List instance groups in folder
        """
        return self._handle_instance_group_request(
            request_type=instance_group_service_pb2.ListInstanceGroupsRequest(
                folder_id=folder_id,
                view=instance_group_service_pb2.InstanceGroupView.FULL,
            ),
            method_type=self.instance_group_service.List,
        )

    def list_references(self, instance_group_id):
        """
        Get instance group references
        """
        return self._handle_instance_group_request(
            request_type=instance_group_service_pb2.ListInstanceGroupReferencesRequest(
                instance_group_id=instance_group_id
            ),
            method_type=self.instance_group_service.ListReferences,
        )

    def delete(self, instance_group_id: str, wait: bool = True, referrer_id: str = None) -> str:
        """
        Delete instance group
        """
        operation_response = self._handle_instance_group_request(
            request_type=instance_group_service_pb2.DeleteInstanceGroupRequest(instance_group_id=instance_group_id),
            method_type=self.instance_group_service.Delete,
            response_meta_type=instance_group_service_pb2.DeleteInstanceGroupMetadata,
            headers={'referrer-id': referrer_id} if referrer_id else {},
        )
        operation_id = operation_response.data.id
        if wait:
            self.wait(operation_id=operation_id)
        return operation_id
