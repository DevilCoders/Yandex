from typing import Callable, Optional, Any

from google.rpc import status_pb2

from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from yandex.cloud.priv.operation import operation_pb2


def enum_from_api(raw_state, klass):
    for item in klass:
        if raw_state == item.value:
            return item
    raise NotImplementedError(
        'Unexpected state of {name}: "{state}". Expect one of: {options}'.format(
            name=klass.__name__,
            state=type(raw_state),
            options=','.join(f'"{item.name}"' for item in klass),
        )
    )


class OperationModel:
    operation_id: str
    done: bool
    error: Optional[GRPCError]
    # Any model object defined in this client.
    response: Optional[Any]
    __raw_data: Any

    def operation_from_api(
        self,
        logger: MdbLoggerAdapter,
        raw_data: operation_pb2.Operation,
        error_parser: Callable[[MdbLoggerAdapter, status_pb2.Status], GRPCError],
    ):
        self.__raw_data = raw_data
        self.operation_id = raw_data.id
        self.done = raw_data.done
        if raw_data.HasField('error'):
            raw_error = raw_data.error
            logger.info('raw error "%s", t: "%s"', raw_error, type(raw_error))
            self.error = error_parser(logger, raw_error)
        else:
            self.error = None
        return self

    def parse_response(self, response_model: Any = None):
        if self.__raw_data.HasField('response'):
            grpc_message = response_model._grpc_model()
            if not self.__raw_data.response.Unpack(grpc_message):
                raise RuntimeError('Operation.response cannot be unpacked in {}'.format(response_model._grpc_model))
            self.response = response_model.from_api(grpc_message)
        else:
            raise RuntimeError('Expected response in operation, but did not get one, check GRPC spec')
        return self

    def __str__(self):
        return f'OperationModel "f{self.operation_id}" done={self.done}'
