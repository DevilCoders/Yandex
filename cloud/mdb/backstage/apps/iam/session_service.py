import logging
from django.conf import settings

import yandex.cloud.priv.oauth.v1.session_service_pb2 as session_service_pb2
import yandex.cloud.priv.oauth.v1.session_service_pb2_grpc as session_service_pb2_grpc

import cloud.mdb.internal.python.logs as logs
import cloud.mdb.internal.python.grpcutil as grpcutil
import cloud.mdb.internal.python.grpcutil.exceptions as grpc_exceptions

import cloud.mdb.backstage.apps.iam.jwt as jwt


logger = logs.MdbLoggerAdapter(logging.getLogger(__name__), extra={})
token_getter = jwt.TokenGetter()
URL = f'{settings.CONFIG.iam.session_service.host}:{settings.CONFIG.iam.session_service.port}'


class AuthenticateRedirectError(grpc_exceptions.GRPCError):
    def __init__(self, redirect_url):
        self.redirect_url = redirect_url


class WrappedSessionService(grpcutil.WrappedGRPCService):
    def specific_errors(self, rich_details):
        for detail in rich_details:
            if detail.Is(session_service_pb2.AuthorizationRequired.DESCRIPTOR):
                error = session_service_pb2.AuthorizationRequired()
                detail.Unpack(error)
                return AuthenticateRedirectError(error.authorize_url)


class SessionService:
    def __init__(self):
        self.config = grpcutil.Config(
            cert_file=None,
            url=URL,
        )

    def _get_grpc_service(self):
        return WrappedSessionService(
            logger=logs.MdbLoggerAdapter(logger, {}),
            channel=grpcutil.new_grpc_channel(config=self.config),
            grpc_service=session_service_pb2_grpc.SessionServiceStub,
            timeout=5,
            get_token=token_getter.get_token,
            error_handlers={},
        )

    def check(self, cookie):
        service = self._get_grpc_service()
        req = session_service_pb2.CheckSessionRequest(cookie_header=cookie)
        return service.Check(req)

    def create(self, access_token):
        service = self._get_grpc_service()
        req = session_service_pb2.CreateSessionRequest(access_token=access_token)
        return service.Create(req)
