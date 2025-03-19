import socket
import grpc

from yc_auth.exceptions import handle_grpc_error
from cloud.gauthling.yc_auth.proto.scms_agent_pb2 import AuthRequest, Status
from cloud.gauthling.yc_auth.proto.scms_agent_pb2_grpc import SCMSAgentStub


_UNDEFINED = object()


class SCMSAgentClient(object):
    def __init__(self, target, credentials=None, options=None, timeout=10):
        if credentials is None:
            self.channel = grpc.insecure_channel(target, options)
        else:
            self.channel = grpc.secure_channel(target, credentials, options)
        self.stub = SCMSAgentStub(self.channel)
        self.timeout = timeout

    @handle_grpc_error
    def auth(self, key_id, signed_string, signature, datestamp, algorithm, ip_address, is_url=False, timeout=_UNDEFINED):
        if timeout is _UNDEFINED:
            timeout = self.timeout

        request = AuthRequest(
            key_id=key_id,
            signed_string=signed_string,
            signature=signature,
            datestamp=datestamp,
            algorithm=algorithm,
            ip_address=socket.inet_pton(
                socket.AF_INET if '.' in ip_address else socket.AF_INET6,
                ip_address,
            ),
            is_url=is_url,
        )
        return self.stub.Auth(request, timeout=timeout)

    @handle_grpc_error
    def auth_stream(self, request_iterator, timeout=_UNDEFINED):
        if timeout is _UNDEFINED:
            timeout = self.timeout

        return self.stub.AuthStream(request_iterator, timeout=timeout)

    @handle_grpc_error
    def health(self, timeout=_UNDEFINED):
        if timeout is _UNDEFINED:
            timeout = self.timeout

        return self.stub.Health(Status(), timeout=timeout)
