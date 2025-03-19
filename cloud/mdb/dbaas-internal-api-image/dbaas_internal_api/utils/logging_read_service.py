"""
Provider for logging service
"""

import datetime
import logging
from collections import namedtuple
from typing import Optional, List

import grpc
from flask import current_app, request

from ..core.exceptions import DbaasClientError, PreconditionFailedError
from ..utils import iam_jwt
from ..utils.iam_token import get_iam_client
from dbaas_common import retry, tracing
from yandex.cloud.priv.logging.v1.log_reading_service_pb2_grpc import LogReadingServiceStub
from yandex.cloud.priv.logging.v1.log_reading_service_pb2 import (
    Criteria,
    ReadRequest,
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


class LoggingReadService:
    """
    Logging service log group provider
    """

    def __init__(self):
        config = current_app.config['LOGGING_READ_SERVICE_CONFIG']
        self.url = config['url']
        self.grpc_timeout = config['grpc_timeout']
        self.logger = logging.getLogger(current_app.config['LOGCONFIG_BACKGROUND_LOGGER'])
        self.user_service_account_token = None  # used for authorize check in validation
        self.service_account_id = None
        self.iam_jwt = iam_jwt.get_provider()

    def _init_channel(self):
        if self.service_account_id:
            self.user_service_account_token = get_iam_client().issue_iam_token(
                service_account_id=self.service_account_id,
            )
            call_credentials = grpc.access_token_call_credentials(self.user_service_account_token)
        else:
            # Passing iam token from incoming request
            call_credentials = grpc.access_token_call_credentials(request.auth_context['token'])

        composite_credentials = grpc.composite_channel_credentials(
            grpc.ssl_channel_credentials(),
            call_credentials,
        )
        channel = grpc.secure_channel(self.url, composite_credentials)
        tracing_channel = tracing.grpc_channel_tracing_interceptor(channel)
        self.log_reading_service = LogReadingServiceStub(tracing_channel)
        return channel

    def work_as_user_specified_account(self, service_account_id):
        """
        Work as user specified service account
        """
        self.service_account_id = service_account_id
        self.logger.info('Working as service account %s from now on', self.service_account_id)

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    def _handle_request(self, request, method_type, response_meta_type=None, headers: dict = None) -> GrpcResponse:
        try:
            metadata = headers or {}
            response_data = method_type(
                request=request,
                timeout=self.grpc_timeout,
                metadata=tuple(metadata.items()) or None,
            )
            self.logger.info('Logging API url=%s request=%s response=%s', self.url, request, response_data)
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
                if hasattr(request, 'log_group_id'):
                    raise LogGroupNotFoundError(f'Log group {request.log_group_id} is not found')
                elif hasattr(request, 'folder_id'):
                    raise LogGroupNotFoundError(f'Folder {request.folder_id} is not found')
            if code == grpc.StatusCode.PERMISSION_DENIED:
                message = 'Permission denied to read from log group'
                if hasattr(request, 'criteria') and hasattr(request.criteria, 'log_group_id'):
                    message += f' {request.criteria.log_group_id}'
                raise LogGroupPermissionDeniedError(message)
            raise

    def read(
        self,
        log_group_id: Optional[str] = None,
        filter_string: Optional[str] = None,
        resource_types: List[Optional[str]] = None,
        resource_ids: List[Optional[str]] = None,
        page_size: Optional[int] = 1000,
        page_token: Optional[str] = None,
        since: Optional[datetime.datetime] = None,
        until: Optional[datetime.datetime] = None,
    ):
        """
        Read logs in folder
        """

        if page_token:
            read_request = ReadRequest(page_token=page_token)
        else:
            criteria = Criteria(
                log_group_id=log_group_id,
                page_size=page_size,
                filter=filter_string,
                resource_types=resource_types,
                resource_ids=resource_ids,
            )
            if since:
                criteria.since.FromDatetime(since)
            if until:
                criteria.until.FromDatetime(until)
            read_request = ReadRequest(criteria=criteria)
        with self._init_channel():
            response = self._handle_request(
                request=read_request,
                method_type=self.log_reading_service.Read,
            )
            return response


def get_logging_read_service() -> LoggingReadService:
    """
    Get logging according to config and flags
    """

    return current_app.config['LOGGING_READ_SERVICE']()
