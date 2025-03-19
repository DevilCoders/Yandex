from cloud.mdb.internal.python import grpcutil
from dbaas_common import tracing
from typing import Callable, Dict, NamedTuple
from yandex.cloud.priv.compute.v1 import host_group_service_pb2, host_group_service_pb2_grpc

from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from .models import HostGroupModel


class HostGroupsClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0
    page_size: int = 1000


class HostGroupsClient:
    __host_groups_service = None

    def __init__(
        self,
        config: HostGroupsClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[GRPCError, Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='HostGroupsClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    @property
    def _host_groups_service(self):
        if self.__host_groups_service is None:
            channel = grpcutil.new_grpc_channel(self.config.transport)
            self.__host_groups_service = ComputeGRPCService(
                self.logger,
                channel,
                host_group_service_pb2_grpc.HostGroupServiceStub,
                timeout=self.config.timeout,
                get_token=self.token_getter,
                error_handlers=self.error_handlers,
            )
        return self.__host_groups_service

    @client_retry
    @tracing.trace('Compute Get Host Group')
    def get_host_group(self, host_group_id: str) -> HostGroupModel:
        """
        Get host group info
        """
        tracing.set_tag('compute.host_group.id', host_group_id)
        request = host_group_service_pb2.GetHostGroupRequest()
        request.host_group_id = host_group_id
        return HostGroupModel.from_api(self._host_groups_service.Get(request))
