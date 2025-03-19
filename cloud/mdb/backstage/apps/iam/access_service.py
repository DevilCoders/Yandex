import logging
from django.conf import settings

import yandex.cloud.priv.servicecontrol.v1.access_service_pb2 as access_service_pb2
import yandex.cloud.priv.servicecontrol.v1.access_service_pb2_grpc as access_service_pb2_grpc

import cloud.mdb.internal.python.logs as logs
import cloud.mdb.internal.python.grpcutil as grpcutil

import cloud.mdb.backstage.apps.iam.jwt as jwt


logger = logs.MdbLoggerAdapter(logging.getLogger(__name__), extra={})
token_getter = jwt.TokenGetter()
URL = f'{settings.CONFIG.iam.access_service.host}:{settings.CONFIG.iam.access_service.port}'


class AccessService:
    def __init__(self):
        self.config = grpcutil.Config(
            cert_file=None,
            url=URL,
        )

    def _get_grpc_service(self):
        return grpcutil.WrappedGRPCService(
            logger=logs.MdbLoggerAdapter(logger, {}),
            channel=grpcutil.new_grpc_channel(config=self.config),
            grpc_service=access_service_pb2_grpc.AccessServiceStub,
            timeout=5,
            get_token=token_getter.get_token,
            error_handlers={},
        )

    def authorize(self, subject, permission):
        service = self._get_grpc_service()
        req = access_service_pb2.AuthorizeRequest(
            subject=subject,
            permission=permission,
            resource_path=settings.CONFIG.iam.permissions.resource_path,
        )
        return service.Authorize(req)
