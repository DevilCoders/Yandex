from yandex.cloud.priv.iam.v1 import iam_token_service_pb2
from yandex.cloud.priv.iam.v1 import iam_token_service_pb2_grpc

from cloud.billing.utils.clients.grpc import base as grpc_base


class YCTokenClient(grpc_base.BaseGrpcClient):
    service_stub_cls = iam_token_service_pb2_grpc.IamTokenServiceStub

    def create_token(self, **kwargs):
        request = iam_token_service_pb2.CreateIamTokenRequest(**kwargs)
        return self.writing_stub.Create(request).iam_token


def get_token_client(endpoint_config=None):
    return grpc_base.get_client(
        client_cls=YCTokenClient,
        endpoint_config=endpoint_config,
        tracer=None,
        metadata=tuple()
    )
