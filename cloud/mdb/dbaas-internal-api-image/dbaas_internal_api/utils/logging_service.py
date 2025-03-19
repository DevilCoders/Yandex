"""
Provider for logging service
"""

import logging
from collections import namedtuple
from typing import Optional

import grpc
from flask import current_app

from ..core.exceptions import DbaasClientError, PreconditionFailedError
from ..utils import iam_jwt
from ..utils.iam_token import get_iam_client
from dbaas_common import retry, tracing
from yandex.cloud.priv.logging.v1 import (
    log_group_service_pb2,
    log_group_service_pb2_grpc,
)


GrpcResponse = namedtuple('GrpcResponse', ['data', 'meta'])


class LoggingApiError(DbaasClientError):
    """
    Base logging service error
    """


class LogGroupPermissionDeniedError(PreconditionFailedError):
    """
    LogGroupNotFoundError not found error
    """


class LogGroupNotFoundError(PreconditionFailedError):
    """
    LogGroupNotFoundError not found error
    """


class OperationNotFoundError(DbaasClientError):
    """
    Operation not found error
    """


class OperationTimeoutError(DbaasClientError):
    """
    Operation timeout error
    """


class LoggingService:
    """
    Logging service log group provider
    """

    def __init__(self):
        config = current_app.config['LOGGING_SERVICE_CONFIG']
        self.url = config['url']
        self.grpc_timeout = config['grpc_timeout']
        self.logger = logging.getLogger(current_app.config['LOGCONFIG_BACKGROUND_LOGGER'])
        self.user_service_account_token = None  # used for authorize check in validation
        self.service_account_id = None
        self.iam_jwt = iam_jwt.get_provider()

    def _init_channel(self):
        self.user_service_account_token = get_iam_client().issue_iam_token(
            service_account_id=self.service_account_id,
        )
        call_credentials = grpc.access_token_call_credentials(self.user_service_account_token)
        composite_credentials = grpc.composite_channel_credentials(
            grpc.ssl_channel_credentials(),
            call_credentials,
        )
        channel = grpc.secure_channel(self.url, composite_credentials)
        tracing_channel = tracing.grpc_channel_tracing_interceptor(channel)
        self.log_group_service = log_group_service_pb2_grpc.LogGroupServiceStub(tracing_channel)
        return channel

    def work_as_user_specified_account(self, service_account_id):
        """
        Work as user specified service account
        """
        self.service_account_id = service_account_id
        self.logger.info('Working as service account %s from now on', self.service_account_id)

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    def _handle_request(self, request_type, method_type, response_meta_type=None, headers: dict = None):
        try:
            metadata = headers or {}
            response_data = method_type(
                request=request_type,
                timeout=self.grpc_timeout,
                metadata=tuple(metadata.items()) or None,
            )
            self.logger.info('Logging API url=%s request=%s response=%s', self.url, request_type, response_data)
            response_meta = None
            if response_meta_type:
                response_meta = response_meta_type()
                response_data.metadata.Unpack(response_meta)
            return GrpcResponse(data=response_data, meta=response_meta)
        except grpc.RpcError as rpc_error:
            code = None
            # Workaround for too general error class in gRPC
            if hasattr(rpc_error, 'code') and callable(rpc_error.code):  # type: ignore
                code = rpc_error.code()  # type: ignore
            if code == grpc.StatusCode.NOT_FOUND:
                if hasattr(request_type, 'log_group_id'):
                    raise LogGroupNotFoundError(f'Log group {request_type.log_group_id} is not found')
                elif hasattr(request_type, 'folder_id'):
                    raise LogGroupNotFoundError(f'Folder {request_type.folder_id} is not found')
            if code == grpc.StatusCode.PERMISSION_DENIED:
                if hasattr(request_type, 'log_group_id'):
                    raise LogGroupPermissionDeniedError(
                        f'Permission denied to get log group {request_type.log_group_id}'
                    )
            raise

    def get(self, log_group_id: Optional[str]):
        """
        Get log group by id
        """
        with self._init_channel():
            response = self._handle_request(
                request_type=log_group_service_pb2.GetLogGroupRequest(log_group_id=log_group_id),
                method_type=self.log_group_service.Get,
            )
            if not response or not response.data or not response.data.folder_id:
                raise LogGroupNotFoundError(f'No folder id in response to get log group {log_group_id}')
            return response

    def get_default(self, folder_id: str):
        """
        Get default log group by folder id
        """
        with self._init_channel():
            response = self._handle_request(
                request_type=log_group_service_pb2.GetDefaultLogGroupRequest(folder_id=folder_id),
                method_type=self.log_group_service.GetDefault,
            )
            if not response or not response.data or not response.data.folder_id:
                raise LogGroupNotFoundError(f'Could not get default log group for folder {folder_id}')
            return response

    def list(self, folder_id: str):
        """
        List log groups in folder
        """
        with self._init_channel():
            response = self._handle_request(
                request_type=log_group_service_pb2.ListLogGroupsRequest(folder_id=folder_id),
                method_type=self.log_group_service.List,
            )
            return response


def get_logging_service() -> LoggingService:
    """
    Get logging according to config and flags
    """

    return current_app.config['LOGGING_SERVICE']()
