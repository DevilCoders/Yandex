"""
IAM JWT token getter with cache
"""

import time
import uuid
from typing import NamedTuple
import logging

import grpc
import jwt

from dbaas_common import retry, tracing
from cloud.mdb.internal.python import grpcutil
from yandex.cloud.priv.iam.v1 import iam_token_service_pb2, iam_token_service_pb2_grpc


class Config(NamedTuple):
    """
    IAM JWT config
    """

    transport: grpcutil.Config
    audience: str
    request_expire: int
    expire_thresh: int


class SACreds(NamedTuple):
    """
    Service Account credentials
    """

    service_account_id: str
    key_id: str
    private_key: str


class IamJwt:
    """
    IAM JWT token provider
    """

    def __init__(self, config: Config, logger: logging.LoggerAdapter, sa_creds: SACreds) -> None:
        if not sa_creds.service_account_id or not sa_creds.key_id or not sa_creds.private_key:
            raise RuntimeError('Secrets are not initialized')
        self.config = config
        self.logger = logger
        self.sa_creds = sa_creds
        self._token = None
        self._expire_ts = None

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    @tracing.trace('IAM JWT request token')
    def _request_token(self):
        """
        Request token with jwt
        """
        tracing.set_tag('iam_jwt.service_account_id', self.sa_creds.service_account_id)
        tracing.set_tag('iam_jwt.key_id', self.sa_creds.key_id)

        request = iam_token_service_pb2.CreateIamTokenRequest()

        now = int(time.time())

        payload = {
            'aud': self.config.audience,
            'iss': self.sa_creds.service_account_id,
            'iat': now,
            'exp': now + self.config.request_expire,
        }
        encoded = jwt.encode(
            payload,
            self.sa_creds.private_key,
            algorithm='PS256',
            headers={'kid': self.sa_creds.key_id},
        )
        request.jwt = encoded

        request_id = str(uuid.uuid4())

        metadata = [('x-request-id', request_id)]

        extra = self.logger.extra.copy()
        extra['request_id'] = request_id

        self.logger.logger.info(
            'Requesting token for service account %s key %s in %s',
            self.sa_creds.service_account_id,
            self.sa_creds.key_id,
            self.config.transport.url,
            extra=extra,
        )

        channel = grpcutil.new_grpc_channel(self.config.transport)
        stub = iam_token_service_pb2_grpc.IamTokenServiceStub(channel)
        try:
            res = stub.Create(request, metadata=metadata, timeout=5.0)
        except grpc.RpcError as exc:
            self.logger.logger.error('Token request failed: %s', repr(exc), extra=extra)
            raise
        finally:
            channel.close()

        self.logger.logger.info(
            'Got token for for service account %s key %s in %s',
            self.sa_creds.service_account_id,
            self.sa_creds.key_id,
            self.config.transport.url,
            extra=extra,
        )

        self._token = res.iam_token
        self._expire_ts = res.expires_at.ToSeconds()
        return self._token

    def get_token(self):
        """
        Get iam token
        """
        if self._token and time.time() + self.config.expire_thresh < self._expire_ts:
            return self._token
        return self._request_token()
