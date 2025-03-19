"""
gRPC client to iam token service
"""

import logging
from typing import Dict

import grpc

from flask import current_app
from yandex.cloud.priv.iam.v1 import iam_token_service_pb2, iam_token_service_pb2_grpc
from .api import IAM
from dbaas_common import retry, tracing
from ...utils.request_context import get_x_request_id
from ...utils import iam_jwt
from cloud.mdb.internal.python import grpcutil
from ..logs import get_logger
from cloud.mdb.internal.python.logs import MdbLoggerAdapter


def extract_code(rpc_error):
    """
    Extract grpc.StatusCode from grpc.RpcError
    """
    code = None
    # Workaround for too general error class in gRPC
    if hasattr(rpc_error, 'code') and callable(rpc_error.code):  # pylint: disable=E1101
        code = rpc_error.code()  # pylint: disable=E1101
    return code


def giveup(rpc_error):
    """
    Defines whether to retry operation on exception or give up
    """
    code = extract_code(rpc_error)
    if code in (grpc.StatusCode.UNAVAILABLE, grpc.StatusCode.INTERNAL, grpc.StatusCode.DEADLINE_EXCEEDED):
        return False
    return True


def grpc_metadata():
    """
    Forward some metadata to gRPC services
    """
    return (('x-request-id', get_x_request_id()),)


class IAMClient(IAM):
    """
    IAM API provider
    """

    def __init__(self, config: Dict) -> None:
        self.config = config
        self.grpc_channel_config = grpcutil.Config(
            url=self.config['url'],
            cert_file=self.config.get('cert_file', ''),
            server_name=self.config.get('server_name', ''),
        )
        self.logger = MdbLoggerAdapter(get_logger(), extra={'request_id': get_x_request_id()})
        self.timeout = self.config.get('timeout', 5.0)

    def _logger(self):
        logger = logging.LoggerAdapter(
            logging.getLogger(current_app.config['LOGCONFIG_BACKGROUND_LOGGER']),
            extra={
                'request_id': get_x_request_id(),
            },
        )
        return logger

    @tracing.trace('IAM TokenServer.CreateIamTokenForServiceAccountRequest')
    @retry.on_exception((grpc.RpcError,), giveup=giveup, max_tries=3)
    def issue_iam_token(self, service_account_id: str):
        """
        Issue IAM token for specified service_account_id
        """
        req = iam_token_service_pb2.CreateIamTokenForServiceAccountRequest()
        req.service_account_id = service_account_id

        channel = grpcutil.new_grpc_channel(self.grpc_channel_config)
        token_service = grpcutil.WrappedGRPCService(
            logger=self.logger,
            channel=channel,
            grpc_service=iam_token_service_pb2_grpc.IamTokenServiceStub,
            timeout=self.timeout,
            get_token=iam_jwt.get_provider().get_iam_token,
            error_handlers={},
        )

        try:
            self._logger().debug(f'Starting request to issue iam token for ' f'service_account_id {service_account_id}')
            resp = token_service.CreateForServiceAccount(req)
            return resp.iam_token
        except grpc.RpcError:
            self._logger().warning('Failed to issue iam token')
            raise
        finally:
            channel.close()
