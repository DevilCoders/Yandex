from cloud.mdb.internal.python import grpcutil
from dbaas_common import tracing
from typing import Callable, Dict, NamedTuple
from yandex.cloud.priv.compute.v1 import operation_service_pb2, operation_service_pb2_grpc

from cloud.mdb.internal.python.compute.models import OperationModel
from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter


class OperationsClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0

    @staticmethod
    def make(url: str, cert_file: str) -> 'OperationsClientConfig':
        return OperationsClientConfig(
            transport=grpcutil.Config(
                url=url,
                cert_file=cert_file,
            ),
        )


class OperationsClient:
    """
    Operations client
    """

    __operation_service = None

    def __init__(
        self,
        config: OperationsClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[GRPCError, Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='OperationsClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    @property
    def _operation_service(self):
        if self.__operation_service is None:
            channel = grpcutil.new_grpc_channel(self.config.transport)
            self.__operation_service = ComputeGRPCService(
                self.logger,
                channel,
                operation_service_pb2_grpc.OperationServiceStub,
                timeout=self.config.timeout,
                get_token=self.token_getter,
                error_handlers=self.error_handlers,
            )
        return self.__operation_service

    @client_retry
    @tracing.trace('Compute Operations Get')
    def get_operation(self, operation_id: str) -> OperationModel:
        """
        Get operation by id
        """
        tracing.set_tag('compute.operation.id', operation_id)
        request = operation_service_pb2.GetOperationRequest()
        request.operation_id = str(operation_id)
        with self.logger.context(operation_id=operation_id):
            operation = OperationModel().operation_from_api(
                self.logger,
                self._operation_service.Get(request),
                self._operation_service.error_from_rich_status,
            )
            if operation.error:
                self.logger.info('Operation "%s" ended with error: "%s"', operation_id, operation.error)
        return operation
