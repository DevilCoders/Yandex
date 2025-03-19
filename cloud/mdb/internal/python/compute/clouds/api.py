from cloud.mdb.internal.python import grpcutil
from contextlib import AbstractContextManager
from dbaas_common import tracing
from typing import Callable, Dict, NamedTuple, List, Type
from yandex.cloud.priv.resourcemanager.v1 import cloud_service_pb2, cloud_service_pb2_grpc

from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter


class CloudsClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0


class CloudsClient(AbstractContextManager):
    __cloud_service = None

    def __init__(
        self,
        config: CloudsClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[Type[GRPCError], Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='CloudsClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    def __enter__(self) -> 'CloudsClient':
        channel = grpcutil.new_grpc_channel(self.config.transport)
        self.__cloud_service = ComputeGRPCService(
            self.logger,
            channel,
            cloud_service_pb2_grpc.CloudServiceStub,
            timeout=self.config.timeout,
            get_token=self.token_getter,
            error_handlers=self.error_handlers,
        )
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.__cloud_service.channel.close()
        self.__cloud_service = None

    @property
    def _cloud_service(self):
        return self.__cloud_service

    @client_retry
    @tracing.trace('Resource Manager Cloud GetPermissionStages')
    def get_permission_stages(self, cloud_id: str) -> List[str]:
        request = cloud_service_pb2.GetPermissionStagesRequest()
        request.cloud_id = cloud_id
        response = self._cloud_service.GetPermissionStages(request)
        return response.permission_stages
