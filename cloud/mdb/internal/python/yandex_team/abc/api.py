from cloud.mdb.internal.python import grpcutil
from dbaas_common import tracing
from typing import Callable, Dict, NamedTuple, Type

from yandex.cloud.priv.team.integration.v1 import abc_service_pb2, abc_service_pb2_grpc

from cloud.mdb.internal.python.grpcutil import WrappedGRPCService
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from .models import ABCModel


class ABCClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0


class ABCClient:
    """
    Yandex Team Integration ABC client
    """

    __abc_service = None

    def __init__(
        self,
        config: ABCClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[Type[GRPCError], Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='ABCClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

        channel = grpcutil.new_grpc_channel(self.config.transport)
        self.__abc_service = WrappedGRPCService(
            self.logger,
            channel,
            abc_service_pb2_grpc.AbcServiceStub,
            timeout=self.config.timeout,
            get_token=self.token_getter,
            error_handlers=self.error_handlers,
        )

    @client_retry
    @tracing.trace('Yandex Team Integration Resolve')
    def resolve(self, abc_slug: str) -> ABCModel:
        request = abc_service_pb2.ResolveRequest()
        request.abc_slug = abc_slug
        return ABCModel.from_api(self.__abc_service.Resolve(request))
