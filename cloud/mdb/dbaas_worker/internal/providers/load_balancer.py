import uuid
from dataclasses import dataclass
from typing import Any, Callable

from .compute import UserExposedComputeRunningOperationsLimitError, UserExposedComputeApiError
from .iam_jwt import IamJwt
from cloud.mdb.dbaas_worker.internal.providers.common import BaseProvider
from cloud.mdb.internal.python.loadbalancer import (
    LoadBalancerClient,
    LoadBalancerClientConfig,
)
from cloud.mdb.internal.python.grpcutil import exceptions as grpcutil_errors
from cloud.mdb.internal.python import grpcutil


@dataclass
class GrpcResponse:
    data: Any = None
    meta: Any = None


def quota_error(err: grpcutil_errors.ResourceExhausted) -> None:
    if err.message == 'The limit on maximum number of active operations has exceeded.':
        raise UserExposedComputeRunningOperationsLimitError(message=err.message, err_type=err.err_type, code=err.code)
    raise UserExposedComputeApiError(message=err.message, err_type=err.err_type, code=err.code)


def gen_config(ca_path: str) -> Callable:
    def from_url(url: str) -> grpcutil.Config:
        return grpcutil.Config(
            url=url,
            cert_file=ca_path,
        )

    return from_url


class LoadBalancer(BaseProvider):
    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.url = self.config.loadbalancer_service.url
        self.grpc_timeout = self.config.loadbalancer_service.grpc_timeout
        self.iam_jwt = IamJwt(
            config,
            task,
            queue,
            service_account_id=self.config.compute.service_account_id,
            key_id=self.config.compute.key_id,
            private_key=self.config.compute.private_key,
        )

        transport_config = gen_config(self.config.compute.ca_path)
        error_handlers = {
            grpcutil_errors.ResourceExhausted: quota_error,
        }
        self.client = LoadBalancerClient(
            config=LoadBalancerClientConfig(
                transport=transport_config(self.config.loadbalancer_service.url),
            ),
            logger=self.logger,
            token_getter=self.get_token,
            error_handlers=error_handlers,
        )
        self._idempotence_ids = dict()

    def get_token(self):
        return self.iam_jwt.get_token()

    def _get_idempotence_id(self, key):
        """
        Get local idempotence id by key
        """
        if key not in self._idempotence_ids:
            self._idempotence_ids[key] = str(uuid.uuid4())
        return self._idempotence_ids[key]
