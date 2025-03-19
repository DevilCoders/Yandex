from .quotaservice import GrpcChannel
from yandex.cloud.priv.quotamanager.v1 import quota_request_service_pb2 as quota_request_service_pb2
from yandex.cloud.priv.quotamanager.v1 import operation_service_pb2 as operation_service_pb2


class QuotaOperations(GrpcChannel):
    def __init__(self,
                 sa_id: str = '',
                 token: str = '',
                 endpoint: str = ''):
        self.endpoint = endpoint
        self.token = token

    def list_requests(self, cloud_id: str, token: str = ''):
        if token:
            self.token = token
        result = self.use_grpc(self.quota_service_channel.List,
                               quota_request_service_pb2.ListQuotaRequestRequest(cloud_id=cloud_id))
        return result

    def get_request(self, request_id: str, token: str = ''):
        if token:
            self.token = token
        result = self.use_grpc(self.quota_service_channel.Get,
                               quota_request_service_pb2.GetQuotaRequestRequest(quota_request_id=request_id))
        return result

    def update_request(self, request_id: str, limits: dict, token: str = ''):
        if token:
            self.token = token
        result = self.use_grpc(self.quota_service_channel.Update,
                               quota_request_service_pb2.UpdateQuotaRequestRequest(quota_request_id=request_id,
                                                                                   update_actions=limits))
        return result

    def get_operation_request(self, operation_id: str, token: str = ''):
        if token:
            self.token = token
        result = self.use_grpc(self.quota_operation_channel.Get,
                               operation_service_pb2.GetOperationRequest(operation_id=operation_id))
        return result
