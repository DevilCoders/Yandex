
from grpc._channel import _InactiveRpcError
import logging

from yandex.cloud.priv.quotamanager.v1 import quota_request_service_pb2_grpc as quota_request_service
from yandex.cloud.priv.quotamanager.v1 import operation_service_pb2_grpc as operation_service

from .helpers import get_grpc_channel


class GrpcChannel:

    @property
    def quota_service_channel(self):
        channel = get_grpc_channel(quota_request_service.QuotaRequestServiceStub,
                                   self.endpoint,
                                   self.token)

        return channel

    @property
    def quota_operation_channel(self):
        channel = get_grpc_channel(operation_service.OperationServiceStub,
                                   self.endpoint,
                                   self.token)
        return channel

    @staticmethod
    def use_grpc(channel, request):
        result = None

        try:
            result = channel(request)
        except _InactiveRpcError as e:
            print(e.details())
            logging.debug(e.details())
            return f'{e.details()}'

        return result
