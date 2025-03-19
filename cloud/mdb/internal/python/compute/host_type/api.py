from cloud.mdb.internal.python import grpcutil
from dbaas_common import tracing
from typing import Callable, Dict, NamedTuple
from yandex.cloud.priv.compute.v1 import host_type_service_pb2, host_type_service_pb2_grpc

from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from .models import HostTypeModel


class HostTypeClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0
    page_size: int = 1000


class HostTypeClient:
    __host_type_service = None

    def __init__(
        self,
        config: HostTypeClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[GRPCError, Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='HostTypeClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    @property
    def _host_type_service(self):
        if self.__host_type_service is None:
            channel = grpcutil.new_grpc_channel(self.config.transport)
            self.__host_type_service = ComputeGRPCService(
                self.logger,
                channel,
                host_type_service_pb2_grpc.HostTypeServiceStub,
                timeout=self.config.timeout,
                get_token=self.token_getter,
                error_handlers=self.error_handlers,
            )
        return self.__host_type_service

    @client_retry
    @tracing.trace('Compute Get Host Type')
    def get_host_type(self, host_type_id: str) -> HostTypeModel:
        """
        Get host type info
        """
        tracing.set_tag('compute.host_type.id', host_type_id)
        request = host_type_service_pb2.GetHostTypeRequest()
        request.host_type_id = host_type_id
        return HostTypeModel.from_api(self._host_type_service.Get(request))
